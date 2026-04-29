#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ (меняйте при необходимости) =================
const int steeringWheelPin = 34;  // Аналоговый вход руля (подстроечный резистор 10 кОм)
const int gasButtonPin = 25;      // Кнопка газа (подключена к GND, используется внутренняя подтяжка)
// Правый триггер (R2) управляется кнопкой газа: при нажатии отправляется максимальное значение

// Опциональный светодиод на GPIO 2 (раскомментируйте строку ниже, если подключили)
// #define ENABLE_LED
#ifdef ENABLE_LED
const int ledPin = 2;
#endif

// ====== ДАЛЕЕ КОД ЛУЧШЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

// Сглаживание руля
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int average = 0;

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  #ifdef ENABLE_LED
  pinMode(ledPin, OUTPUT);
  #endif

  // Инициализация массива усреднения
  for (int i = 0; i < numReadings; i++) readings[i] = 0;

  bleGamepad.begin();

  #ifdef ENABLE_LED
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  #endif

  Serial.println("BLE Gamepad готов к подключению");
}

void loop() {
  if (bleGamepad.isConnected()) {
    #ifdef ENABLE_LED
    digitalWrite(ledPin, HIGH);
    #endif

    // --- Руль (ось X) ---
    int raw = analogRead(steeringWheelPin);
    // Бегущее среднее
    total = total - readings[readIndex];
    readings[readIndex] = raw;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    average = total / numReadings;

    int steering = map(average, 0, 4095, -32767, 32767);
    bleGamepad.setX(steering);

    // --- Правый триггер (газ) ---
    if (digitalRead(gasButtonPin) == LOW) {
      // Кнопка нажата – эмулируем полностью нажатый правый триггер (R2)
      bleGamepad.setRightTrigger(1023);
    } else {
      // Отпущена – триггер в нуле
      bleGamepad.setRightTrigger(0);
    }

    delay(10);
  } else {
    #ifdef ENABLE_LED
    digitalWrite(ledPin, LOW);
    #endif
    delay(1000);
  }
}
