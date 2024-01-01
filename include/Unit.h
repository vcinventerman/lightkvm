#ifndef UNIT_H
#define UNIT_H

#include "freertos/task.h"

TickType_t operator ""_ms(unsigned long long int time) {
    return time / portTICK_PERIOD_MS;
}

#endif // ifndef UNIT_H