#ifndef PRINT_H
#define PRINT_H

#include "driver/uart.h"

void initUart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 2048, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(0, 1, 2, -1, -1));
}

void print(std::string_view str) {
    printf(str.data());

    // Print to UART0 when it isn't the default channel
    //uart_write_bytes(0, str.data(), str.size());

    // Print to USB when it isn't the default channel
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (const uint8_t*)str.data(), str.size());
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}

#endif // ifndef PRINT_H