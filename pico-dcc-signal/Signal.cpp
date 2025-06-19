#include <pico/stdlib.h>
#include "Signal.h"
#include <stdio.h>
#include "hardware/irq.h"


#define LIGHT_ON 0
#define LIGHT_OFF 1

#define TEST_PHASE_MS 200


#define DEBUG_SWITCH_TO

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
         gpios[0]= led_green = _led_base;
         gpios[1]= led_red1 = _led_base+1;
         gpios[2]= led_orange = _led_base+2;
         gpios[3]= led_red2 = _led_base+3;
         gpios[4]= led_white = _led_base+4;
         current = 0;
         inverse = 0;
         _green= _red1 = _red2 = _orange = _white = 0;
         for(int i=0; i<FADER_COUNT; i++) {
            fader[i]= target[i] = 0;
         }
}

void Signal::test() {
    switch_to(SIGNAL_HP0);
    sleep_ms(TEST_PHASE_MS);
    switch_to(SIGNAL_HP1);
    sleep_ms(TEST_PHASE_MS);
    
    if(current!=last()) {
    switch_to(SIGNAL_HP2);
    sleep_ms(TEST_PHASE_MS);
    }
    if(current!=last()) {
    switch_to(SIGNAL_HP0SH1);
    sleep_ms(TEST_PHASE_MS);
    }
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

void Signal::start_fade(int gpio,int from,int to) {

    // index of fader is gpio - gpios[0]
    target[gpio-gpios[0]]=to*100;
    fader[gpio-gpios[0]]=100-to*100; // 100 or 0    
    
    // test
    //gpio_put(gpio,to);
}


// internal function for setting of lights. use light_on or light_off for parameters, not 0/1!
void Signal::set_lights(int red1, int green,int orange, int red2, int white) {

    // what to fade out
    if(red1 > _red1) { 
        start_fade(led_red1,inverse?(1-_red1):_red1, inverse?(1-red1):red1);
        _red1=red1;
    }
    if(red2 > _red2) { 
        start_fade(led_red2,inverse?(1-_red2):_red2, inverse?(1-red2):red2);
        _red2=red2;
    }
    if(green > _green) { 
        start_fade(led_green,inverse?(1-_green):_green, inverse?(1-green):green);
        _green=green;
    }
    if(orange > _orange) { 
        start_fade(led_orange,inverse?(1-_orange):_orange, inverse?(1-orange):orange);
        _orange=orange;
    }
    if(white > _white) { 
        start_fade(led_white,inverse?(1-_white):_white, inverse?(1-white):white);
        _white=white;
    }

    // what to fade in
    if(red1 < _red1) { 
        start_fade(led_red1,inverse?(1-_red1):_red1, inverse?(1-red1):red1);
        _red1=red1;
    }
    if(red2 < _red2) { 
        start_fade(led_red2,inverse?(1-_red2):_red2, inverse?(1-red2):red2);
        _red2=red2;
    }
    if(green < _green) { 
        start_fade(led_green,inverse?(1-_green):_green, inverse?(1-green):green);
        _green=green;
    }
    if(orange < _orange) { 
        start_fade(led_orange,inverse?(1-_orange):_orange, inverse?(1-orange):orange);
        _orange=orange;
    }
    if(white < _white) { 
        start_fade(led_white,inverse?(1-_white):_white, inverse?(1-white):white);
        _white=white;
    }

}

// switch to Signalbild SIGNAL_HP0, SIGNAL_HP1 etc.
void Signal::switch_to(int what) {

    switch(what) {
        case SIGNAL_HP0: 
            #ifdef DEBUG_SWITCH_TO
                printf("switching %d to hp0\n",address);
            #endif
            set_lights(LIGHT_ON,LIGHT_OFF,LIGHT_OFF,LIGHT_ON,LIGHT_OFF);
            break;
        case SIGNAL_HP1:
            #ifdef DEBUG_SWITCH_TO
                printf("switching %d to hp1\n",address);
            #endif
            set_lights(LIGHT_OFF,LIGHT_ON,LIGHT_OFF,LIGHT_OFF,LIGHT_OFF);
            break;
        case SIGNAL_HP2:
            #ifdef DEBUG_SWITCH_TO
                printf("switching %d to hp2\n",address);
            #endif
            set_lights(LIGHT_OFF,LIGHT_ON,LIGHT_ON,LIGHT_OFF,LIGHT_OFF);
            break;
        case SIGNAL_HP0SH1:
            #ifdef DEBUG_SWITCH_TO
                printf("switching %d to hp0sh1\n",address);
            #endif
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
        case SIGNAL_EINFAHR: return SIGNAL_HP2;
    }
    return SIGNAL_HP2;
}

bool Signal::handleCommand(uint8_t _address, uint8_t cmd_data[]) {
    
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
        if(address != eff_decoderAddress) return false;
        
        // that's for us!
        // get port, output and check for "on"
        uint8_t port = (cmd_data[0]&(3<<1))>>1;
        uint8_t output = (cmd_data[0]&1);
        uint8_t on = (cmd_data[0]&8)>>3;
        
        printf("\naccessory eff_addr=%d on=%d output=%d port=%d   \n",eff_decoderAddress,on,output,port);

        if(on) {
            if(port==0 && output==0) switch_to(SIGNAL_HP0);
            if(port==0 && output==1) switch_to(SIGNAL_HP1);
            if(port==1 && output==0) switch_to(SIGNAL_HP2);
            if(port==1 && output==1) switch_to(SIGNAL_HP0SH1);
            return true;
        }

    }
    return false;
}


inline void soft_pwm(int gpio,int value) {
    gpio_put(gpio,1);
    sleep_us(FADE_DURATION*value);
    gpio_put(gpio,0);
    sleep_us(FADE_DURATION*(100-value));
}

void Signal::process() {
    // do the fade effect for all fades in progress
    for(int i=0; i<FADER_COUNT; i++) {
        if(target[i] > fader[i]) {
            fader[i]++;
            // fade finished?
            if(target[i]==fader[i]) {
                gpio_put(gpios[i],1);
            } else {
                // partial light
                soft_pwm(gpios[i],fader[i]);
                
            }
        } else if(target[i] < fader[i]) {
            fader[i]--;
            // fade finished
            if(fader[i]==0) {
                gpio_put(gpios[i],0);
            } else {
                // partial light
                soft_pwm(gpios[i],fader[i]);
            }
        }
    }

}


