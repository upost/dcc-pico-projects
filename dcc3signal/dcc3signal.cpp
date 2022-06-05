/**
 * Copyright (c) 2021 Uwe Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Signal.h"
#include "DCCDetector.h"
#include "pico/stdlib.h"

#define BUTTON1 27

#define DCC_IN 21



int main() {

    
    stdio_init_all();

    gpio_init(STATUS_LED);
    gpio_set_dir(STATUS_LED, GPIO_OUT);

    gpio_put(STATUS_LED,1);

    // GPIO 0+1 reserved for USB serial

    
    Signal signal1(37, 2, SIGNAL_AUSFAHRSIGNAL); // GPIO 2 3 4 5 6
    signal1.init();

    Signal signal2(33,7,SIGNAL_AUSFAHRSIGNAL); // GPIO 7 8 9 10 11
    signal2.init();

    Signal signal3(9,16,SIGNAL_AUSFAHRSIGNAL); // GPIO 16 17 18 19 20
    signal3.init(SIGNAL_INVERSE);

    Signal signal4(65,14,SIGNAL_HAUPTSIGNAL1); // GPIO 14 15
    signal4.init();

    // init buttons
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1,false);
    gpio_pull_up(BUTTON1);

    // init: test all
    signal1.test();
    signal2.test();
    signal3.test();
    signal4.test();

    Device* signals[] = {&signal1,&signal2,&signal3,&signal4};
    DCCDetector detector(DCC_IN, signals, sizeof(signals)/sizeof(Device*) );

    gpio_put(STATUS_LED,0);

    detector.start();

    printf("Decoder init\n");

    while(true) {
        if(!gpio_get(BUTTON1)) {
            signal1.switch_next();
            signal2.switch_next();
            signal3.switch_next();
            signal4.switch_next();
            sleep_ms(500);             
        }
    }


}
