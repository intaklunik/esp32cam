#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_camera.h"
#include "modules/camera.h"

#define CAMERA_MODULE_AI_THINKER


#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 5000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_VGA,//FRAMESIZE_QQVGA,

    .jpeg_quality = 12,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

typedef struct {
    sensor_t * sensor;
    camera_fb_t * current_fb;
} camera_context_t;

static camera_context_t context;

uint32_t camera_get_fb(uint8_t ** buffer)
{
    context.current_fb = esp_camera_fb_get();
    if (context.current_fb == NULL) {
        printf("fb s NULL\n");
        return 0;
    }
    *buffer = context.current_fb->buf;

    return context.current_fb->len;
}

void camera_free_fb()
{
    esp_camera_fb_return(context.current_fb);
}

int8_t camera_get_brightness()
{
    printf("brightness: %d", context.sensor->status.brightness);
    return context.sensor->status.brightness;
}

void camera_set_brightness(int8_t value)
{
    printf("brightness_val: %d", value);
    context.sensor->set_brightness(context.sensor, value);
}

int8_t camera_get_contrast()
{
    printf("contrast: %d", context.sensor->status.contrast);
    return context.sensor->status.contrast;
}

void camera_set_contrast(int8_t value)
{
    printf("contrast_val: %d", value);
    context.sensor->set_contrast(context.sensor, value);
}

int8_t camera_get_saturation()
{
    printf("saturation: %d", context.sensor->status.saturation);
    return context.sensor->status.saturation;
}

void camera_set_saturation(int8_t value)
{
    printf("saturation_val: %d", value);
    context.sensor->set_saturation(context.sensor, value);
}

esp_err_t camera_init()
{
    esp_err_t ret = ESP_OK;

    if (CAM_PIN_PWDN != -1) {
        printf("!=-1\n");
        gpio_reset_pin(CAM_PIN_PWDN);
        gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
        gpio_set_level(CAM_PIN_PWDN, 0);
    }

    ret = esp_camera_init(&camera_config);
    if (ret != ESP_OK) {
        return ret;
    }

    context.sensor = esp_camera_sensor_get();
    context.current_fb = NULL;

    return ret;
}
