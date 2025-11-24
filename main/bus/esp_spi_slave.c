#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include <esp_check.h>
#include <stdatomic.h>
#include "bus/esp_spi_slave.h"
#include "app_context.h"

#define RCV_HOST SPI2_HOST
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14

#define MAX_TRANSFER_SIZE 160 * 120 * 2 * 8

static const char * spi_task_name = "spi_slave_task";
static const char * TAG = "SPI";

typedef struct {
    TaskHandle_t xHandle;
    atomic_bool enabled;
} spi_context_t;

static spi_context_t context;

static void spi_slave_task(void * arg);

static spi_bus_config_t bus_config = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MAX_TRANSFER_SIZE,
};

static spi_slave_interface_config_t slave_config = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 2,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL
};

esp_err_t inline app_spi_slave_init()
{
    return xTaskCreate(spi_slave_task, spi_task_name, 4096, &context, 10, &context.xHandle) ? ESP_OK : ESP_ERR_NO_MEM;
}

void app_spi_disable()
{
    atomic_store(&context.enabled, false);
}

void app_spi_enable()
{
    if (app_status()) {
        atomic_store(&context.enabled, true);
        xTaskNotifyGive(context.xHandle);
    }
}

static void spi_slave_task(void * arg) 
{
    ESP_LOGV(TAG, "%s start", spi_task_name);

    esp_err_t ret = ESP_OK;
    spi_context_t * context = arg;   
    
    gpio_set_pull_mode(bus_config.mosi_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(bus_config.sclk_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(slave_config.spics_io_num, GPIO_PULLUP_ONLY);

    ESP_GOTO_ON_ERROR(spi_slave_initialize(RCV_HOST, &bus_config, &slave_config, SPI_DMA_CH_AUTO), err, TAG, "spi_slave_initialize failed");
    atomic_init(&context->enabled, false);

    uint8_t * tx_buffer;
    uint32_t tx_length;
    spi_slave_transaction_t transaction = {
        .flags = 0,
        .rx_buffer = NULL,
    };

    while (app_status()) {
        ulTaskNotifyTake(0, portMAX_DELAY);
        ESP_LOGD(TAG, "context->enabled %d", context->enabled);
        while (context->enabled) {
            app_spi_prepare_tx(&tx_buffer, &tx_length);
            transaction.length = tx_length * 8;
            transaction.tx_buffer = tx_buffer;
            ESP_ERROR_CHECK_WITHOUT_ABORT(spi_slave_transmit(RCV_HOST, &transaction, portMAX_DELAY));
            app_spi_free_tx();
        }
    }

    ESP_LOGD(TAG, "hwm %lu", uxTaskGetStackHighWaterMark2(NULL));

    spi_slave_free(RCV_HOST);
err:
    app_shutdown();

    vTaskDelete(NULL);
}


