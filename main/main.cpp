#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <tusb_console.h>
#include <tusb_cdc_acm.h>
#include <sdkconfig.h>
#include <driver/gpio.h>
#include <soc/periph_defs.h>
#include <hal/cpu_hal.h>
#include <esp_ota_ops.h>

#include <lightkvm/print.h>
#include <lightkvm/activityLed.h>
#include <lightkvm/usb.h>
#include <lightkvm/unit.h>
#include <lightkvm/wireless.h>
#include <lightkvm/update.h>

// GPIO of the signal line for the power button
constexpr gpio_num_t POWER_BUTTON_GPIO = GPIO_NUM_14;

constexpr TickType_t KEY_PRESS_DURATION = 100_ms;
constexpr TickType_t POWER_BUTTON_START_DURATION = 250_ms;
constexpr TickType_t  POWER_BUTTON_RESET_DURATION = 11_s;

// Time from power button press to feeding the watchdog (takes 45s on my machine)
constexpr TickType_t COMPUTER_BOOT_DURATION = 60_s;

// Upper bound on time from power button press to feeding the watchdog (takes 120s on my machine since GRUB waits for 30s after some events)
constexpr TickType_t COMPUTER_MAX_BOOT_DURATION = 180_s;

// Time to initiate a reboot if the computer has yet to mount the USB keyboard
// This is a bit shorter as the BIOS will usually mount it (takes 16s on my machine)
constexpr TickType_t COMPUTER_USB_READY_DURATION = 30_s;

// Must be 32 bit on ESP32 so that writes are atomic (not actually since the upper word is always zero)
// Since ticks happen 100 times for second, this is not going to rollover
TickType_t lastSerialMessage = 0;

// How often to change status of the indicator LED
TickType_t ledFreq = 250_ms;

void initPowerButton()
{
    gpio_config_t config = 
    {
        .pin_bit_mask = (1ULL << POWER_BUTTON_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&config);
}

void pressPowerButton(TickType_t duration = POWER_BUTTON_START_DURATION)
{
    printf("Pressing power button for %d ms\n", int(duration * portTICK_PERIOD_MS));

    gpio_set_level(POWER_BUTTON_GPIO, false);
    vTaskDelay(duration);
    gpio_set_level(POWER_BUTTON_GPIO, true);
}

// On my computer, holding the power button for a long time forces it to shut down, or keeps it off if it already is
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

void watchdog(void *params)
{
    while (true)
    {
        if (readUsbQueue())
        {
            lastSerialMessage = xTaskGetTickCount();
        }

        printf("Status %d %d\n", int(tud_connected()), int(tud_ready()));
        vTaskDelay(1_s);
    }
}

void statusIndicator(void *params)
{
    while (true)
    {
        static bool toggle = false;
        gpio_set_level(BLINK_GPIO, toggle = !toggle);
        vTaskDelay(ledFreq);
    }
}

void restart()
{
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
    vTaskDelay(COMPUTER_MAX_BOOT_DURATION);
}

extern "C"
{
    void init()
    {
        printf("LightKVM Version %d\n", VERSION);

        initPowerButton();
        ActivityLed::init();

        setupUsb();

        xTaskCreate(watchdog, "watchdog", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 10, nullptr);
        xTaskCreate(statusIndicator, "statusIndicator", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 9, nullptr);

        startWifi();

        // If it has survived this far, the software is probably good
        esp_ota_mark_app_valid_cancel_rollback();

        xTaskCreate(updater, "updater", 8192, nullptr, tskIDLE_PRIORITY, nullptr);
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
            uint32_t now = xTaskGetTickCount();

            if (now - lastSerialMessage > 600_s)
            {
                // Give up
                printf("System inactive");
                ledFreq = 5_s;
            }
            if ((((now - lastSerialMessage) > COMPUTER_USB_READY_DURATION) && !tud_ready()) || ((now - lastSerialMessage) > COMPUTER_BOOT_DURATION))
            {
                // Computer is no longer communicating, restart it
                printf("Missed too many pings, restarting\n");
                ledFreq = 1_s;
                restart();
                ledFreq = 250_ms;
            }

            vTaskDelay(1_s);
        }
    }
}