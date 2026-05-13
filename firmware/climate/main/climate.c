#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "nvs_flash.h"

#include "driver/i2c_master.h"

// USER SETTINGS

#define WIFI_SSID      "wifi-name"
#define WIFI_PASSWORD  "wifi-pass"

#define API_URL        "http://192.168.1.91:8000/readings"

// I2C SETTINGS

#define I2C_SDA_GPIO              21
#define I2C_SCL_GPIO              22
#define I2C_FREQ_HZ               100000

#define SHT41_ADDR                0x44
#define OLED_ADDR                 0x3C

#define SHT41_MEASURE_HIGH_PREC   0xFD

// OLED SETTINGS

#define OLED_WIDTH                128
#define OLED_HEIGHT               64
#define OLED_PAGES                8
#define OLED_BUFFER_SIZE          1024

// TIMING

#define SENSOR_UPDATE_MS         1000
//  150000   

#define WIFI_CONNECTED_BIT        BIT0

static const char *TAG = "CLIMATE_MONITOR";

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t sht41_handle;
static i2c_master_dev_handle_t oled_handle;

static EventGroupHandle_t wifi_event_group;

static uint8_t oled_buffer[OLED_BUFFER_SIZE];

// SMALL FONT

static const uint8_t FONT_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t FONT_0[5]     = {0x3E, 0x51, 0x49, 0x45, 0x3E};
static const uint8_t FONT_1[5]     = {0x00, 0x42, 0x7F, 0x40, 0x00};
static const uint8_t FONT_2[5]     = {0x42, 0x61, 0x51, 0x49, 0x46};
static const uint8_t FONT_3[5]     = {0x21, 0x41, 0x45, 0x4B, 0x31};
static const uint8_t FONT_4[5]     = {0x18, 0x14, 0x12, 0x7F, 0x10};
static const uint8_t FONT_5[5]     = {0x27, 0x45, 0x45, 0x45, 0x39};
static const uint8_t FONT_6[5]     = {0x3C, 0x4A, 0x49, 0x49, 0x30};
static const uint8_t FONT_7[5]     = {0x01, 0x71, 0x09, 0x05, 0x03};
static const uint8_t FONT_8[5]     = {0x36, 0x49, 0x49, 0x49, 0x36};
static const uint8_t FONT_9[5]     = {0x06, 0x49, 0x49, 0x29, 0x1E};

static const uint8_t FONT_A[5]     = {0x7E, 0x11, 0x11, 0x11, 0x7E};
static const uint8_t FONT_C[5]     = {0x3E, 0x41, 0x41, 0x41, 0x22};
static const uint8_t FONT_D[5]     = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t FONT_E[5]     = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t FONT_F[5]     = {0x7F, 0x09, 0x09, 0x09, 0x01};
static const uint8_t FONT_H[5]     = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t FONT_I[5]     = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t FONT_L[5]     = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t FONT_M[5]     = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
static const uint8_t FONT_O[5]     = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t FONT_P[5]     = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t FONT_R[5]     = {0x7F, 0x09, 0x19, 0x29, 0x46};
static const uint8_t FONT_S[5]     = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t FONT_T[5]     = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t FONT_U[5]     = {0x3F, 0x40, 0x40, 0x40, 0x3F};
static const uint8_t FONT_W[5]     = {0x7F, 0x20, 0x18, 0x20, 0x7F};

static const uint8_t FONT_DOT[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t FONT_COLON[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t FONT_MINUS[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t FONT_PERCENT[5] = {0x23, 0x13, 0x08, 0x64, 0x62};

static const uint8_t *get_font(char c)
{
    switch (c) {
        case '0': return FONT_0;
        case '1': return FONT_1;
        case '2': return FONT_2;
        case '3': return FONT_3;
        case '4': return FONT_4;
        case '5': return FONT_5;
        case '6': return FONT_6;
        case '7': return FONT_7;
        case '8': return FONT_8;
        case '9': return FONT_9;

        case 'A': return FONT_A;
        case 'C': return FONT_C;
        case 'D': return FONT_D;
        case 'E': return FONT_E;
        case 'F': return FONT_F;
        case 'H': return FONT_H;
        case 'I': return FONT_I;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'O': return FONT_O;
        case 'P': return FONT_P;
        case 'R': return FONT_R;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case 'U': return FONT_U;
        case 'W': return FONT_W;

        case '.': return FONT_DOT;
        case ':': return FONT_COLON;
        case '-': return FONT_MINUS;
        case '%': return FONT_PERCENT;
        case ' ': return FONT_SPACE;

        default: return FONT_SPACE;
    }
}

// I2C SETUP

static esp_err_t i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_io_num = I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    i2c_device_config_t sht41_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT41_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &sht41_config, &sht41_handle));

    i2c_device_config_t oled_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &oled_config, &oled_handle));

    ESP_LOGI(TAG, "I2C initialized");

    return ESP_OK;
}

