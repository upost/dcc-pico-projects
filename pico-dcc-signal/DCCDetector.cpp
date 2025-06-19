#include "DCCDetector.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "Device.h"
#include "pico_led.h"

// these influence the timing! Don't use
//#define DEBUG_BITS
//#define DEBUG_DCC

#define ONE_PULSE_MIN_LENGTH 40

// singleton
DCCDetector *detector;
uint8_t char_counter=0;
uint64_t last_rise=0,last_fall=0;
uint8_t blink=1;
uint8_t max_pr_len=0;
uint16_t irq_took=0;
int irq_pin=0;
uint16_t count_0=0,count_1=0,count_invalid=0;

void blink_led() {
    blink = blink?0:1;
    pico_set_led(blink);
}


// Interrupt callback
void gpio_callback(uint gpio, uint32_t events) {
    if(gpio!=irq_pin) return;
    //int value=(events&GPIO_IRQ_EDGE_RISE)?1:0;
    //gpio_put(STATUS_LED,value);
    uint64_t last_rise_bak=last_rise;
    uint64_t now = time_us_64();
    
    if(events & GPIO_IRQ_EDGE_RISE) {
        last_rise = now;
    } else if(events & GPIO_IRQ_EDGE_FALL) {
        // falling
        if(last_rise>0) {
            // calculate length, then 
            uint64_t pulse_length_us = now-last_rise;   
            // ignore pulse length below threshold
            if(pulse_length_us<ONE_PULSE_MIN_LENGTH) {
                count_invalid++;
                // invalid, ignore
                irq_took = (uint16_t) pulse_length_us;
            } else
            // identify One Bit (as of NMRA DCC standard)
            if(pulse_length_us>=ONE_PULSE_MIN_LENGTH && pulse_length_us<=64) {
                
                count_1++;
                #ifdef DEBUG_BITS
                putchar('1');
                char_counter++;
                #endif
                detector->onBitReceived(1);
            } else
            // identify Null Bit (as of NMRA DCC standard)
            if(pulse_length_us>=90 && pulse_length_us<=10000) {
                
                count_0++;
                #ifdef DEBUG_BITS
                putchar('0');
                char_counter++;
                #endif
                detector->onBitReceived(0);
            }
        }
    }
    #ifdef DEBUG_BITS
    if(char_counter>120) {
        char_counter=0;
        putchar(13);putchar(10);
    }
    #endif


}


// void listen_for_dcc() {
//     while(1) {
//         int value=gpio_get(detector->getInputPin()) ;
//         gpio_put(STATUS_LED,value);
//     }
// }


DCCDetector::DCCDetector(uint8_t _input_pin, Device* _devices[], uint8_t _device_count) {
    irq_pin = input_pin = _input_pin;
    last_rise = 0;
    detector = this;
    devices = _devices;
    device_count = _device_count;
    
    gpio_set_dir(input_pin, GPIO_IN);
    gpio_set_pulls(input_pin,false,true);
    pico_set_led(false);
}

