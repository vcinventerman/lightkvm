#ifndef UNIT_H
#define UNIT_H

#include "freertos/task.h"

TickType_t operator ""_ms(unsigned long long int time) {
    return pdMS_TO_TICKS(time);
}

TickType_t operator ""_s(unsigned long long int time) {
    // seconds * seconds^-1
    return time * CONFIG_FREERTOS_HZ;
}

#endif // ifndef UNIT_H