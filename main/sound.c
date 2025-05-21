#include "sound.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "driver/i2s_std.h"
// #include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "KTM_PLAYER"

#define I2S_NUM 0
#define I2S_DO_IO 14
#define I2S_BCK_IO 12
#define I2S_WS_IO 13
#define BUFFER_SIZE 1024

i2s_chan_handle_t tx_handle = NULL;

QueueHandle_t xQueue; // Khai báo queue

static const char *digit_str[] = {
    "000", "001", "002", "003", "004", "005", "006", "007", "008", "009"};

void read_three_digits(int num, char *out)
{
    int hundred = num / 100;
    int ten = (num / 10) % 10;
    int unit = num % 10;

    char temp[100] = "";

    if (hundred > 0)
    {
        sprintf(temp + strlen(temp), "%stra", digit_str[hundred]);
    }

    if (ten > 1)
    {
        sprintf(temp + strlen(temp), "%smuo", digit_str[ten]);
        if (unit == 1)
            strcat(temp, "mot");
        else if (unit == 5)
            strcat(temp, "lam");
        else if (unit != 0)
        {
            sprintf(temp + strlen(temp), "%s", digit_str[unit]);
        }
    }
    else if (ten == 1)
    {
        strcat(temp, "010");
        if (unit == 5)
            strcat(temp, "lam");
        else if (unit != 0)
        {
            sprintf(temp + strlen(temp), "%s", digit_str[unit]);
        }
    }
    else if (unit != 0)
    {
        sprintf(temp + strlen(temp), "%s", digit_str[unit]);
    }

    strcat(out, temp);
}

void read_number_to_text(uint32_t number, char *out)
{
    if (number == 0)
    {
        strcpy(out, "000don");
        return;
    }

    const char *unit_str[] = {"", "ngh", "tri", "0ty"};
    int parts[4] = {0}; // hỗ trợ đến hàng tỷ
    int part_count = 0;

    // Tách thành các nhóm 3 chữ số
    while (number > 0 && part_count < 4)
    {
        parts[part_count++] = number % 1000;
        number /= 1000;
    }

    out[0] = '\0';
    int first_part = 1; // Biến để kiểm tra phần đầu tiên có cần thêm "không" không
    for (int i = part_count - 1; i >= 0; i--)
    {
        if (parts[i] != 0 || i == 0)
        {
            char segment[128] = "";
            read_three_digits(parts[i], segment);

            // Nếu không phải phần đầu tiên mà số = 0, không thêm "không"
            if (first_part && parts[i] == 0)
            {
                continue;
            }

            strcat(out, segment);
            strcat(out, unit_str[i]);
            first_part = 0;
        }
    }

    strcat(out, "don");
}

void sound_play(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return;
    }

    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_read, bytes_written;

    fseek(f, 44, SEEK_SET); // bỏ header

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, f)) > 0)
    {
        i2s_channel_write(tx_handle, buffer, bytes_read, &bytes_written, portMAX_DELAY);
    }

    fclose(f);
}

void sound_queue(void *)
{
    int32_t received_value;
    while (1)
    {
        if (xQueueReceive(xQueue, &received_value, portMAX_DELAY) == pdPASS)
        {
            switch (received_value)
            {
            case 0:
                sound_play("/spiffs/khoidong.wav");
                break;
            case -1:
                sound_play("/spiffs/daketnoi.wav");
                break;
            case -2:
                sound_play("/spiffs/matketnoi.wav");
                break;
            case -3:
                sound_play("/spiffs/giu5giay.wav");
                break;
            case -4:
                sound_play("/spiffs/khoidonglai.wav");
                break;
            default:
                printf("Received: %lu\n", received_value);
                char result[256];
                read_number_to_text(received_value, result);
                printf("%s\n", result); // Output: "một trăm nghìn đồng"
                sound_play("/spiffs/chuong.wav");
                sound_play("/spiffs/taikhoantang.wav");
                size_t i = 0;
                while (result[i] != '\0')
                {
                    char path[16];
                    snprintf(path, sizeof(path), "/spiffs/%c%c%c.wav", result[i], result[i + 1], result[i + 2]);
                    printf("%s\n", path); // Output: "một trăm nghìn đồng"
                    sound_play(path);
                    i += 3;
                }
                break;
            }
        }
    }
}

void sound_init(void)
{
    // Initialize SPIFFS
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 2,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
    ESP_LOGI(TAG, "SPIFFS mounted");

    // Initialize I2S
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(8000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_GPIO_UNUSED}};

    i2s_new_channel(&(i2s_chan_config_t){
                        .id = I2S_NUM,
                        .role = I2S_ROLE_MASTER,
                        .dma_desc_num = 4,
                        .dma_frame_num = 256,
                    },
                    &tx_handle, NULL);

    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);

    // Tạo queue chứa tối đa 10 phần tử kiểu int
    xQueue = xQueueCreate(10, sizeof(int));
    if (xQueue == NULL)
    {
        printf("Failed to create queue!\n");
        return;
    }

    xTaskCreate(sound_queue, "Queue process", 20480, NULL, 1, NULL);
    int32_t value = 0;
    xQueueSend(xQueue, &value, portMAX_DELAY);
}