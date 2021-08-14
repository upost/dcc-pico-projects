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




class Signal : public Device {
    private:
     int led_green,led_red1,led_red2,led_orange,led_white,mode,current;

     void set_lights(int red1,int green,int orange, int red2, int white);
     int last();

    public:
     Signal(int _address, int _led_base, int _mode);

     void init();

     void test();

     void switch_to(int what);

     void switch_next();
     
     void handleCommand(uint8_t address, uint8_t cmd_data[]) override;

};
#endif