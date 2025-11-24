#pragma once

#include <esp_err.h>

esp_err_t app_spi_slave_init(void);

void app_spi_disable(void);
void app_spi_enable(void);