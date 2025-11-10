#include <stdint.h>
#include <stdbool.h>
#include "app_context.h"
#include "regs.h"
#include "bus/esp_i2c_slave.h"
#include "bus/esp_spi_slave.h"
#include "modules/app_config.h"
#include "modules/camera.h"
#include <esp_err.h>

static const char * TAG = "app_context";

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
    i2c_tx_buffer.length = 0;
}

static void i2c_app_handle(const cmd_t * cmd)
{
    if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case APP_ID_REG:
                printf("id\n");
                i2c_tx_buffer.u8 = app_get_id();
                i2c_tx_buffer.length = 1;
                break;
            case APP_VERSION_REG:
                printf("version\n");
                i2c_tx_buffer.u8 = app_get_version();
                i2c_tx_buffer.length = 1;
                break;
            case CAMERA_STREAM_STATUS_REG:
                printf("cam_status\n");
                i2c_tx_buffer.u8 = app_get_camera_stream_status();
                i2c_tx_buffer.length = 1;
                break;
            default:
                printf("app_handle invalid\n");
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAMERA_STREAM_STATUS_REG:
                    printf("set cam_status\n");
                    uint8_t status_updated;
                    status_updated = app_set_camera_stream_status(cmd->regval.u8);
                    if (status_updated) {
                        printf("status updated\n");
                        app_spi_update();
                    }
                    break;
            }
        }
    }

}

static void i2c_camera_handle(const cmd_t * cmd)
{
     if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case CAM_BRIGHTNESS_REG:
                printf("get brightness\n");
                i2c_tx_buffer.u8 = camera_get_brightness();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_CONTRAST_REG:
                printf("get contrast\n");
                i2c_tx_buffer.u8 = camera_get_contrast();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_SATURATION_REG:
                printf("get saturation\n");
                i2c_tx_buffer.u8 = camera_get_saturation();
                i2c_tx_buffer.length = 1;
                break;
            default:
                printf("cam_handle invalid\n");
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAM_BRIGHTNESS_REG:
                    printf("set brightness\n");
                    camera_set_brightness(cmd->regval.u8);
                    break;
                case CAM_CONTRAST_REG:
                    printf("set contrast\n");
                    camera_set_contrast(cmd->regval.u8);
                    break;
                case CAM_SATURATION_REG:
                    printf("set saturation\n");
                    camera_set_saturation(cmd->regval.u8);
                    break;
            }
        }
    }
}

void app_i2c_handle_rx(const cmd_t * cmd)
{
    module_t module_type = cmd->reg & 0xFF00;

  //  printf("reg %x, len %u, mt %x\n", cmd->reg, cmd->regval.length, module_type);

    switch(module_type) {
        case MODULE_APP:
          //  printf("mapp\n");
            i2c_app_handle(cmd);
            break;
        case MODULE_CAMERA:
           // printf("mcam\n");
            i2c_camera_handle(cmd);
            break;
        default:
         //   printf("def\n");
            if (cmd->regval.length > 0) {
              //  printf("invalid\n");
                invalidate_i2c_tx_buffer();
            }
    }  
}

void app_i2c_prepare_tx(uint8_t ** data, uint8_t * length)
{
    *data = i2c_tx_buffer.buf;
    *length = i2c_tx_buffer.length;
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


