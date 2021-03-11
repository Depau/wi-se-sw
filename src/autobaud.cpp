//
// Created by depau on 3/11/21.
//

#include "autobaud.h"

#include <uart.h>
#include <esp8266_peri.h>
#include <user_interface.h>

// This is uart_detect_baudrate from esp8266-arduino, modified to return the measured value.
// The closest standard rate can be obtained by calling wise_autobaud_closest_std_rate

int wise_autobaud_detect(HardwareSerial *serial) {
    int uart_nr = UART0;
    if (serial == &Serial1) {
        uart_nr = UART1;
    }

    static bool doTrigger = true;

    if (doTrigger) {
        uart_start_detect_baudrate(uart_nr);
        doTrigger = false;
    }

    int32_t divisor = uart_baudrate_detect(uart_nr, 1);
    if (!divisor) {
        return 0;
    }

    doTrigger = true;    // Initialize for a next round
    int32_t baudrate = UART_CLK_FREQ / divisor;

    return baudrate;
}

int wise_autobaud_closest_std_rate(int32_t rawBaud) {
    static const int default_rates[] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400,
                                        256000, 460800, 921600, 1500000, 1843200, 3686400};

    size_t i;
    for (i = 1; i < sizeof(default_rates) / sizeof(default_rates[0]) - 1; i++)    // find the nearest real baudrate
    {
        if (rawBaud <= default_rates[i]) {
            if (rawBaud - default_rates[i - 1] < default_rates[i] - rawBaud) {
                i--;
            }
            break;
        }
    }

    return default_rates[i];
}