// OLED FUNCTIONS

static esp_err_t oled_send_command(uint8_t command)
{
    uint8_t data[2] = {0x00, command};
    return i2c_master_transmit(oled_handle, data, sizeof(data), 100);
}

static esp_err_t oled_send_data(uint8_t *data, size_t length)
{
    uint8_t buffer[129];

    if (length > 128) {
        return ESP_ERR_INVALID_SIZE;
    }

    buffer[0] = 0x40;
    memcpy(&buffer[1], data, length);

    return i2c_master_transmit(oled_handle, buffer, length + 1, 100);
}

static void oled_clear_buffer(void)
{
    memset(oled_buffer, 0x00, OLED_BUFFER_SIZE);
}

static void oled_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }

    int byte_index = x + (y / 8) * OLED_WIDTH;
    uint8_t bit_mask = 1 << (y % 8);

    if (color) {
        oled_buffer[byte_index] |= bit_mask;
    } else {
        oled_buffer[byte_index] &= ~bit_mask;
    }
}

static void oled_draw_char(int x, int y, char c)
{
    const uint8_t *font = get_font(c);

    for (int col = 0; col < 5; col++) {
        uint8_t column_data = font[col];

        for (int row = 0; row < 7; row++) {
            bool pixel_on = column_data & (1 << row);
            oled_draw_pixel(x + col, y + row, pixel_on);
        }
    }
}

static void oled_draw_text(int x, int y, const char *text)
{
    int cursor_x = x;

    while (*text) {
        oled_draw_char(cursor_x, y, *text);
        cursor_x += 6;
        text++;
    }
}

static void oled_update_screen(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_send_command(0xB0 + page);
        oled_send_command(0x00);
        oled_send_command(0x10);

        oled_send_data(&oled_buffer[OLED_WIDTH * page], OLED_WIDTH);
    }
}

static void oled_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    oled_send_command(0xAE); // Display off
    oled_send_command(0x20); // Memory addressing mode
    oled_send_command(0x00); // Horizontal addressing mode
    oled_send_command(0xB0); // Page start address
    oled_send_command(0xC8); // COM output scan direction
    oled_send_command(0x00); // Low column
    oled_send_command(0x10); // High column
    oled_send_command(0x40); // Start line
    oled_send_command(0x81); // Contrast
    oled_send_command(0x7F);
    oled_send_command(0xA1); // Segment remap
    oled_send_command(0xA6); // Normal display
    oled_send_command(0xA8); // Multiplex ratio
    oled_send_command(0x3F);
    oled_send_command(0xA4); // Display follows RAM
    oled_send_command(0xD3); // Display offset
    oled_send_command(0x00);
    oled_send_command(0xD5); // Clock divide
    oled_send_command(0x80);
    oled_send_command(0xD9); // Pre-charge
    oled_send_command(0xF1);
    oled_send_command(0xDA); // COM pins
    oled_send_command(0x12);
    oled_send_command(0xDB); // VCOM detect
    oled_send_command(0x40);
    oled_send_command(0x8D); // Charge pump
    oled_send_command(0x14);
    oled_send_command(0xAF); // Display on

    oled_clear_buffer();
    oled_update_screen();

    ESP_LOGI(TAG, "OLED initialized");
}

static void oled_show_status(const char *line1, const char *line2, const char *line3)
{
    oled_clear_buffer();

    oled_draw_text(0, 0,  "CLIMATE");
    oled_draw_text(0, 16, line1);
    oled_draw_text(0, 32, line2);
    oled_draw_text(0, 48, line3);

    oled_update_screen();
}

