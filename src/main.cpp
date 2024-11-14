#include <Arduino.h>
#include <MPU6050.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <ModbusTCPServer.h>
#include <ModbusServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
const char* ssid = "your_ssid";
const char* password = "your_password";
IPAddress serverIP(192, 168, 1, 50); // Modbus client IP
IPAddress local_IP(192, 168, 1, 186);  // Địa chỉ IP tĩnh
IPAddress gateway(192, 168, 1, 1);     // Gateway (thường là router)
IPAddress subnet(255, 255, 255, 0);    // Subnet Mask
IPAddress dns(192, 168, 1, 1);         // DNS server (thường là địa chỉ router)
ModbusTCPServer modbusServer;
QueueHandle_t dataQueue;

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
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Adjust as needed
    }
}

// Task 2: Write data to SD card
void taskWriteSDCard(void *pvParameters) {
    SensorData sensorData;
    while (1) {
        if (xQueueReceive(dataQueue, &sensorData, portMAX_DELAY) == pdTRUE) {
            File file = SD_MMC.open("/data.txt", FILE_APPEND);
            if (file) {
                file.printf("AccelX: %d, AccelY: %d, AccelZ: %d\n",
                            sensorData.accelX, sensorData.accelY, sensorData.accelZ);
                file.close();
            } else {
                Serial.println("Failed to open file for writing.");
            }
        }
    }
}

// Task 3: Send data to Modbus Client and clear SD card when done

void Task3_SendToModbus(void *pvParameters) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected.");

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


void setup() {
    Serial.begin(115200);
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 initialization failed!");
        while (1);
    }

    // Initialize SD card
    if (!SD_MMC.begin()) {
        Serial.println("SD Card Mount Failed");
        return;
    }

    // Start WiFi
  
  // Kết nối Wi-Fi với địa chỉ IP tĩnh
  WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.begin("SSID", "PASSWORD");  // Thay thế "SSID" và "PASSWORD" với thông tin mạng của bạn
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected.");
    modbusServer.begin();

    // Create tasks
    dataQueue = xQueueCreate(10, sizeof(SensorData));
    xTaskCreate(taskReadSensor, "Task1_ReadMPU6050", 2048, NULL, 1, &task1Handle);
    xTaskCreate(taskWriteSDCard, "Task2_WriteToSD", 2048, NULL, 1, &task2Handle);
    xTaskCreate(Task3_SendToModbus, "Task3_SendToModbus", 2048, NULL, 1, &task3Handle);
}

void loop() {
    // FreeRTOS handles the tasks; nothing needed here
}
