#ifndef DEVICE_H
#define DEVICE_H

#include "pico/stdlib.h"

class Device {
    private:
    public: 
        uint8_t address;
        Device(uint8_t address);
        virtual bool handleCommand(uint8_t address, uint8_t cmd_data[]) =0;
};

#endif
