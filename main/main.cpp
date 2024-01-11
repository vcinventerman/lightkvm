#include <stdio.h>
#include <stdlib.h>
// #include <sys/reent.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tusb_console.h"
#include "tusb_cdc_acm.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "soc/periph_defs.h"
#include "hal/cpu_hal.h"

#include "lightkvm/print.h"
#include "lightkvm/activityLed.h"
#include "lightkvm/usb.h"
#include "lightkvm/unit.h"

// #define POWER_BUTTON_GPIO GPIO_NUM_16
#define POWER_BUTTON_GPIO GPIO_NUM_14

#define KEY_PRESS_DURATION (100_ms)
#define POWER_BUTTON_START_DURATION (250_ms)
#define POWER_BUTTON_RESET_DURATION (11_s)

// Upper bound on time from power button press to feeding the watchdog
#define COMPUTER_BOOT_DURATION (30_s)

void initPowerButton()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << POWER_BUTTON_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}

void pressPowerButton(TickType_t duration = POWER_BUTTON_START_DURATION)
{
    printf("Pressing power button for %d ms\n", int(duration * portTICK_PERIOD_MS));

    gpio_set_level(POWER_BUTTON_GPIO, false);
    vTaskDelay(duration);
    gpio_set_level(POWER_BUTTON_GPIO, true);
}

void forceShutDown()
{
    pressPowerButton(POWER_BUTTON_RESET_DURATION);
}

void pressKey(uint8_t key)
{
    printf("Pressing key %d for %d ms\n", (int)key, int(KEY_PRESS_DURATION * portTICK_PERIOD_MS));

    uint8_t keycode[6] = {key};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(KEY_PRESS_DURATION);
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, nullptr);
}

bool readUsbQueue()
{
    size_t cnt = 0;
    uint8_t out[50] = {};
    tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0, out, sizeof(out) - 1, &cnt);
    if (cnt != 0)
    {
        printf("Got data %s\n", out);
        return true;
    }
    else
    {
        return false;
    }
}

TickType_t lastSerialMessage = 0;
TickType_t lastUsbConnection = 0;

void watchdog(void *params)
{
    while (true)
    {
        if (readUsbQueue())
        {
            lastSerialMessage = xTaskGetTickCount();
        }

        // Not sure whether tud_connected or tud_ready is appropriate here
        if (tud_connected())
        {
            lastUsbConnection = xTaskGetTickCount();
        }

        printf("Status %d %d\n", int(tud_connected()), int(tud_ready()));
        vTaskDelay(1_s);
    }
}

extern "C"
{
    void init()
    {
        initPowerButton();
        ActivityLed::init();

        setupUsb();
        xTaskCreate(watchdog, "watchdog", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 10, nullptr);
    }

    void app_main(void)
    {
        init();

        // Wait the time usually required for the computer to send its first message
        // This is designed to avoid scenarios where the computer turns on of its
        // own accord at the same time as the microcontroller
        vTaskDelay(30_s);

        while (true)
        {
            auto now = xTaskGetTickCount();
            ActivityLed::blink();

            if (now - lastSerialMessage > 300_s)
            {
                // Give up
                printf("System inactive");
            }
            if (now - lastSerialMessage > COMPUTER_BOOT_DURATION)
            {
                // Computer is no longer communicating, restart it
                printf("Missed too many pings, restarting\n");

                // Restore to known state (off)
                forceShutDown();
                vTaskDelay(1_s);

                // Turn on
                pressPowerButton();
                vTaskDelay(30_s);

                // All following operations should not affect the functioning of a system booting,
                // so they can safely assume it is stuck in the BIOS

                // Exit BIOS configuration changed screen
                pressKey(HID_KEY_F1);
                vTaskDelay(5_s);

                // Save and quit
                pressKey(HID_KEY_F10);
                vTaskDelay(1_s);

                // Confirm
                pressKey(HID_KEY_ENTER);

                // Wait for full OS boot
                vTaskDelay(COMPUTER_BOOT_DURATION);
            }

            vTaskDelay(1_s);
        }

        while (true)
        {
            // print("Hey\n");
            printf("Hey Ready:%d\n", (int)tud_ready());
            vTaskDelay(1_s);

            ActivityLed::blink();

            if (!gpio_get_level(GPIO_NUM_0))
            {

                pressKey(HID_KEY_F1);

                vTaskDelay(5_s);

                pressKey(HID_KEY_F10);

                vTaskDelay(1_s);

                pressKey(HID_KEY_ENTER);

                vTaskDelay(60_s);
            }
        }
    }
}