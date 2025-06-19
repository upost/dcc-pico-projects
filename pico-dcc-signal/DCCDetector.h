#ifndef DCCDETECTOR_H
#define DCCDETECTOR_H
#include <stdio.h>
#include "pico/stdlib.h"
#include "Device.h"

#define STATUS_WAITING_FOR_PREAMBLE 1
#define STATUS_WAITING_FOR_STARTBIT 2
#define STATUS_WAITING_FOR_ADDRESS 3
#define STATUS_WAITING_FOR_DATASTARTBIT 4
#define STATUS_WAITING_FOR_INSTRUCTIONS 5
#define STATUS_WAITING_FOR_START_OR_END_BIT 6

#define STATUS_LED 25

class DCCDetector {
    private:
        uint input_pin;
        
        Device** devices;
        uint8_t device_count;
        uint8_t status= STATUS_WAITING_FOR_PREAMBLE;
        uint8_t one_bit_counter=0;
        uint8_t detected_address,detected_address_bit_counter;
        uint8_t detected_data[3];
        uint8_t detected_data_counter,detected_data_bit_counter;
        void handleCommand();
    public:
        DCCDetector(uint8_t _input_pin, Device* _devices[], uint8_t _device_count);
        void onBitReceived(uint8_t a_bit);
        void process(uint gpio,uint32_t events);
        void start();
        uint getInputPin();
        uint16_t timing();
        void debug();
};

#endif
