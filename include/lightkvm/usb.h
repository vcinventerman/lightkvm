#ifndef USB_H
#define USB_H

#include <stdint.h>

#include <tinyusb.h>
#include <tusb_cdc_acm.h>
#include <vfs_tinyusb.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

  enum
  {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
  };

  const uint8_t hid_report_descriptor[] = {
      TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD))};

  extern const char *usbDescriptors[6];

  const char *usbStringDescriptors[6] = {
      // array of pointer to string descriptors (these are lowercase to improve case-sensitive compatibility)
      (char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
      "karrmedia.com",      // 1: Manufacturer
      "lightkvm",           // 2: Product
      "000001",             // 3: Serials, should use chip ID
      "lightkvm Serial",    // 4: CDC
      "lightkvm Keyboard",  // 5: HID
  };

  static const uint8_t hid_configuration_descriptor[] = {
      // Configuration number, interface count, string index, total length, attribute, power in mA
      TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, 0, 200),

      // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
      TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x81, 8, 0x02, 0x82, CFG_TUD_CDC_EP_BUFSIZE),

      // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
      TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, false, sizeof(hid_report_descriptor), 0x83, 16, 10),
  };

  // Invoked when received GET HID REPORT DESCRIPTOR request
  // Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
  uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
  {
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
  }

  // Invoked when received GET_REPORT control request
  // Application must fill buffer report's content and return its length.
  // Return zero will cause the stack to STALL request
  uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
  {
    return 0;
  }

  // Invoked when received SET_REPORT control request or
  // received data on OUT endpoint ( Report ID = 0, Type = 0 )
  void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
  {
  }

  void setupUsb()
  {
    printf("USB initialization\n");

    const static tusb_desc_device_t descriptor_config = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,

        // Use Interface Association Descriptor (IAD) for CDC
        // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,

        .bMaxPacketSize0 = 64,
        .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
        .idProduct = 0x4002,
        .bcdDevice = 0x100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01};

    const static tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor_config,
        .string_descriptor = usbStringDescriptors,
        .string_descriptor_count = sizeof(usbStringDescriptors) / sizeof(usbStringDescriptors[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
        .self_powered = false,
        .vbus_monitor_io = 0};

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0};
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    ESP_ERROR_CHECK(esp_vfs_tusb_cdc_register(TINYUSB_CDC_ACM_0, NULL));

    printf("USB initialization DONE\n");
  }

#ifdef __cplusplus
}
#endif

#endif // ifndef USB_H