#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ (меняйте при необходимости) =================
const int steeringWheelPin = 34;  // Аналоговый вход руля (подстроечный резистор 10 кОм)
const int gasButtonPin = 25;      // Кнопка газа (подключена к GND, используется внутренняя подтяжка)

// Опциональный светодиод на GPIO 2 (раскомментируйте строку ниже, если подключили)
// #define ENABLE_LED
#ifdef ENABLE_LED
const int ledPin = 2;
#endif

// ====== ДАЛЕЕ КОД ЛУЧШЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

// --- Переменные для руля (улучшенная фильтрация и адаптивный центр) ---
int centerSteeringValue = 2048;   // Значение центра, запомненное при старте
float emaSteeringValue = 2048.0;  // Текущее значение EMA фильтра
float emaAlpha = 0.1;             // Коэффициент фильтрации EMA (0.0 - 1.0)
int deadZone = 200;               // Мертвая зона в отсчетах АЦП

// --- Переменные для кнопки газа (возвращён стабильный цифровой метод) ---
bool gasPressed = false;
bool lastGasButtonState = HIGH;

// --- Переменные для отладки ---
int lastReportedSteering = 0;
unsigned long lastDebugPrint = 0;
const unsigned long debugInterval = 500;

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  #ifdef ENABLE_LED
  pinMode(ledPin, OUTPUT);
  #endif

  // --- Калибровка центрального положения руля ---
  Serial.println("Калибровка центра руля...");
  delay(500); // Небольшая задержка для стабилизации питания
  
  long sum = 0;
  const int calibrationSamples = 50;
  for (int i = 0; i < calibrationSamples; i++) {
    sum += analogRead(steeringWheelPin);
    delay(10);
  }
  centerSteeringValue = sum / calibrationSamples;
  
  // Инициализация фильтра значением центра
  emaSteeringValue = (float)centerSteeringValue;

  Serial.print("Центр руля откалиброван: ");
  Serial.println(centerSteeringValue);
  Serial.println("Отклонение руля влево: (центр - значение)");
  Serial.println("Отклонение руля вправо: (центр + значение)");
  Serial.println("BLE Gamepad готов к подключению");

  bleGamepad.begin();

  #ifdef ENABLE_LED
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  #endif
}

void loop() {
  if (bleGamepad.isConnected()) {
    #ifdef ENABLE_LED
    digitalWrite(ledPin, HIGH);
    #endif

    // --- Руль (ось X) ---
    int raw = analogRead(steeringWheelPin);
    
    // Применяем экспоненциальное бегущее среднее (EMA) для сглаживания
    emaSteeringValue = (emaAlpha * raw) + ((1 - emaAlpha) * emaSteeringValue);
    int smoothedValue = (int)emaSteeringValue;
    
    // Вычисляем отклонение от центра
    int deviation = smoothedValue - centerSteeringValue;
    
    // Применяем мертвую зону
    if (abs(deviation) < deadZone) {
      deviation = 0;
    }

    // Масштабируем отклонение в диапазон оси X (-32767 до 32767)
    int steering = 0;
    if (deviation > 0) {
      // Отклонение вправо
      steering = map(deviation, 0, 4095 - centerSteeringValue, 0, 32767);
    } else if (deviation < 0) {
      // Отклонение влево
      steering = map(deviation, -centerSteeringValue, 0, -32767, 0);
    }
    
    bleGamepad.setX(steering);

    // --- Кнопка газа (R2) – надёжный цифровой метод ---
    bool gasButtonState = digitalRead(gasButtonPin);
    
    if (gasButtonState == LOW && lastGasButtonState == HIGH) {
      // Кнопка только что нажата
      gasPressed = true;
      bleGamepad.press(BUTTON_8);   // Нажимаем кнопку R2
      Serial.println("ГАЗ НАЖАТ");
    } 
    else if (gasButtonState == HIGH && lastGasButtonState == LOW) {
      // Кнопка только что отпущена
      gasPressed = false;
      bleGamepad.release(BUTTON_8); // Отпускаем кнопку R2
      Serial.println("ГАЗ ОТПУЩЕН");
    }
    
    lastGasButtonState = gasButtonState;

    // --- Отладочный вывод (значения руля при изменении) ---
    if (abs(steering - lastReportedSteering) > 500) {
      if (millis() - lastDebugPrint > debugInterval) {
        Serial.print("Руль (отклонение): ");
        Serial.print(deviation);
        Serial.print(" -> Ось X: ");
        Serial.println(steering);
        lastReportedSteering = steering;
        lastDebugPrint = millis();
      }
    }

    delay(10);
  } else {
    #ifdef ENABLE_LED
    digitalWrite(ledPin, LOW);
    #endif
    delay(1000);
  }
}
