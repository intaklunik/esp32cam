#pragma once


typedef struct {
        union {
            uint8_t u8;
            uint16_t u16;
            uint8_t buf[2];
        };
        uint8_t length;
} regval_t;

typedef struct {
    uint16_t reg;
    regval_t regval;
} cmd_t;

void app_i2c_handle_rx(const cmd_t * cmd);
void app_i2c_prepare_tx(uint8_t ** data, uint8_t * length);

void app_spi_free_tx();
void app_spi_prepare_tx(uint8_t ** data, uint32_t * length);