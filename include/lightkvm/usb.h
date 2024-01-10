#ifndef USB_H
#define USB_H

#include <stdint.h>

#include <tinyusb.h>
#include <tusb_cdc_acm.h>

#ifdef __cplusplus
extern "C"
{
#endif

  const uint8_t hid_report_descriptor[] = {
      TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD))};

  extern const char *usbDescriptors[6];

  const char *hid_string_descriptor[6] = {
      // array of pointer to string descriptors
      (char[]){0x09, 0x04},    // 0: is supported language is English (0x0409)
      "TinyUSB",               // 1: Manufacturer
      "TinyUSB Device",        // 2: Product
      "123456",                // 3: Serials, should use chip ID
      "Example CDC interface", // 4: CDC
      "Example HID interface", // 5: HID
  };

  enum
  {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
  };

#ifdef __cplusplus
}
#endif

#endif // ifndef USB_H