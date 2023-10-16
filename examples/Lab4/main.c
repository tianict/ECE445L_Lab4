/**
 * @file main.c
 * @author Matthew Yu (matthewjkyu@gmail.com)
 * @brief Base for Lab 4.
 * @version 0.1
 * @date 2023-10-13
 * @copyright Copyright (c) 2021
 */

/** General imports. */
#include <stdlib.h>
#include <stdio.h>

/** Device specific imports. */
#include <lib/PLL/PLL.h>
#include <lib/Timer/Timer.h>
#include <lib/UART/UART.h>
#include <lib/ST7735/ST7735.h>


void EnableInterrupts(void);    // Defined in startup.s
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s

#define MSG_LEN 25
#define NUM_TIMERS 3

UART_t uart;
Timer_t timers[4];

static uint8_t mode;
static uint8_t hour;
static uint8_t min;
static uint8_t sec;

void tm4cToMqtt(uint32_t * args) {
    // TODO: Send message.
    uint8_t message[MSG_LEN] = { '\0' };
    uint8_t size = snprintf((char*)message, MSG_LEN, "%d,%d,%d,%d,\n", mode, hour, min, sec);
    UARTSend(uart, message, size);
}

void MqttToTm4c(uint32_t * args) {
    // TODO: Parse message response.
    uint8_t message[MSG_LEN] = { '\0' };
    uint8_t size = UARTReceive(uart, message, MSG_LEN);
}

void tick(uint32_t * args) {
    sec = (sec + 1) % 60;
    if (!sec) {
        min = (min + 1) % 60;
        if (!min) {
            hour = (hour + 1) % 24;
        }
    }
}

int main(void) {
    PLLInit(BUS_80_MHZ);
    EnableInterrupts();

    // UART5 to ESP8266.
    UARTConfig_t config = {
        .module=UART_MODULE_5,
        .baudrate=UART_BAUD_9600,
        .dataLength=UART_BITS_8,
        .isFIFODisabled=false,
        .isTwoStopBits=false,
        .parity=UART_PARITY_DISABLED,
        .isLoopback=false
    };
    uart = UARTInit(config);

    // Timers.
    TimerConfig_t timerConfigs[NUM_TIMERS] = {
        /* The first timer has keyed arguments notated to show you what each positional argument means. */
        {.timerID=TIMER_0A, .period=freqToPeriod(1, MAX_FREQ),   .isIndividual=false,  .prescale=0, .timerTask=tm4cToMqtt, .isPeriodic=true, .priority=0, .timerArgs=NULL},
        {         TIMER_1A,         freqToPeriod(10, MAX_FREQ),                false,            0,            MqttToTm4c,             true,           0,            NULL},
        {         TIMER_2A,         freqToPeriod(60, MAX_FREQ),                false,            0,            tick,                   true,           1,            NULL},

    };
    uint8_t i;
    for (i = 0; i < NUM_TIMERS; ++i) {
        timers[i] = TimerInit(timerConfigs[i]);
    }

    // ST7735 Display. Uses Systick.
    ST7735Init();
    ST7735DrawString(0, 0, "Hello World! Lab 4.", ST7735_WHITE, ST7735_BLACK);

    /* Newline and carriage return special characters are used here so the
        serial output looks nice. */

    DelayMillisec(500);
    uint8_t message1[] = "aaa,Matthew,fbef406^,broker.emqx.io,1883,\n";
    uint8_t size1 = sizeof(message1);
    UARTSend(uart, message1, size1);

    // Wait for connection.
    DelayMillisec(7500);

    // Set default state of time.
    mode = 0;
    sec = 0;
    min = 30;
    hour = 4;

    // Start timers.
    for (i = 0; i < NUM_TIMERS; ++i) {
        TimerStart(timers[i]);
    }
    ST7735DrawString(0, 1, "Starting comm w/ ESP.", ST7735_WHITE, ST7735_BLACK);

    while (1) {}
}
