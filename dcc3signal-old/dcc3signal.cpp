/**
 * Copyright (c) 2021 Uwe Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Signal.h"
#include "DCCDetector.h"
#include "pico/stdlib.h"

#define VERSION "v3.3 Jun22 Rade rechts"

#define BUTTON1 28

#define DCC_IN 22



int main() {
    
    stdio_init_all();

    gpio_init(STATUS_LED);
    gpio_set_dir(STATUS_LED, GPIO_OUT);

    gpio_put(STATUS_LED,1);
    
    // GPIO 0+1 reserved for USB serial
    Signal signal1(53, 2, SIGNAL_FORM_EINFAHR); // GPIO 2 3 4 5 6
    signal1.init();

    // init buttons
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1,false);
    gpio_pull_up(BUTTON1);

    printf("RasPico Decoder init - " VERSION " - DCC input GPIO %d\n",DCC_IN);


    // init: test all
    signal1.test();
    

    Device* signals[] = {&signal1};
    size_t signals_count =sizeof(signals)/sizeof(Device*);
    DCCDetector detector(DCC_IN, signals,  signals_count);

    gpio_put(STATUS_LED,0);

    detector.start();

    while(true) {
        if(!gpio_get(BUTTON1)) {
            puts("Test Button pressed");
            detector.debug();
            signal1.switch_next();
            sleep_ms(500);             
            
        }
        tight_loop_contents();   
        for(int i=0; i<signals_count; i++) {
            signals[i]->process();
        }


        //sleep_ms(500);
        //detector.start();
//       int bar = detector.timing()/10; // 5..6   or 9..10
//       for(int i=0; i<15; i++) if(bar>i) putchar('*'); else putchar(' ');
//        putchar('\r');
        //printf("pulse len: %d   \r",detector.timing());


    }


}
