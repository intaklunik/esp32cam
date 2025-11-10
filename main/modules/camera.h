#pragma once

#include <esp_err.h>

esp_err_t camera_init(void);

int8_t camera_get_brightness(void);
void camera_set_brightness(int8_t value);
int8_t camera_get_contrast(void);
void camera_set_contrast(int8_t value);
int8_t camera_get_saturation(void);
void camera_set_saturation(int8_t value);

uint32_t camera_get_fb(uint8_t ** buffer);
void camera_free_fb(void);