static void oled_show_reading(float temp_c, float humidity, const char *status)
{
    char temp_line[32];
    char hum_line[32];

    snprintf(temp_line, sizeof(temp_line), "Temp: %.1f C", temp_c);
    snprintf(hum_line, sizeof(hum_line),  "Hum: %.1f %%", humidity);

    oled_clear_buffer();

    oled_draw_text(0, 0,  "CLIMATE");
    oled_draw_text(0, 18, temp_line);
    oled_draw_text(0, 34, hum_line);
    oled_draw_text(0, 52, status);

    oled_update_screen();
}

// SHT41 FUNCTIONS

static esp_err_t sht41_read(float *temperature_c, float *humidity_percent)
{
    uint8_t command = SHT41_MEASURE_HIGH_PREC;
    uint8_t data[6] = {0};

    esp_err_t err = i2c_master_transmit(sht41_handle, &command, 1, 100);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send SHT41 command: %s", esp_err_to_name(err));
        return err;
    }

    // High precision measurement takes about 8-10 ms.
    vTaskDelay(pdMS_TO_TICKS(20));

    err = i2c_master_receive(sht41_handle, data, sizeof(data), 100);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SHT41 data: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t raw_temp = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_hum  = ((uint16_t)data[3] << 8) | data[4];

    float temp = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
    float hum  = -6.0f + 125.0f * ((float)raw_hum / 65535.0f);

    if (hum > 100.0f) {
        hum = 100.0f;
    }

    if (hum < 0.0f) {
        hum = 0.0f;
    }

    *temperature_c = temp;
    *humidity_percent = hum;

    return ESP_OK;
}

// WIFI FUNCTIONS

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected. Reconnecting...");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
}

static bool wifi_wait_connected(void)
{
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(15000)
    );

    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// ===============================
// HTTP FUNCTIONS
// ===============================

static esp_err_t post_reading_to_api(float temperature_c, float humidity_percent)
{
    char json_body[128];

    snprintf(
        json_body,
        sizeof(json_body),
        "{\"temperature\":%.2f,\"humidity\":%.2f}",
        temperature_c,
        humidity_percent
    );

    esp_http_client_config_t config = {
        .url = API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    ESP_LOGI(TAG, "Posting JSON: %s", json_body);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST status: %d", status_code);

        if (status_code >= 200 && status_code < 300) {
            esp_http_client_cleanup(client);
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "API returned non-success status code");
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
}

// APP MAIN

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 Climate Monitor");

    esp_err_t nvs_result = nvs_flash_init();

    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_result);

    ESP_ERROR_CHECK(i2c_init());

    oled_init();
    oled_show_status("BOOTING", "WIFI", "WAIT");

    wifi_init();

    if (wifi_wait_connected()) {
        ESP_LOGI(TAG, "Wi-Fi connected");
        oled_show_status("WIFI OK", "API WAIT", "");
    } else {
        ESP_LOGW(TAG, "Wi-Fi connection timeout");
        oled_show_status("WIFI FAIL", "CHECK INFO", "");
    }

    while (1) {
        float temperature_c = 0.0f;
        float humidity_percent = 0.0f;

        esp_err_t sensor_result = sht41_read(&temperature_c, &humidity_percent);

        if (sensor_result == ESP_OK) {
            ESP_LOGI(
                TAG,
                "Temperature: %.2f C, Humidity: %.2f %%",
                temperature_c,
                humidity_percent
            );

            oled_show_reading(temperature_c, humidity_percent, "POSTING");

            bool wifi_connected = wifi_wait_connected();

            if (wifi_connected) {
                esp_err_t post_result = post_reading_to_api(temperature_c, humidity_percent);

                if (post_result == ESP_OK) {
                    oled_show_reading(temperature_c, humidity_percent, "POST OK");
                } else {
                    oled_show_reading(temperature_c, humidity_percent, "POST FAIL");
                }
            } else {
                ESP_LOGW(TAG, "Skipping POST because Wi-Fi is not connected");
                oled_show_reading(temperature_c, humidity_percent, "WIFI FAIL");
            }
        } else {
            ESP_LOGE(TAG, "Sensor read failed");
            oled_show_status("SENSOR", "READ FAIL", "");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_UPDATE_MS));
    }
}