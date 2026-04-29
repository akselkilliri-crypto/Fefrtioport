#include <Arduino.h>
#include <BleGamepad.h>

// ================= НАСТРОЙКИ =================
const int steeringWheelPin = 34;  // Аналоговый вход руля
const int gasButtonPin = 25;      // Кнопка газа

// Ширина мёртвой зоны вокруг центра (в отсчётах АЦП, можно менять)
const int DEAD_ZONE = 80;   // чем больше, тем шире центральное "окно"

// ====== ОСТАЛЬНОЕ НЕ ТРОГАТЬ ======
BleGamepad bleGamepad("ESP32 Racing Wheel", "ESP32 Community", 100);

// Центр руля, запомненный при включении
int centerRaw = 2048;        // будет переопределён в setup()
int leftLimit, rightLimit;   // границы зон

// Газ
bool lastGasButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(steeringWheelPin, INPUT);
  pinMode(gasButtonPin, INPUT_PULLUP);

  // --- Калибровка центра (не трогайте руль первые секунды!) ---
  delay(500); // даём питанию устаканиться
  long sum = 0;
  const int samples = 50;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(steeringWheelPin);
    delay(10);
  }
  centerRaw = sum / samples;

  // Вычисляем границы зон относительно центра
  leftLimit  = centerRaw - DEAD_ZONE;   // всё, что меньше – лево
  rightLimit = centerRaw + DEAD_ZONE;   // всё, что больше – право

  Serial.print("Центр откалиброван: ");
  Serial.println(centerRaw);
  Serial.print("Левая зона: 0 ... ");
  Serial.println(leftLimit);
  Serial.print("Центральная зона: ");
  Serial.print(leftLimit);
  Serial.print(" ... ");
  Serial.println(rightLimit);
  Serial.print("Правая зона: ");
  Serial.print(rightLimit);
  Serial.println(" ... 4095");

  bleGamepad.begin();
  Serial.println("BLE Gamepad готов (без фильтра, с автокалибровкой)");
}

void loop() {
  if (bleGamepad.isConnected()) {

    // --- РУЛЬ (без фильтра) ---
    int raw = analogRead(steeringWheelPin);

    int steering = 0;

    if (raw < leftLimit) {
      // ЛЕВО: масштабируем от leftLimit (центр) до 0 (крайне левое положение)
      steering = map(raw, leftLimit, 0, 0, 32767);
      steering = constrain(steering, 0, 32767);
      steering = -steering;  // лево – отрицательные значения оси X
    }
    else if (raw > rightLimit) {
      // ПРАВО: масштабируем от rightLimit до 4095 (крайне правое положение)
      steering = map(raw, rightLimit, 4095, 0, 32767);
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
