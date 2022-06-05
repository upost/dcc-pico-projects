#include "Signal.h"
#include <pico/stdlib.h>
#include <stdio.h>

#define LIGHT_ON 0
#define LIGHT_OFF 1

#define TEST_PHASE_MS 200

void binprintf(int v)
{
    unsigned int mask=128, bit=0;
    while(bit<8) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
        bit++;
    }
}


// SRCPd uses "strange addresses". To get the real DCC accessory address, calculate -1 and /4
Signal::Signal(int _address, int _led_base, int _mode) : Device((_address-1)/4)
{
         mode = _mode;
         led_green = _led_base;
         led_red1 = _led_base+1;
         led_orange = _led_base+2;
         led_red2 = _led_base+3;
         led_white = _led_base+4;
         current=0;
         inverse =0;
}

void Signal::test() {
    switch_to(SIGNAL_HP0);
    sleep_ms(TEST_PHASE_MS);
    switch_to(SIGNAL_HP1);
    sleep_ms(TEST_PHASE_MS);
    switch_to(SIGNAL_HP2);
    sleep_ms(TEST_PHASE_MS);
    switch_to(SIGNAL_HP0SH1);
    sleep_ms(TEST_PHASE_MS);
    switch_to(SIGNAL_HP0);
}



void Signal::init(int _inverse) {
    inverse=_inverse;
    gpio_init(led_green);
    gpio_init(led_red1);
    gpio_init(led_red2);
    gpio_init(led_orange);
    gpio_init(led_white);

    gpio_set_pulls(led_green,true,false);
    gpio_set_pulls(led_red1,true,false);
    gpio_set_pulls(led_red2,true,false);
    gpio_set_pulls(led_orange,true,false);
    gpio_set_pulls(led_white,true,false);

    gpio_set_dir(led_green, GPIO_OUT);
    gpio_set_dir(led_red1, GPIO_OUT);
    gpio_set_dir(led_red2, GPIO_OUT);
    gpio_set_dir(led_orange, GPIO_OUT);
    gpio_set_dir(led_white, GPIO_OUT);

    switch_to(SIGNAL_HP0);
}

// internal function for setting of lights. use light_on or light_off for parameters, not 0/1!
void Signal::set_lights(int red1, int green,int orange, int red2, int white) {
    gpio_put(led_red1,inverse?(1-red1):red1);
    gpio_put(led_red2,inverse?(1-red2):red2);
    gpio_put(led_green,inverse?(1-green):green);
    gpio_put(led_orange,inverse?(1-orange):orange);
    gpio_put(led_white,inverse?(1-white):white);
}

// switch to Signalbild SIGNAL_HP0, SIGNAL_HP1 etc.
void Signal::switch_to(int what) {
    // TODO blend
    switch(what) {
        case SIGNAL_HP0: 
            set_lights(LIGHT_ON,LIGHT_OFF,LIGHT_OFF,LIGHT_ON,LIGHT_OFF);
            break;
        case SIGNAL_HP1:
            set_lights(LIGHT_OFF,LIGHT_ON,LIGHT_OFF,LIGHT_OFF,LIGHT_OFF);
            break;
        case SIGNAL_HP2:
            set_lights(LIGHT_OFF,LIGHT_ON,LIGHT_ON,LIGHT_OFF,LIGHT_OFF);
            break;
        case SIGNAL_HP0SH1:
            set_lights(LIGHT_ON,LIGHT_OFF,LIGHT_OFF,LIGHT_OFF,LIGHT_ON);
            break;
    }
    current=what;
};

void Signal::switch_next() {
    if(current==last())
        switch_to(SIGNAL_HP0);
    else 
        switch_to(current+1);
    
}

int Signal::last() {
    switch(mode) {
        case SIGNAL_HAUPTSIGNAL1: return SIGNAL_HP1;
        case SIGNAL_HAUPTSIGNAL2: return SIGNAL_HP2;
        case SIGNAL_AUSFAHRSIGNAL: return SIGNAL_HP0SH1;
    }
    return SIGNAL_HP2;
}

void Signal::handleCommand(uint8_t _address, uint8_t cmd_data[]) {
    
    // TODO check error detection byte cmd_data[1]
    
//    if((_address & 0b10000000) == 0) {
        // loco decoder, ignore
//        return;
//    }

    // process command data - bit 7 must be 1 and bit 6 must be 0
    if((_address & 0b11000000) == 0b10000000) {
        //printf(" accessory ");
        _address &= 63;
        //binprintf(_address);
        //putchar(' ');
        //binprintf(cmd_data[0]);

        // assume accessory decoder packet
        // calculate effective address and compare to ours
        uint8_t eff_decoderAddress = ((((cmd_data[0]^0b01110000)&(7<<4)))<<6) | _address;
        if(address != eff_decoderAddress) return;
        
        // that's for us!
        // get port, output and check for "on"
        uint8_t port = (cmd_data[0]&(3<<1))>>1;
        uint8_t output = (cmd_data[0]&1);
        uint8_t on = (cmd_data[0]&8)>>3;
        
        printf("accessory eff_addr=%d on=%d output=%d port=%d\n",eff_decoderAddress,on,output,port);

        if(on) {
            if(port==0 && output==0) switch_to(SIGNAL_HP0);
            if(port==0 && output==1) switch_to(SIGNAL_HP1);
            if(port==1 && output==0) switch_to(SIGNAL_HP2);
            if(port==1 && output==1) switch_to(SIGNAL_HP0SH1);
        }

    }
}


