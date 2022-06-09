if (TARGET tinyusb_device)
        add_executable(dcc3signal
                dcc3signal.cpp
                Signal.cpp
                Device.cpp
                DCCDetector.cpp
                )

        # Pull in our pico_stdlib which pulls in commonly used features
        target_link_libraries(dcc3signal pico_stdlib pico_multicore)

        # enable usb output, disable uart output
        pico_enable_stdio_usb(dcc3signal 1)
        pico_enable_stdio_uart(dcc3signal 0)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(dcc3signal)

        # add url via pico_set_program_url
        example_auto_set_url(dcc3signal)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()