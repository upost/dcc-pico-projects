#ifndef SIGNAL_H
#define SIGNAL_H

#include "Device.h"


// Konstanten für _mode
#define SIGNAL_HAUPTSIGNAL1 1
#define SIGNAL_HAUPTSIGNAL2 2
#define SIGNAL_AUSFAHRSIGNAL 3

// Konstanten für switch_to
#define SIGNAL_HP0 0
#define SIGNAL_HP1 1
#define SIGNAL_HP2 2
#define SIGNAL_HP0SH1 3

#define SIGNAL_INVERSE 1




class Signal : public Device {
    private:
     int led_green,led_red1,led_red2,led_orange,led_white,mode,current,inverse;
     int _green,_red1,_red2,_orange,_white;

     void set_lights(int red1,int green,int orange, int red2, int white);
     int last();
     void start_fade(int gpio,int from,int to);

    public:
     Signal(int _address, int _led_base, int _mode);

     void init(int _inverse=0);

     void test();

     void switch_to(int what);

     void switch_next();
     
     void handleCommand(uint8_t address, uint8_t cmd_data[]) override;

};
#endif