void DCCDetector::start() {
    gpio_set_irq_enabled_with_callback(input_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

uint DCCDetector::getInputPin() {
    return input_pin;
}

void DCCDetector::process(uint gpio,uint32_t events) {
    
}

// implements a state machine controlled by the received bits, collecting NMRA DCC packets and calling handleCommand() for each.
void DCCDetector::onBitReceived(uint8_t bit) {
    // collect bits.
    // once something in the buffer is identified as a valid DCC command, pass to decoder.

    switch(status) {
        // the preambel has 10 or more ONE bits followed by a 0 (the startbit). 1111111111...11110
        case STATUS_WAITING_FOR_PREAMBLE: {
            if(bit) {
                one_bit_counter++;
            } else {
                if(one_bit_counter>max_pr_len) {
                    max_pr_len=one_bit_counter;
                    //printf("max_pr: %d     \n",max_pr_len);
                }
                if(one_bit_counter>=10) {
                    // this is a preamble
                    one_bit_counter=0;
                    status=STATUS_WAITING_FOR_ADDRESS;
                    detected_address=0;
                    detected_address_bit_counter=0;
                    pico_set_led(1);
                    #ifdef DEBUG_DCC                    
                    putchar('A');
                    #endif          
                } else {
                    // not a preamble, reset counter
                    one_bit_counter=0;
                }
            }
            break;
        }

        // then 8 bits (address data byte) first bit ist most significant.
        case STATUS_WAITING_FOR_ADDRESS: {
            #ifdef DEBUG_DCC
            putchar('p');
            putchar(bit?'1':'0'); char_counter++;
            #endif
            detected_address <<= 1;
            detected_address|=bit;
            detected_address_bit_counter++;
            if(detected_address_bit_counter==8) {
                status=STATUS_WAITING_FOR_DATASTARTBIT;        
                #ifdef DEBUG_DCC
                putchar(' ');
                #endif
            }
            break;
        }

        // then a 0 follows (data byte start bit)
        case STATUS_WAITING_FOR_DATASTARTBIT: {
            #ifdef DEBUG_DCC            
            putchar('a');
            #endif
            if(!bit) {
                #ifdef DEBUG_DCC
                putchar('D');
                #endif
                status=STATUS_WAITING_FOR_INSTRUCTIONS;
                detected_data_counter= detected_data_bit_counter = 0 ;
                detected_data[0] = detected_data[1] = detected_data[2] = 0;
                
            }
            break;
        }

        // then instruction data byte(s)
        case STATUS_WAITING_FOR_INSTRUCTIONS: {
            #ifdef DEBUG_DCC
            putchar('d');
            #endif
            #ifdef DEBUG_DCC
            putchar(bit?'1':'0'); char_counter++;
            #endif
            detected_data[detected_data_counter] <<= 1;
            detected_data[detected_data_counter] |= bit;
            detected_data_bit_counter++;
            if(detected_data_bit_counter==8) {
                detected_data_bit_counter=0;
                detected_data_counter++;
                status=STATUS_WAITING_FOR_START_OR_END_BIT;
                
            }
            break;
        }

        // then collect more bytes, until a 1 follows. last is the error detection data byte
        case STATUS_WAITING_FOR_START_OR_END_BIT: {
            if(bit==0) {
                
                // another data byte follows unless we already have 3
                if(detected_data_counter==3) {
                    //gpio_put(STATUS_LED,0);
                    
                    #ifdef DEBUG_DCC
                    putchar('e');
                    #endif
                    handleCommand();
                    pico_set_led(0);
                    status = STATUS_WAITING_FOR_PREAMBLE;
                } else {
                    status = STATUS_WAITING_FOR_INSTRUCTIONS;
                }
            } else {
                // then packet end bit (1)
                #ifdef DEBUG_DCC
                putchar('e');
                #endif
                pico_set_led(false);

                handleCommand();
                status = STATUS_WAITING_FOR_PREAMBLE;
                #ifdef DEBUG_DCC
                puts(".");
                #endif
            }
            break;
        }   
    }
    //putchar('\r');
}


void DCCDetector::handleCommand() {
    #ifdef DEBUG_DCC
    printf("\rdcc: adr %b, dta %b            ",detected_address,detected_data[0]);
    #endif
    if(detected_address == 0b11111111 && detected_data[0]== 0) {
        // idle packet, do nothing
        #ifdef DEBUG_DCC
        puts("idle");
        #endif
        return;
    }
    for(int i=0; i<device_count; i++) {
        // try to handle command, on success break
        if(devices[i]->handleCommand(detected_address, detected_data)) break;
    }
    
}

uint16_t DCCDetector::timing() {
    return irq_took;
}

void DCCDetector::debug() {
    printf("counted 0: %d\ncounted 1: %d\ncounted -: %d (%dms)\n",count_0,count_1,count_invalid,irq_took);
}