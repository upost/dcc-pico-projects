#include "DCCDetector.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "Device.h"

//#define DEBUG_BITS

// singleton
DCCDetector *detector;
uint8_t char_counter=0;
uint64_t last_rise=0;
//uint8_t blink=1;

// Interrupt callback
void gpio_callback(uint gpio, uint32_t events) {
    //int value=(events&GPIO_IRQ_EDGE_RISE)?1:0;
    //gpio_put(STATUS_LED,value);
    //blink = blink?0:1;
    //detector->process(gpio,events);

    uint64_t now = time_us_64();
    //if(gpio!=input_pin) return;
    if(events & GPIO_IRQ_EDGE_RISE) {
        // 1
        last_rise = now;
        

    } else if(events & GPIO_IRQ_EDGE_FALL) {
        //gpio_put(STATUS_LED,0);
        // falling
        if(last_rise>0) {
            // calculate length, then 
            uint64_t pulse_length_us = now-last_rise;   
            // identify One Bit (as of NMRA DCC standard)
            if(pulse_length_us>=52 && pulse_length_us<=64) {
                //putchar('1');
                //counter++;
                detector->onBitReceived(1);
            } else
            // identify Null Bit (as of NMRA DCC standard)
            if(pulse_length_us>=90 && pulse_length_us<=10000) {
                //putchar('0');
                //counter++;
                detector->onBitReceived(0);
            }
        }
    }
    
    if(char_counter>120) {
        char_counter=0;
        putchar(13);putchar(10);
    }
}

// void listen_for_dcc() {
//     while(1) {
//         int value=gpio_get(detector->getInputPin()) ;
//         gpio_put(STATUS_LED,value);
//     }
// }


DCCDetector::DCCDetector(uint8_t _input_pin, Device* _devices[], uint8_t _device_count) {
    input_pin = _input_pin;
    last_rise = 0;
    detector = this;
    devices = _devices;
    device_count = _device_count;
    gpio_init(STATUS_LED);
    gpio_set_dir(STATUS_LED, GPIO_OUT);
    gpio_set_dir(input_pin, GPIO_IN);
    gpio_set_pulls(input_pin,false,true);
    gpio_put(STATUS_LED,0);
}

void DCCDetector::start() {
//    multicore_launch_core1(listen_for_dcc);
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
                if(one_bit_counter>=10) {
                    // this is a preamble
                    one_bit_counter=0;
                    status=STATUS_WAITING_FOR_ADDRESS;
                    detected_address=0;
                    detected_address_bit_counter=0;
                    gpio_put(STATUS_LED,0);
                    #ifdef DEBUG_BITS                    
                    putchar('A');
                    #endif                    
                }
            }
            break;
        }

        // then 8 bits (address data byte) first bit ist most significant.
        case STATUS_WAITING_FOR_ADDRESS: {
            #ifdef DEBUG_BITS
            putchar(bit?'1':'0'); char_counter++;
            #endif
            detected_address <<= 1;
            detected_address|=bit;
            detected_address_bit_counter++;
            if(detected_address_bit_counter==8) {
                status=STATUS_WAITING_FOR_DATASTARTBIT;        
                #ifdef DEBUG_BITS
                putchar(' ');
                #endif
            }
            break;
        }

        // then a 0 follows (data byte start bit)
        case STATUS_WAITING_FOR_DATASTARTBIT: {
            if(!bit) {
                #ifdef DEBUG_BITS
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
            #ifdef DEBUG_BITS
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
                    handleCommand();
                    status = STATUS_WAITING_FOR_PREAMBLE;
                } else {
                    status = STATUS_WAITING_FOR_INSTRUCTIONS;
                }
            } else {
                // then packet end bit (1)
                handleCommand();
                status = STATUS_WAITING_FOR_PREAMBLE;
                #ifdef DEBUG_BITS
                putchar('.');
                #endif
            }
            break;
        }
        
    }
    
    
}


void DCCDetector::handleCommand() {
    if(detected_address == 0b11111111 && detected_data[0]== 0) {
        // idle packet, do nothing
        return;
    }
    for(int i=0; i<device_count; i++) {
        devices[i]->handleCommand(detected_address, detected_data);
    }
    
}
