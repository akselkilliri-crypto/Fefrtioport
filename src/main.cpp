#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ =================
const int steeringWheelPin = 34;  // Аналоговый вход руля
const int gasButtonPin = 25;      // Кнопка газа (GND – нажатие)

// ================= ЗОНЫ РУЛЯ =================
const int ZONE_CENTER_LOW  = 1400;  // нижняя граница центра
const int ZONE_CENTER_HIGH = 2050;  // верхняя граница центра
// < 1400  – лево
// 1400-2050 – центр
// > 2050  – право

// ====== ОСТАЛЬНОЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

// EMA-фильтр для руля
float emaSteeringValue = 1800.0;  // начальное приближение
const float emaAlpha = 0.1;       // коэффициент сглаживания

// Газ
bool lastGasButtonState = HIGH;

// Для отладочного вывода
unsigned long lastPrint = 0;
const unsigned long printInterval = 200;  // раз в 200 мс

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  // Начальная калибровка фильтра
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(steeringWheelPin);
    delay(5);
  }
  emaSteeringValue = (float)sum / 50.0;

  bleGamepad.begin();
  Serial.println("BLE Gamepad готов");
}

void loop() {
  if (bleGamepad.isConnected()) {

    // --- РУЛЬ ---
    int raw = analogRead(steeringWheelPin);
    emaSteeringValue = (emaAlpha * raw) + ((1 - emaAlpha) * emaSteeringValue);
    int smooth = (int)emaSteeringValue;

    int steering = 0;

    if (smooth < ZONE_CENTER_LOW) {
      // ЛЕВО: чем меньше значение, тем сильнее поворот
      steering = map(smooth, ZONE_CENTER_LOW, 0, 0, -32767);
      steering = constrain(steering, -32767, 0);
    }
    else if (smooth > ZONE_CENTER_HIGH) {
      // ПРАВО: чем больше значение, тем сильнее поворот
      steering = map(smooth, ZONE_CENTER_HIGH, 4095, 0, 32767);
      steering = constrain(steering, 0, 32767);
    }
    else {
      // ЦЕНТР: стик по середине
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

    // --- ОТЛАДКА: вывод раз в 200 мс ---
    if (millis() - lastPrint > printInterval) {
      Serial.print("Потенциометр: ");
      Serial.print(raw);
      Serial.print("\tГаз: ");
      if (gasButtonState == LOW) {
        Serial.println("НАЖАТ");
      } else {
        Serial.println("отпущен");
      }
      lastPrint = millis();
    }

    delay(10);
  }
  else {
    delay(500);
  }
}
