#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_MAX31865.h>

// Khai báo cảm biến
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
Adafruit_MAX31865 thermo = Adafruit_MAX31865(5, 23, 19, 18);

ModbusIP mb;

// Cấu hình mẫu
#define SAMPLE_RATE 100      // Tốc độ đọc (1000 Hz)
#define SAMPLE_COUNT 100    // Số mẫu đọc mỗi giây
#define THRESHOLD 0.5         // Ngưỡng phát hiện đỉnh (g)
#define ALPHA 0.1             // Hệ số lọc thông thấp

#define RREF      430.0
#define RNOMINAL  100.0

// Mảng lưu giá trị gia tốc
float ax[SAMPLE_COUNT], ay[SAMPLE_COUNT], az[SAMPLE_COUNT];
float offsetX = 0, offsetY = 0, offsetZ = 0; // Biến hiệu chỉnh



// Modbus register addresses for temperature and humidity
const int TEMPERATURE_REGISTER_ADDRESS = 3;
const int FrequencyX_REGISTER_ADDRESS = 0;
const int FrequencyY_REGISTER_ADDRESS = 1;
const int FrequencyZ_REGISTER_ADDRESS = 2;
void setup() {
  Serial.begin(115200);
  WiFi.begin("Quan Dao", "12022022");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mb.server();
  mb.addHreg(TEMPERATURE_REGISTER_ADDRESS, 0);
  mb.addHreg(FrequencyX_REGISTER_ADDRESS, 0);
  mb.addHreg(FrequencyY_REGISTER_ADDRESS, 0);
  mb.addHreg(FrequencyZ_REGISTER_ADDRESS, 0);
  if (!accel.begin()) {
    Serial.println("Không tìm thấy cảm biến ADXL345, kiểm tra kết nối!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);
  thermo.begin(MAX31865_2WIRE);

  // Hiệu chỉnh cảm biến
  calibrateSensor(offsetX, offsetY, offsetZ);
  Serial.println("Adafruit MAX31865 PT100 Sensor Test!");
  Serial.println("Khởi động thành công!");
  
}

void loop() {
  mb.task();

  float temperature = 10*thermo.temperature(RNOMINAL, RREF);
  collectData();
  float freqX = 10*calculateFrequency(ax);
  float freqY = 10*calculateFrequency(ay);
  float freqZ = 10*calculateFrequency(az);

  Serial.print("Temperature = "); Serial.println(temperature);

  Serial.print("Tần số trục X: ");
  Serial.print(freqX, 1);
  Serial.println(" Hz");

  Serial.print("Tần số trục Y: ");
  Serial.print(freqY, 1);
  Serial.println(" Hz");

  Serial.print("Tần số trục Z: ");
  Serial.print(freqZ, 1);
  Serial.println(" Hz");



  if (!isnan(temperature) && !isnan(freqX) && !isnan(freqY) && !isnan(freqZ)) {
    mb.Hreg(TEMPERATURE_REGISTER_ADDRESS, temperature);
    mb.Hreg(FrequencyX_REGISTER_ADDRESS, freqX);
    mb.Hreg(FrequencyY_REGISTER_ADDRESS, freqY);
    mb.Hreg(FrequencyZ_REGISTER_ADDRESS, freqZ);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");
    Serial.print("Frequency X: ");
    Serial.print(freqX);
    Serial.println(" Hz");
    Serial.print("Frequency Y: ");
    Serial.print(freqY);
    Serial.println(" Hz");
    Serial.print("Frequency Z: ");
    Serial.print(freqZ);
    Serial.println(" Hz");
  }

  delay(1000);
}

// Thu thập dữ liệu và áp dụng hiệu chỉnh, lọc
void collectData() {
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sensors_event_t event;
    accel.getEvent(&event);

    // Lấy dữ liệu và trừ offset
    ax[i] = event.acceleration.x - offsetX;
    ay[i] = event.acceleration.y - offsetY;
    az[i] = event.acceleration.z - offsetZ;

    delayMicroseconds(1000000 / SAMPLE_RATE); // Đảm bảo tốc độ mẫu
  }

  // Áp dụng lọc thông thấp
  lowPassFilter(ax, SAMPLE_COUNT, ALPHA);
  lowPassFilter(ay, SAMPLE_COUNT, ALPHA);
  lowPassFilter(az, SAMPLE_COUNT, ALPHA);
}

// Hàm phát hiện tần số từ dữ liệu
float calculateFrequency(float *data) {
  int peakCount = 0;
  int valleyCount = 0;
  bool rising = false;

  for (int i = 0; i < SAMPLE_COUNT - 1; i++) {
    if ((data[i] - data[i - 1]) > THRESHOLD) {
      rising = true;
      i = SAMPLE_COUNT;
    } else if ((data[i - 1] - data[i]) > THRESHOLD && rising) {
      rising = false;
      i = SAMPLE_COUNT;
    }
  }

  for (int i = 1; i < SAMPLE_COUNT; i++) {
    if ((data[i] - data[i - 1]) > THRESHOLD && rising == false) {
      valleyCount++;
      rising = true;
    } else if ((data[i - 1] - data[i]) > THRESHOLD && rising == true) {
      peakCount++;
      rising = false;
    }
  }
    
  return (float)(peakCount + valleyCount) * SAMPLE_RATE / (2 * SAMPLE_COUNT);
}

// Hàm lọc thông thấp
void lowPassFilter(float *data, int length, float alpha) {
  for (int i = 1; i < length; i++) {
    data[i] = alpha * data[i] + (1 - alpha) * data[i - 1];
  }
}

// Hiệu chỉnh cảm biến (tính offset)
void calibrateSensor(float &offsetX, float &offsetY, float &offsetZ) {
  float sumX = 0, sumY = 0, sumZ = 0;
  const int calibrationSamples = 100;

  Serial.println("Đang hiệu chỉnh cảm biến...");
  for (int i = 0; i < calibrationSamples; i++) {
    sensors_event_t event;
    accel.getEvent(&event);

    sumX += event.acceleration.x;
    sumY += event.acceleration.y;
    sumZ += event.acceleration.z;

    delay(10);
  }

  offsetX = sumX / calibrationSamples;
  offsetY = sumY / calibrationSamples;
  offsetZ = sumZ / calibrationSamples;
  Serial.println("Hoàn tất hiệu chỉnh!");
}
