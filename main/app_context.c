#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "app_context.h"
#include "regs.h"
#include "bus/esp_i2c_slave.h"
#include "bus/esp_spi_slave.h"
#include "modules/app_config.h"
#include "modules/camera.h"
#include "esp_log.h"

static const char * TAG = "AppContext";

esp_err_t init_app_context()
{
    esp_err_t ret = ESP_OK;

    app_config_init();
    ret = camera_init();
    if (ret) {
        printf("camera_init failed");
    }
    ret = i2c_init_slave();
    if (ret) {
        printf("i2c_init_slave failed");
    }
    
    ret = app_spi_slave_init();
    if (ret) {
        printf("spi_init_slave failed");
    }

    return ret;
}

/*
i2c context
*/

typedef enum {
    MODULE_APP = APP_BASE,
    MODULE_CAMERA = CAMERA_BASE
} module_t;

static regval_t i2c_tx_buffer = {
    .length = 0,
};

static inline void invalidate_i2c_tx_buffer()
{
    ESP_LOGD(TAG, "invalidate_i2c_tx_buffer");
    i2c_tx_buffer.length = 0;
}

static void i2c_app_handle(const cmd_t * cmd)
{
    ESP_LOGV(TAG, "i2c_app_handle: reg %x, length %d", cmd->reg, cmd->regval.length);
    if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case APP_ID_REG:
                i2c_tx_buffer.u8 = app_get_id();
                i2c_tx_buffer.length = 1;
                break;
            case APP_VERSION_REG:
                i2c_tx_buffer.u8 = app_get_version();
                i2c_tx_buffer.length = 1;
                break;
            case CAMERA_STREAM_STATUS_REG:
                i2c_tx_buffer.u8 = app_get_camera_stream_status();
                i2c_tx_buffer.length = 1;
                break;
            default:
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAMERA_STREAM_STATUS_REG:
                    uint8_t status_updated;
                    status_updated = app_set_camera_stream_status(cmd->regval.u8);
                    if (status_updated) {
                        ESP_LOGD(TAG, "camera_stream_status updated");
                        app_spi_update();
                    }
                    break;
            }
        }
    }

}

static void i2c_camera_handle(const cmd_t * cmd)
{
    ESP_LOGV(TAG, "i2c_camera_handle: reg %x, length %d", cmd->reg, cmd->regval.length);
    if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case CAM_BRIGHTNESS_REG:
                i2c_tx_buffer.u8 = camera_get_brightness();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_CONTRAST_REG:
                i2c_tx_buffer.u8 = camera_get_contrast();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_SATURATION_REG:
                i2c_tx_buffer.u8 = camera_get_saturation();
                i2c_tx_buffer.length = 1;
                break;
            default:
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAM_BRIGHTNESS_REG:
                    camera_set_brightness(cmd->regval.u8);
                    break;
                case CAM_CONTRAST_REG:
                    camera_set_contrast(cmd->regval.u8);
                    break;
                case CAM_SATURATION_REG:
                    camera_set_saturation(cmd->regval.u8);
                    break;
            }
        }
    }
}

void app_i2c_handle_rx(const cmd_t * cmd)
{
    module_t module_type = cmd->reg & 0xFF00;

    ESP_LOGD(TAG, "app_i2c_handle_rx, module_type: %x", module_type);

    switch(module_type) {
        case MODULE_APP:
            i2c_app_handle(cmd);
            break;
        case MODULE_CAMERA:
            i2c_camera_handle(cmd);
            break;
        default:
            if (cmd->regval.length > 0) {
                invalidate_i2c_tx_buffer();
            }
    }  
}

void app_i2c_prepare_tx(uint8_t ** data, uint8_t * length)
{
    *data = i2c_tx_buffer.buf;
    *length = i2c_tx_buffer.length;

    ESP_LOGD(TAG, "app_i2c_prepare_tx: length %d", *length);
}

/*
spi context
*/

void app_spi_prepare_tx(uint8_t ** data, uint32_t * length)
{
    *length = camera_get_fb(data);
}

void app_spi_free_tx()
{
    camera_free_fb();
}


