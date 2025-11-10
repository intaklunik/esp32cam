#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include "bus/esp_spi_slave.h"
#include <esp_random.h>
#include "app_context.h"

#define RCV_HOST SPI2_HOST
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14

typedef struct {
    TaskHandle_t xHandle;
    bool enabled;
} spi_context_t;

static spi_context_t context;

static void spi_slave_task(void * arg);

esp_err_t app_spi_slave_init()
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_slave_interface_config_t slave_config = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 2,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL
    };

    esp_err_t ret;
    
    gpio_set_pull_mode(bus_config.mosi_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(bus_config.sclk_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(slave_config.spics_io_num, GPIO_PULLUP_ONLY);

    ret = spi_slave_initialize(RCV_HOST, &bus_config, &slave_config, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);
    context.enabled = false;
    app_spi_update();

    return ESP_OK;
}

void app_spi_update()
{
    context.enabled = !context.enabled;

    if (context.enabled) {
        xTaskCreate(spi_slave_task, "spi_slave_task", 4096, &context, 10, &context.xHandle);
        if (!context.xHandle) {
            printf("error xtask spi creat\n");
            context.enabled = !context.enabled;
            return ;
        }
    }

}

static void spi_slave_task(void * arg) 
{
    printf("spi_slave_task start\n");
    esp_err_t ret = 0;
    spi_context_t * context = arg;   
    uint8_t * tx_buffer;
    uint32_t tx_length;
    spi_slave_transaction_t transaction = {
        .flags = 0,
        .rx_buffer = NULL,
    };

    unsigned char a = 'a';
    tx_buffer = &a;
    tx_length = 1;

    while (context->enabled) {
       // app_spi_prepare_tx(&tx_buffer, &tx_length);
        a = esp_random() % 200;
        transaction.length = tx_length * 8;
        transaction.tx_buffer = tx_buffer;
        printf("spi tx a %u\n", a);
        ret = spi_slave_transmit(RCV_HOST, &transaction, portMAX_DELAY);
        printf("after trasmit %d\n", ret);
       // app_spi_free_tx();
    }
    
    vTaskDelete(NULL);
}


