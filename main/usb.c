#include "lightkvm/Usb.h"



const char* usbDescriptors[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "karrmedia.com",        // 1: Manufacturer
    "LightKVM",             // 2: Product
    "000001",               // 3: Serials, should use chip ID
    "LightKVM Serial",      // 4: CDC
    "LightKVM Keyboard",    // 5: HID
};