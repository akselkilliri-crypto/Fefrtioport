#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ =================
const int steeringWheelPin = 34;  // Аналоговый вход руля
const int gasButtonPin = 25;      // Кнопка газа

// ================= ЗОНЫ РУЛЯ =================
const int ZONE_CENTER_LOW  = 1400;  // нижняя граница центра
const int ZONE_CENTER_HIGH = 2050;  // верхняя граница центра
// < 1400  – лево
// 1400-2050 – центр
// > 2050  – право

// ====== ОСТАЛЬНОЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

// Газ
bool lastGasButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  bleGamepad.begin();
  Serial.println("BLE Gamepad готов (без фильтра)");
}

void loop() {
  if (bleGamepad.isConnected()) {

    // --- РУЛЬ (без фильтра) ---
    int raw = analogRead(steeringWheelPin);

    int steering = 0;

    if (raw < ZONE_CENTER_LOW) {
      // ЛЕВО: чем меньше значение, тем сильнее поворот
      steering = map(raw, 0, ZONE_CENTER_LOW, 0, 32767);
      steering = constrain(steering, 0, 32767);
      steering = -steering;  // лево – отрицательные значения оси X
    }
    else if (raw > ZONE_CENTER_HIGH) {
      // ПРАВО: чем больше значение, тем сильнее поворот
      steering = map(raw, ZONE_CENTER_HIGH, 4095, 0, 32767);
      steering = constrain(steering, 0, 32767);
    }
    else {
      // ЦЕНТР
      steering = 0;
    }

    bleGamepad.setX(steering);

    // --- ГАЗ (R2) ---
    bool gasButtonState = digitalRead(gasButtonPin);

    if (gasButtonState == LOW && lastGasButtonState == HIGH) {
      bleGamepad.press(BUTTON_8);
    }
    else if (gasButtonState == HIGH && lastGasButtonState == LOW) {
      bleGamepad.release(BUTTON_8);
    }

    lastGasButtonState = gasButtonState;

    delay(10);
  }
  else {
    delay(500);
  }
}
