#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_MAX31865.h>

// Khai báo cảm biến ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
Adafruit_MAX31865 thermo = Adafruit_MAX31865(5, 23, 19, 18);

// Cấu hình mẫu
#define SAMPLE_RATE 1000      // Tốc độ đọc (1000 Hz)
#define SAMPLE_COUNT 1000     // Số mẫu đọc mỗi giây
#define THRESHOLD 0.2         // Ngưỡng phát hiện đỉnh (g)
#define ALPHA 0.1             // Hệ số lọc thông thấp

#define RREF      430.0
#define RNOMINAL  100.0

// Mảng lưu giá trị gia tốc
float ax[SAMPLE_COUNT], ay[SAMPLE_COUNT], az[SAMPLE_COUNT];
float offsetX = 0, offsetY = 0, offsetZ = 0; // Biến hiệu chỉnh

void setup() {
  Serial.begin(115200);
  if (!accel.begin()) {
    Serial.println("Không tìm thấy cảm biến ADXL345, kiểm tra kết nối!");
    while (1);
  }

  // Cấu hình dải đo ±16g
  accel.setRange(ADXL345_RANGE_16_G);
  thermo.begin(MAX31865_2WIRE);

  // Hiệu chỉnh cảm biến
  calibrateSensor(offsetX, offsetY, offsetZ);
  Serial.println("Adafruit MAX31865 PT100 Sensor Test!");
  Serial.println("Khởi động thành công!");
}

void loop() {
  uint16_t rtd = thermo.readRTD();

  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  Serial.print("Ratio = "); Serial.println(ratio,8);
  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));
  // Thu thập dữ liệu trong 1 giây
  collectData();

  // Tính tần số cho từng trục
  float freqX = calculateFrequency(ax);
  float freqY = calculateFrequency(ay);
  float freqZ = calculateFrequency(az);

  // In kết quả
  Serial.print("Tần số trục X: ");
  Serial.print(freqX, 1);
  Serial.println(" Hz");

  Serial.print("Tần số trục Y: ");
  Serial.print(freqY, 1);
  Serial.println(" Hz");

  Serial.print("Tần số trục Z: ");
  Serial.print(freqZ, 1);
  Serial.println(" Hz");

  delay(1000); // Chờ 1 giây
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
  bool rising = false;

  for (int i = 1; i < SAMPLE_COUNT; i++) {
    if ((data[i] - data[i - 1]) > THRESHOLD) {
      rising = true;
    } else if ((data[i - 1] - data[i]) > THRESHOLD && rising) {
      peakCount++;
      rising = false;
    }
  }

  // Tính tần số
  return (float)peakCount / 2.0; // Số đỉnh chia đôi để lấy số chu kỳ
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
