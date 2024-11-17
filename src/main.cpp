#include <Arduino.h>
#include <MPU6050.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <ModbusTCPServer.h>
#include <ModbusServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include "driver/sdmmc_host.h"
#include "driver/adc.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include <ArduinoRS485.h>
#include <WebServer.h>
const char* ssid = "your_ssid";
const char* password = "your_password";
IPAddress serverIP(192, 168, 1, 50); // Modbus client IP
IPAddress local_IP(192, 168, 1, 186);  // Địa chỉ IP tĩnh
IPAddress gateway(192, 168, 1, 1);     // Gateway (thường là router)
IPAddress subnet(255, 255, 255, 0);    // Subnet Mask
IPAddress dns(192, 168, 1, 1);         // DNS server (thường là địa chỉ router)
ModbusTCPServer modbusServer;
WebServer server(80);
QueueHandle_t dataQueue;
#define TAG "SDMMC_Logger"
#define FILENAME "/data.txt"
// Data storage
MPU6050 mpu;
TaskHandle_t task1Handle, task2Handle, task3Handle;

struct SensorData {
    int16_t accelX, accelY, accelZ;
};

// Task 1: Read data from MPU6050
void taskReadSensor(void *pvParameters) {
    SensorData sensorData;
    while (1) {
        mpu.getAcceleration(&sensorData.accelX, &sensorData.accelY, &sensorData.accelZ);
        xQueueSend(dataQueue, &sensorData, portMAX_DELAY);  // Send data to queue
        vTaskDelay(1 / portTICK_PERIOD_MS);  // Adjust as needed
    }
}
// Task 2: Write data to SD card
void taskWriteSDCard(void *pvParameters) {
    SensorData sensorData;
    // Cấu hình GPIO
    gpio_set_pull_mode((gpio_num_t)15, GPIO_PULLUP_ONLY); // D3
    gpio_set_pull_mode((gpio_num_t)2, GPIO_PULLUP_ONLY);  // D0
    gpio_set_pull_mode((gpio_num_t)4, GPIO_PULLUP_ONLY);  // D1
    gpio_set_pull_mode((gpio_num_t)12, GPIO_PULLUP_ONLY); // D2
    gpio_set_pull_mode((gpio_num_t)11, GPIO_PULLUP_ONLY); // CMD
    gpio_set_pull_mode((gpio_num_t)6, GPIO_PULLUP_ONLY); // CLK
    // Cấu hình SDMMC
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = "/sdcard";
    ESP_LOGI(TAG, "Khởi tạo SDMMC...");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // Đọc từ GPIO5
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Không thể gắn thẻ nhớ: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Đã gắn thẻ nhớ, thông tin:");
    sdmmc_card_print_info(stdout, card);
    while (1) {
        if (xQueueReceive(dataQueue, &sensorData, portMAX_DELAY) == pdTRUE) {
            FILE* file = fopen("/sdcard/data.txt", "a");
            if (file) {
                fprintf(file, "AccelX: %d, AccelY: %d, AccelZ: %d\n",
                        sensorData.accelX, sensorData.accelY, sensorData.accelZ);
                fclose(file);
            } else {
                ESP_LOGE(TAG, "Không thể mở file để ghi.");
            }
        }
    }
    vTaskDelay(0.5/portTICK_PERIOD_MS);
}
// Task 3: Send data to Modbus Client and clear SD card when done

void taskSendToModbus(void *pvParameters) {
   try_to_connect_wifi();
    modbusServer.begin(502);  // Set up Modbus server on port 502
    while (1) {
        File file = SD_MMC.open("/data.txt", FILE_READ);
        if (file) {
            int registerAddress = 0; // Start register address for Modbus
            while (file.available()) {
                String dataLine = file.readStringUntil('\n');
                
                // Parse data assuming format: "AccelX: val, AccelY: val, AccelZ: val"
                int16_t accelX, accelY, accelZ;
                sscanf(dataLine.c_str(), "AccelX: %d, AccelY: %d, AccelZ: %d", &accelX, &accelY, &accelZ);
                
                // Write values to Modbus holding registers
                modbusServer.holdingRegisterWrite(registerAddress, accelX);  // Register for AccelX
                modbusServer.holdingRegisterWrite(registerAddress + 1, accelY);  // Register for AccelY
                modbusServer.holdingRegisterWrite(registerAddress + 2, accelZ);  // Register for AccelZ                
                registerAddress += 3;  // Advance to next set of registers if needed
            }
            file.close();

            // Delete file after data transfer
            SD_MMC.remove("/data.txt");
        } else {
            Serial.println("Failed to open file for reading.");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void handleRoot();
void handleUpdate();
void try_to_connect_wifi(){
    WiFi.begin(ssid, password);
    WiFi.config(local_IP, gateway, subnet, dns);
  Serial.println("Connecting to WiFi...");
    unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 5000) { // Nếu quá 5 giây
      Serial.println("Kết nối thất bại!");
      break;
    }
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Kết nối thành công!");
  }
}
void setup() {
    Serial.begin(115200);
    server.on("/", handleRoot);
    server.on("/update", handleUpdate);
    server.begin();
    Serial.println("Server started!");
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 initialization failed!");
        while (1);
    }
    try_to_connect_wifi();
    // Initialize SD card
    if (!SD_MMC.begin()) {
        Serial.println("SD Card Mount Failed");
        return;
    }
    server.on("/", handleRoot);
    server.on("/update", handleUpdate);
    server.begin();
    Serial.println("Server started!");
    modbusServer.begin();
    // Create tasks
    dataQueue = xQueueCreate(10, sizeof(SensorData));
    xTaskCreate(taskReadSensor, "Task1_ReadMPU6050", 2048, NULL, 1, &task1Handle);
    xTaskCreate(taskWriteSDCard, "Task2_WriteToSD", 2048, NULL, 1, &task2Handle);
    xTaskCreate(taskSendToModbus, "Task3_SendToModbus", 2048, NULL, 1, &task3Handle);
}

void loop() {
     server.handleClient();
}

void handleRoot() {
  String html = "<html><body>"
                "<h1>Cấu hình Wi-Fi</h1>"
                "<form action='/update' method='POST'>"
                "SSID: <input type='text' name='ssid'><br>"
                "Password: <input type='text' name='password'><br>"
                "<input type='submit' value='Cập nhật Wi-Fi'>"
                "</form>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    WiFi.disconnect();
    WiFi.begin(newSSID.c_str(), newPassword.c_str());

    server.send(200, "text/html", "<html><body><h1>Đang kết nối lại...</h1></body></html>");

    Serial.println("Đang kết nối lại với Wi-Fi mới...");
    unsigned long reconnectTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - reconnectTime > 5000) { // Nếu quá 5 giây
        Serial.println("Kết nối thất bại với thông tin mới!");
        return;
      }
      delay(500);
    }

    Serial.println("Kết nối thành công với Wi-Fi mới!");
  } else {
    server.send(400, "text/html", "Thiếu thông tin SSID hoặc mật khẩu!");
  }
}

