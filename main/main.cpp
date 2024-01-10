#include <stdio.h>
#include <stdlib.h>
// #include <sys/reent.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tusb_console.h"
#include "vfs_tinyusb.h"
#include "tusb_cdc_acm.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "soc/periph_defs.h"
#include "hal/cpu_hal.h"

#include "lightkvm/print.h"
#include "lightkvm/activityLed.h"
#include "lightkvm/usb.h"

#define POWER_BUTTON_GPIO GPIO_NUM_16

void waitms(TickType_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void initPowerButton()
{
    gpio_reset_pin(POWER_BUTTON_GPIO);
    gpio_set_direction(POWER_BUTTON_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_BUTTON_GPIO, false);
}

void pressPowerButton()
{
    print("Pressing power button\n");

    gpio_set_level(POWER_BUTTON_GPIO, true);
    waitms(5000);
    gpio_set_level(POWER_BUTTON_GPIO, false);

    print("Done pressing power button\n");
}

void pressKey(uint8_t key)
{
    print("Pressing key\n");

    uint8_t keycode[6] = { key };
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    waitms(100);
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, nullptr);
}

extern "C"
{
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

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

    void app_main(void)
    {
        ActivityLed::init();
        initPowerButton();

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
            .string_descriptor = hid_string_descriptor,
            .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
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

        while (1)
        {
            print("Hey\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ActivityLed::blink();

            if (!gpio_get_level(GPIO_NUM_0))
            {
                pressPowerButton();

                waitms(60 * 1000);

                pressKey(HID_KEY_F1);

                waitms(5 * 1000);

                pressKey(HID_KEY_F10);

                waitms(1 * 1000);

                pressKey(HID_KEY_ENTER);

                //waitms(60 * 1000);
            }
        }
    }
}