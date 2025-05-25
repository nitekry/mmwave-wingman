#include <M5AtomS3.h>

HardwareSerial RD03Serial(1);

const int RX_PIN = 2; 
const int TX_PIN = 1;
const int BAUD_RATE = 256000;
const int BUZZER_PIN = 7;  // External buzzer on pin 7 (active-low)

uint8_t RX_BUF[64];
int bufIndex = 0;
unsigned long readingCount = 0;

uint16_t lastDistance = 0;
int16_t lastX = 0, lastY = 0;
int16_t lastSpeed = 0;
uint16_t lastRes = 0;
int repeatCount = 0;
int maxRepeatThreshold = 3;
unsigned long lastBuzzTime = 0;
const unsigned long buzzInterval = 3000;
bool movementDetected = false;

void setup() {
  M5.begin();
  Serial.begin(115200);
  RD03Serial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);  // Active 5v Buzzer needs to be set silent from start.
  delay(100);
  sendSingleTargetCommand();
}

void sendSingleTargetCommand() {
  uint8_t cmd[] = {0xAA, 0xFF, 0x01, 0x00, 0x00, 0x55, 0xCC};
  RD03Serial.write(cmd, sizeof(cmd));
}

void buzzAlert() {
  static bool buzzState = false;
  unsigned long now = millis();

  if (movementDetected) {
    if (!buzzState && now - lastBuzzTime >= buzzInterval) {
      digitalWrite(BUZZER_PIN, LOW);   // Active-low: buzz ON
      delay(50);                        // Short beep
      digitalWrite(BUZZER_PIN, HIGH);  // Buzz OFF
      lastBuzzTime = now;
      buzzState = true;
    }
  } else {
    buzzState = false;
    digitalWrite(BUZZER_PIN, HIGH);  // Ensure buzzer is OFF
  }
}

void loop() {
  movementDetected = false;

  while (RD03Serial.available()) {
    uint8_t c = RD03Serial.read();
    RX_BUF[bufIndex++] = c;

    if (bufIndex >= 2 && RX_BUF[bufIndex - 2] == 0x55 && RX_BUF[bufIndex - 1] == 0xCC) {
      if (bufIndex >= 30) {
        readingCount++;

        if (RX_BUF[0] == 0xAA && RX_BUF[1] == 0xFF && RX_BUF[2] == 0x03 && RX_BUF[3] == 0x00) {
          int16_t x = (RX_BUF[4] | (RX_BUF[5] << 8)) - 0x200;
          int16_t y = (RX_BUF[6] | (RX_BUF[7] << 8)) - 0x8000;
          int16_t speed = (RX_BUF[8] | (RX_BUF[9] << 8)) - 0x10;
          uint16_t distance_res = (RX_BUF[10] | (RX_BUF[11] << 8));
          float distanceMeters = sqrt((float)(x * x + y * y)) / 1000.0;

          bool isRepeated = (x == lastX && y == lastY && speed == lastSpeed && distance_res == lastRes);
          if (isRepeated) {
            repeatCount++;
            if (repeatCount > 20) maxRepeatThreshold = min(maxRepeatThreshold + 1, 10);
          } else {
            repeatCount = 0;
            maxRepeatThreshold = 3;
            lastX = x;
            lastY = y;
            lastSpeed = speed;
            lastRes = distance_res;
          }

          bool invalidY = (y == -32768); //Y data not required for my purposes of detection around my area.
          bool invalidSpeed = (speed == -16);

          if (repeatCount < maxRepeatThreshold && !invalidY && !invalidSpeed && distanceMeters <= 12.0) {
            Serial.print("Raw Frame: ");
            for (int i = 0; i < 30; i++) {
              Serial.print("0x");
              if (RX_BUF[i] < 0x10) Serial.print("0");
              Serial.print(RX_BUF[i], HEX);
              Serial.print(" ");
            }
            Serial.println();

            Serial.print("Distance: ");
            Serial.print(distanceMeters);
            Serial.println(" m");

            float angle = atan2((float)x, (float)y) * 180.0 / PI;
            Serial.print("Angle from X: ");
            Serial.print(angle);
            Serial.println(" degrees");

            movementDetected = true;
          }
        }
      }
      bufIndex = 0;
    }

    if (bufIndex >= sizeof(RX_BUF)) {
      bufIndex = 0;
    }
  }

  buzzAlert();

  static unsigned long lastCmdTime = 0;
  if (millis() - lastCmdTime > 5000) {
    sendSingleTargetCommand();
    lastCmdTime = millis();
  }
}
