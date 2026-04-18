#include "sdcard.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "driver/i2c.h"

#include "DEV_Config.h"
#include "CH422G.h"

#define TAG "SDCARD"
#define MOUNT_POINT "/sdcard"

// SPI según hardware real
#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12

static sdmmc_card_t *card;

// =========================
// INIT SD
// =========================
esp_err_t sdcard_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Inicializando SD...");

    // =========================
    // SPI HOST
    // =========================
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 5000;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "SPI init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // =========================
    // CH422G → configurar IO
    // =========================

    uint8_t mode = CH422G_Mode_IO_OE;
    i2c_master_write_to_device(I2C_MASTER_NUM, CH422G_Mode, &mode, 1, pdMS_TO_TICKS(1000));

    uint8_t state = 0x1E;
    state &= ~CH422G_IO_4;   // CS LOW

    i2c_master_write_to_device(I2C_MASTER_NUM, CH422G_IO_OUT, &state, 1, pdMS_TO_TICKS(1000));

    vTaskDelay(pdMS_TO_TICKS(20));

    // =========================
    // MOUNT
    // =========================
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = -1;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error montando SD: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD montada OK");
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}