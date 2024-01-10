#ifndef ACTIVITY_LED_H
#define ACTIVITY_LED_H

#include <freertos/task.h>
#include <sdkconfig.h>
#include <FreeRTOSConfig.h>
#include <projdefs.h>

#define BLINK_GPIO GPIO_NUM_15

namespace ActivityLed {
    void init() {
        gpio_reset_pin(BLINK_GPIO);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(BLINK_GPIO, false);
    }

    void blink() {
        gpio_set_level(BLINK_GPIO, true);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(BLINK_GPIO, false);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
};

#endif // ifndef ACTIVITY_LED_H