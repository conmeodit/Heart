#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Kích thước màn hình OLED SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Định nghĩa chân I2C cho ESP32
#define SDA_PIN 21  // Chân SDA của I2C
#define SCL_PIN 22  // Chân SCL của I2C

// Định nghĩa chân reset (nếu có) cho OLED; ESP32 thường dùng -1 nếu không có
#define OLED_RESET -1

// Khai báo đối tượng display (giao tiếp I2C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Hàm khởi tạo I2C với chân SDA, SCL tùy chỉnh
void initI2C() {
  Wire.begin(SDA_PIN, SCL_PIN);
}

// Hàm fillPolygon sử dụng thuật toán scan-line
// x, y: mảng tọa độ các đỉnh; n: số đỉnh; color: màu tô
void fillPolygon(int *x, int *y, int n, uint16_t color) {
  int i, j, minY, maxY;
  // Tìm giới hạn theo trục Y
  minY = y[0];
  maxY = y[0];
  for (i = 1; i < n; i++) {
    if (y[i] < minY) minY = y[i];
    if (y[i] > maxY) maxY = y[i];
  }
  
  // Quét theo từng dòng (scan-line)
  for (int scanline = minY; scanline <= maxY; scanline++) {
    int nodes = 0;
    int nodeX[n]; // Mảng chứa giao điểm với các cạnh
    // Tìm giao điểm của đường ngang với các cạnh của đa giác
    for (i = 0, j = n - 1; i < n; j = i++) {
      if ((y[i] < scanline && y[j] >= scanline) ||
          (y[j] < scanline && y[i] >= scanline)) {
        nodeX[nodes++] = x[i] + (scanline - y[i]) * (x[j] - x[i]) / (y[j] - y[i]);
      }
    }
    
    // Sắp xếp các giao điểm theo thứ tự tăng dần
    for (i = 0; i < nodes - 1; i++) {
      for (j = i + 1; j < nodes; j++) {
        if (nodeX[i] > nodeX[j]) {
          int temp = nodeX[i];
          nodeX[i] = nodeX[j];
          nodeX[j] = temp;
        }
      }
    }
    
    // Vẽ các đường ngang giữa các cặp giao điểm
    for (i = 0; i < nodes; i += 2) {
      if (i + 1 < nodes) {
        display.drawFastHLine(nodeX[i], scanline, nodeX[i + 1] - nodeX[i] + 1, color);
      }
    }
  }
}

// Hàm vẽ trái tim được lấp đầy, sử dụng phương trình tham số
void fillHeart(float scale) {
  // Tâm trái tim (có thể điều chỉnh theo ý muốn)
  int cx = 96;
  int cy = 35;
  
  const int numPoints = 100; // Số điểm để vẽ biên trái tim
  int xPoints[numPoints + 1];
  int yPoints[numPoints + 1];
  
  // Tính toán tọa độ các đỉnh của trái tim theo phương trình
  for (int i = 0; i <= numPoints; i++) {
    float t = -PI + (2 * PI * i / numPoints);
    // Phương trình trái tim
    float x = 16 * pow(sin(t), 3);
    float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);
    
    // Điều chỉnh tỉ lệ và chuyển sang tọa độ màn hình
    int xPos = cx + (int)(x * scale);
    int yPos = cy - (int)(y * scale);
    
    xPoints[i] = xPos;
    yPoints[i] = yPos;
  }
  
  // Tô màu bên trong trái tim
  fillPolygon(xPoints, yPoints, numPoints + 1, SSD1306_WHITE);
  
  // (Tùy chọn) Vẽ viền trái tim để nổi bật đường biên
  for (int i = 0; i < numPoints; i++) {
    display.drawLine(xPoints[i], yPoints[i], xPoints[i + 1], yPoints[i + 1], SSD1306_BLACK);
  }
}

void fillHeartAt(int cx, int cy, float scale) {
  const int numPoints = 100;
  int xPoints[numPoints + 1];
  int yPoints[numPoints + 1];
  
  for (int i = 0; i <= numPoints; i++) {
    float t = -PI + (2 * PI * i / numPoints);
    float x = 16 * pow(sin(t), 3);
    float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);
    
    int xPos = cx + (int)(x * scale);
    int yPos = cy - (int)(y * scale);
    
    xPoints[i] = xPos;
    yPoints[i] = yPos;
  }
  
  fillPolygon(xPoints, yPoints, numPoints + 1, SSD1306_WHITE);
  for (int i = 0; i < numPoints; i++) {
    display.drawLine(xPoints[i], yPoints[i], xPoints[i + 1], yPoints[i + 1], SSD1306_BLACK);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo I2C với chân SDA và SCL tùy chỉnh
  initI2C();
  
  // Khởi tạo màn hình OLED với địa chỉ I2C 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Không thể khởi tạo SSD1306"));
    for (;;); // Dừng lại nếu khởi tạo không thành công
  }
  display.clearDisplay();
}

void loop() {
  // Sử dụng biến tĩnh để tạo hiệu ứng "đập" của trái tim
  static unsigned long lastTime = 0;
  static float scale = 1.0;     // tỉ lệ của trái tim ban đầu
  static bool expanding = true; // trạng thái: đang mở rộng hay thu nhỏ

  static float smallScale = 0.3;    // tỉ lệ ban đầu cho trái tim nhỏ
  static bool smallExpanding = true;
  
  unsigned long currentTime = millis();
  if (currentTime - lastTime > 50) { // cập nhật mỗi 50ms
    lastTime = currentTime;
    
    // Cập nhật tỉ lệ cho hiệu ứng đập
    if (expanding) {
      scale += 0.05;
      if (scale > 1.7) {  // giới hạn mở rộng
        expanding = false;
      }
    } else {
      scale -= 0.05;
      if (scale < 1.4) {  // giới hạn thu nhỏ
        expanding = true;
      }
    }

     if (smallExpanding) {
      smallScale += 0.02;
      if (smallScale > 0.61) {
        smallExpanding = false;
      }
    } else {
      smallScale -= 0.02;
      if (smallScale < 0.49) {
        smallExpanding = true;
      }
    }
    
    
    // Xóa màn hình, vẽ trái tim lấp đầy và cập nhật hiển thị
    display.clearDisplay();
    fillHeart(scale);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setTextSize(1);
    // display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0, 2);
    display.print("Happy Valentine's Day");
    display.setCursor(5, 22);
    display.print("I love you");
    display.setTextSize(1);
    display.setCursor(5, 32);
    display.print("SO MUCH!!!");
    fillHeartAt(10, 55, smallScale);   // tái tim nhỏ bên trái
    fillHeartAt(40, 55, smallScale);   // trái tim nhỏ ở giữa
    fillHeartAt(70, 55, smallScale);   // trái tim nhỏ bên phải
    display.display();
  }
}
