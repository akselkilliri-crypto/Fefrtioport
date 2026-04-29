#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ (меняйте при необходимости) =================
const int steeringWheelPin = 34;  // Аналоговый вход руля (подстроечный резистор 10 кОм)
const int gasButtonPin = 25;      // Кнопка газа (подключена к GND, используется внутренняя подтяжка)

// Опциональный светодиод на GPIO 2 (раскомментируйте, если подключили)
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

// Переменные для кнопки газа и предотвращения дребезга
bool gasPressed = false;
bool lastGasButtonState = HIGH;

// Переменные для отладочного вывода
int lastReportedSteering = 0;          // Предыдущее отправленное значение руля
unsigned long lastDebugPrint = 0;      // Время последнего вывода отладки
const unsigned long debugInterval = 500; // Минимальный интервал между выводами (мс)
bool bleWasConnected = false;          // Предыдущее состояние BLE-подключения

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
  bool currentlyConnected = bleGamepad.isConnected();

  // --- Отслеживание изменения статуса BLE-подключения ---
  if (currentlyConnected != bleWasConnected) {
    bleWasConnected = currentlyConnected;
    if (currentlyConnected) {
      Serial.println(">>> BLE ПОДКЛЮЧЕНО <<<");
      #ifdef ENABLE_LED
      digitalWrite(ledPin, HIGH);
      #endif
    } else {
      Serial.println("--- BLE ОТКЛЮЧЕНО ---");
      #ifdef ENABLE_LED
      digitalWrite(ledPin, LOW);
      #endif
    }
  }

  if (currentlyConnected) {
    // --- Руль (ось X) ---
    int raw = analogRead(steeringWheelPin);
    total = total - readings[readIndex];
    readings[readIndex] = raw;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    average = total / numReadings;

    int steering = map(average, 0, 4095, -32767, 32767);
    bleGamepad.setX(steering);

    // --- Кнопка газа (R2) ---
    bool gasButtonState = digitalRead(gasButtonPin);
    
    if (gasButtonState == LOW && lastGasButtonState == HIGH) {
      // Кнопка только что нажата
      gasPressed = true;
      bleGamepad.press(BUTTON_8); // Нажимаем кнопку R2
      Serial.println("ГАЗ НАЖАТ");
    } 
    else if (gasButtonState == HIGH && lastGasButtonState == LOW) {
      // Кнопка только что отпущена
      gasPressed = false;
      bleGamepad.release(BUTTON_8); // Отпускаем кнопку R2
      Serial.println("ГАЗ ОТПУЩЕН");
    }
    
    lastGasButtonState = gasButtonState;

    // --- Периодический вывод значений руля (только при изменении более чем на 500) ---
    if (abs(steering - lastReportedSteering) > 500) {
      if (millis() - lastDebugPrint > debugInterval) {
        Serial.print("Руль: ");
        Serial.println(steering);
        lastReportedSteering = steering;
        lastDebugPrint = millis();
      }
    }

    delay(10);
  } else {
    // Если BLE не подключено – экономим ресурсы
    delay(1000);
  }
}
