/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

// DESCRIPTION:
// This example contains minimal code to make ESP32-S2 based device
// recognizable by USB-host devices as a USB Serial Device printing output from
// the application.

#include <stdio.h>
#include <stdlib.h>
#include <sys/reent.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "vfs_tinyusb.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "soc/periph_defs.h"
#include "hal/cpu_hal.h"

#include <string_view>

#include "../include/Uart.h"

static const char *TAG = "example";



extern "C" {

extern volatile bool debuggerIsAttached = false;

void app_main(void)
{   
    gpio_reset_pin((gpio_num_t)15);
    gpio_set_direction((gpio_num_t)15, GPIO_MODE_OUTPUT);

    bool toggle = false;

    while (!debuggerIsAttached) {
        gpio_set_level((gpio_num_t)15, toggle = !toggle);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    
    while (true) {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        gpio_set_level((gpio_num_t)15, toggle = !toggle);
    }

    //initUart();

    /* Setting TinyUSB up */
    ESP_LOGI(TAG, "USB initialization");

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false, // In the most cases you need to use a `false` value
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = { TINYUSB_USBDEV_0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    //esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to usb
    ESP_ERROR_CHECK(esp_vfs_tusb_cdc_register(TINYUSB_CDC_ACM_0, NULL));

    ESP_LOGI(TAG, "USB initialization DONE");

    while (1) {
        print("Hey\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!gpio_get_level(GPIO_NUM_0)) {
            abort();
        }
    }
}
}