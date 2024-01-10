#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "freertos/task.h"

extern "C" {

extern volatile bool debuggerIsAttached;

inline bool isDebuggerAttached() {
    return debuggerIsAttached;
}

inline void waitForDebuggerToAttach() {
    while (!debuggerIsAttached) {
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

};

#endif // ifndef DEBUGGER_H