#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ =================
const int steeringWheelPin = 34;  // Аналоговый вход руля
const int gasButtonPin = 25;      // Кнопка газа (подключена к GND)

// ★★★ ИЗМЕРЬТЕ И ВСТАВЬТЕ СЮДА СВОИ ЗНАЧЕНИЯ ★★★
const int MIN_RAW = 0;   // Крайний левый упор
const int MAX_RAW = 4095;  // Крайний правый упор

// Ширина мёртвой зоны вокруг центра (чем больше, тем меньше дрожи)
const int DEAD_ZONE = 2026;   // Подберите от 80 до 250

// ====== ОСТАЛЬНОЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

int centerRaw;       // запомненный центр
int leftLimit, rightLimit;

// Газ
bool lastGasButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  // --- Калибровка центра (не трогайте руль 1-2 секунды после включения) ---
  delay(500);
  long sum = 0;
  const int samples = 50;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(steeringWheelPin);
    delay(10);
  }
  centerRaw = sum / samples;

  leftLimit  = centerRaw - DEAD_ZONE;
  rightLimit = centerRaw + DEAD_ZONE;

  Serial.print("Центр: "); Serial.println(centerRaw);
  Serial.print("Левая зона: "); Serial.print(MIN_RAW); Serial.print(" ... "); Serial.println(leftLimit);
  Serial.print("Правая зона: "); Serial.print(rightLimit); Serial.print(" ... "); Serial.println(MAX_RAW);

  bleGamepad.begin();
  Serial.println("BLE Gamepad готов");
}

void loop() {
  if (bleGamepad.isConnected()) {

    // --- РУЛЬ ---
    int raw = analogRead(steeringWheelPin);
    int steering = 0;

    if (raw < leftLimit) {
      // ЛЕВО: шкала от leftLimit до MIN_RAW -> -32767..0
      steering = map(raw, leftLimit, MIN_RAW, 0, -32767);
      steering = constrain(steering, -32767, 0);
    }
    else if (raw > rightLimit) {
      // ПРАВО: шкала от rightLimit до MAX_RAW -> 0..32767
      steering = map(raw, rightLimit, MAX_RAW, 0, 32767);
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

    // Отладка (раскомментируйте, если нужно)
    // Serial.print("RAW: "); Serial.print(raw); Serial.print(" -> "); Serial.println(steering);

    delay(10);
  }
  else {
    delay(500);
  }
}
