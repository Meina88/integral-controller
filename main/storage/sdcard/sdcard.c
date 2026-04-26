#include "sdcard.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"

#define TAG "SDCARD"
#define MOUNT_POINT "/sdcard"

// SPI
#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK 12
#define PIN_NUM_CS 15

static sdmmc_card_t *card;
static bool sd_ok = false;

// =========================
// INIT SD
// =========================
esp_err_t sdcard_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Inicializando SD (SPI + GPIO CS)...");

    // =========================
    // CONFIGURAR CS
    // =========================
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    gpio_set_level(PIN_NUM_CS, 1); // idle HIGH

    // =========================
    // SPI HOST
    // =========================
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 10000;

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
        sd_ok = false;
        return ret;
    }

    // =========================
    // CONFIG SD
    // =========================
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error montando SD: %s", esp_err_to_name(ret));
        sd_ok = false;
        return ret;
    }

    ESP_LOGI(TAG, "SD montada OK");
    sdmmc_card_print_info(stdout, card);

    // 🔥 crear estructura de carpetas
    sdcard_create_dirs();

    sd_ok = true;
    return ESP_OK;
}

// =========================
// ESTADO SD
// =========================
bool sdcard_is_ready(void)
{
    return sd_ok;
}

// =========================
// TEST ESCRITURA / LECTURA
// =========================
void sdcard_test(void)
{
    if (!sd_ok)
    {
        printf("SD no disponible\n");
        return;
    }

    const char *path = "/sdcard/test_write.txt";

    printf("Escribiendo archivo: %s\n", path);

    FILE *f = fopen(path, "w");
    if (!f)
    {
        printf("ERROR: no se pudo abrir archivo para escritura\n");
        return;
    }

    fprintf(f, "Hola desde ESP32-S3\n");
    fprintf(f, "Prueba de escritura OK\n");

    fclose(f);

    printf("Archivo escrito correctamente\n");

    // =========================
    // VERIFICACIÓN
    // =========================
    f = fopen(path, "r");
    if (!f)
    {
        printf("ERROR: no se pudo abrir archivo para lectura\n");
        return;
    }

    char buffer[128];

    printf("Contenido del archivo:\n");

    while (fgets(buffer, sizeof(buffer), f))
    {
        printf("%s", buffer);
    }

    fclose(f);
}

#include <sys/stat.h>

void sdcard_create_dirs(void)
{
    mkdir("/sdcard/logs", 0775);
    mkdir("/sdcard/profiles", 0775);
    mkdir("/sdcard/summary", 0775);
}