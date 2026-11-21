// =============================================
// SISTEM MONITORING GAS - MQ-6 dan MQ-135 pakai MQUnifiedsensor
// Fitur: MQ-6 LPG pakai Rs/R0 + kurva eksponensial
// MQ-135 tetap eksponensial
// =============================================

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6vsJK_FW7"
#define BLYNK_TEMPLATE_NAME "Pendeksi Gas New"
#define BLYNK_AUTH_TOKEN "cDgP9Y6qogilxVJkQMgLB5M3roPy_9aP"

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

// --------------------------------------------------------------
// WIFI
// --------------------------------------------------------------
char ssid[] = "HIMATRO";
char pass[] = "BEBANgaji";

#define EspSerial Serial1
#define ESP8266_BAUD 115200

ESP8266 wifi(&EspSerial);

BlynkTimer timer;

// --------------------------------------------------------------
// SENSOR, DFPLAYER, EEPROM
// --------------------------------------------------------------
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <MQUnifiedsensor.h>
#include <EEPROM.h>

  
#define MQ6_PIN A3
#define MQ135_PIN A0
#define LED_HIJAU 2
#define LED_KUNING 3
#define LED_MERAH 4
#define BUZZER 5
#define RELAY1_PIN 6
#define RELAY2_PIN 7

SoftwareSerial mp3Serial(10, 11);
DFRobotDFPlayerMini myDFPlayer;
bool dfReady = false;
bool wifiAvailable = false;
bool manualRelayOn = false;

#define Board "Arduino Mega"
MQUnifiedsensor MQ6(Board, 5, 10, MQ6_PIN, "MQ-6");
MQUnifiedsensor MQ135(Board, 5, 10, MQ135_PIN, "MQ-135");

// --------------------------------------------------------------
// KONFIGURASI KALIBRASI SENSOR
// --------------------------------------------------------------
const float VREF = 5.0;                           // Tegangan referensi ADC Arduino Mega default
const float RatioMQ6CleanAir = 10.0;
const float RatioMQ135CleanAir = 3.6;
const float PROPANE_RATIO = 0.30;                 // Estimasi komposisi LPG: 30% propana
const float BUTANE_RATIO = 0.70;                  // Estimasi komposisi LPG: 70% butana

// Struktur kalibrasi
struct CalData {
  float kCO2;
  float r0MQ6;
  uint32_t magic;
};
CalData cal;
const uint32_t CAL_MAGIC = 0xA5C01236;

// --------------------------------------------------------------
// UTILITAS
// --------------------------------------------------------------
float smoothVoltage(int pin, int n) {
  float sum = 0;
  for (int i = 0; i < n; i++) {
    sum += analogRead(pin);
    delay(30);
  }
  return (sum / n / 1023.0) * VREF;
}

// Debug raw voltage MQ-6
float readRawVoltageMQ6() {
  return smoothVoltage(MQ6_PIN, 1);  // Baca sekali untuk debug real-time
}

void saveCal() { EEPROM.put(0, cal); }

bool loadCal() {
  EEPROM.get(0, cal);
  return cal.magic == CAL_MAGIC
    && isfinite(cal.kCO2)
    && isfinite(cal.r0MQ6)
    && cal.kCO2 > 0.01f
    && cal.kCO2 < 100.0f
    && cal.r0MQ6 > 0.01f
    && cal.r0MQ6 < 1000.0f;
}

void autoBaseline() {
  Serial.println("[BASELINE] Mulai 3 menit... jauhi sumber gas.");
  Serial.println("[CAL] Kalibrasi R0 MQ6...");
  unsigned long T0 = millis();
  float sumC = 0, sumR0MQ6 = 0;
  int count = 0;

  while (millis() - T0 < 180000UL) {
    MQ6.update();
    MQ135.update();
    float r0_mq6 = MQ6.calibrate(RatioMQ6CleanAir);
    float co2_raw = MQ135.readSensor();
    sumR0MQ6 += r0_mq6;
    sumC += co2_raw;
    count++;
    delay(150);
  }

  float avgC = sumC / count;
  cal.kCO2 = (avgC > 0.1f) ? (400.0 / avgC) : 1.0f;
  cal.r0MQ6 = sumR0MQ6 / count;
  cal.magic = CAL_MAGIC;
  saveCal();

  MQ6.setR0(cal.r0MQ6);
  Serial.print("[BASELINE] kCO2 = "); Serial.println(cal.kCO2);
  Serial.print("[BASELINE] MQ6 R0 = "); Serial.println(cal.r0MQ6, 3);
}

void updateConnectionIndicator() {
  if (wifiAvailable && Blynk.connected()) {
    digitalWrite(LED_HIJAU, HIGH);
  } else {
    digitalWrite(LED_HIJAU, (millis() % 1000 < 500));
  }
}

// --------------------------------------------------------------
// DIAGNOSTIK ESP
// --------------------------------------------------------------
bool checkESP() {
  Serial.println("[CHECK] Membersihkan buffer...");
  while (EspSerial.available()) EspSerial.read();
  Serial.println("[CHECK] Mengirim AT...");
  EspSerial.print("AT\r\n");
  long timeout = millis() + 1500;
  while (millis() < timeout) {
    if (EspSerial.available()) {
      String r = EspSerial.readStringUntil('\n');
      r.trim();
      Serial.print("[RESP] ");
      Serial.println(r);
      if (r.startsWith("OK")) return true;
    }
  }
  return false;
}

// --------------------------------------------------------------
// SETUP
// --------------------------------------------------------------
int currentStatus = 0;

void setup() {
  Serial.begin(9600);
  delay(300);
  Serial.println("==== STARTING SYSTEM (MQ-6 MQUnifiedsensor) ====");

  EspSerial.begin(ESP8266_BAUD);
  delay(1000);

  wifiAvailable = checkESP();
  if (wifiAvailable) {
    Serial.println("[OK] ESP-01 siap.");
    Blynk.config(wifi, BLYNK_AUTH_TOKEN);
    Blynk.connectWiFi(ssid, pass);
    Blynk.connect(5000);
  } else {
    Serial.println("[WARNING] ESP-01 tidak merespons AT. Sistem lanjut mode offline.");
  }

  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  mp3Serial.begin(9600);
  dfReady = myDFPlayer.begin(mp3Serial);
  if (dfReady) {
    myDFPlayer.volume(30);
    Serial.println("[OK] DFPlayer siap.");
  } else {
    Serial.println("[WARNING] DFPlayer tidak terdeteksi.");
  }

  // Setup MQ-6 LPG (eksponensial)
  MQ6.setRegressionMethod(1);
  MQ6.setA(1009.2); MQ6.setB(-2.35);
  MQ6.init();

  // Setup MQ135 (eksponensial)
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.86);
  MQ135.init();

  Serial.println("[CAL] Kalibrasi R0 MQ135...");
  float r0_135 = 0;
  for (int i = 0; i < 40; i++) {
    MQ135.update();
    r0_135 += MQ135.calibrate(RatioMQ135CleanAir);
    delay(300);
  }
  MQ135.setR0(r0_135 / 40);

  if (!loadCal()) {
    autoBaseline();
  } else {
    MQ6.setR0(cal.r0MQ6);
    Serial.println("[OK] Baseline dimuat dari EEPROM.");
  }

  timer.setInterval(500L, updateConnectionIndicator);

  Serial.println("==== SISTEM SIAP (MQ-6 MQUnifiedsensor) ====");
}

// --------------------------------------------------------------
// BLYNK HANDLER
// --------------------------------------------------------------
BLYNK_WRITE(V4) {
  manualRelayOn = param.asInt() == 1;
}

BLYNK_WRITE(V5) {
  if (param.asInt() == 1) {
    autoBaseline();
    safeVirtualWrite(V5, 0.0);
  }
}

BLYNK_CONNECTED() {
  Serial.println("[BLYNK] Terhubung ke server!");
}

bool applyRelayOutputs() {
  bool relayOn = currentStatus >= 1 || manualRelayOn;
  digitalWrite(RELAY1_PIN, relayOn ? LOW : HIGH);
  digitalWrite(RELAY2_PIN, relayOn ? LOW : HIGH);
  return relayOn;
}

void safeVirtualWrite(uint8_t pin, float value) {
  if (wifiAvailable && Blynk.connected()) Blynk.virtualWrite(pin, value);
}

void safeVirtualWrite(uint8_t pin, const char* value) {
  if (wifiAvailable && Blynk.connected()) Blynk.virtualWrite(pin, value);
}

// --------------------------------------------------------------
// LOOP
// --------------------------------------------------------------
void loop() {
  if (wifiAvailable) {
    Blynk.run();
  }
  timer.run();

  // Baca MQ-6: metode Rs/R0 eksponensial
  float lpg_ppm = 0;
  for (int i = 0; i < 8; i++) {
    MQ6.update();
    lpg_ppm += MQ6.readSensor();
    delay(30);
  }
  lpg_ppm /= 8;
  float propane_ppm = lpg_ppm * PROPANE_RATIO;
  float butane_ppm = lpg_ppm * BUTANE_RATIO;

  // Baca MQ135: metode eksponensial
  float co2_raw = 0;
  for (int i = 0; i < 8; i++) {
    MQ135.update();
    co2_raw += MQ135.readSensor();
    delay(30);
  }
  float co2_ppm = (co2_raw / 8) * cal.kCO2;
  co2_ppm = constrain(co2_ppm, 350, 5000);

  // Debug
  float rawVoltageMQ6 = readRawVoltageMQ6();
  Serial.print("LPG (MQ-6 Rs/R0): "); Serial.print(lpg_ppm, 1);
  Serial.print(" ppm | Propana: "); Serial.print(propane_ppm, 1);
  Serial.print(" ppm | Butana: "); Serial.print(butane_ppm, 1);
  Serial.print(" ppm | CO2: "); Serial.print(co2_ppm, 0);
  Serial.print(" ppm | MQ6 Raw Voltage: "); Serial.println(rawVoltageMQ6, 3);

  safeVirtualWrite(V0, propane_ppm);
  safeVirtualWrite(V1, butane_ppm);
  safeVirtualWrite(V6, co2_ppm);

  // Logika alarm (threshold bisa disesuaikan)
  int newStatus = 0;
  if (lpg_ppm > 800 || co2_ppm > 1000) newStatus = 2;
  else if (lpg_ppm > 500 || co2_ppm > 500) newStatus = 1;

  if (newStatus != currentStatus) {
    currentStatus = newStatus;

    if (currentStatus == 2) {
      digitalWrite(LED_MERAH, HIGH);
      digitalWrite(LED_KUNING, LOW);
      tone(BUZZER, 1000);
      if (dfReady) myDFPlayer.play(1);
      safeVirtualWrite(V2, "BAHAYA");
      safeVirtualWrite(V7, "Kipas ON, evakuasi");
    }
    else if (currentStatus == 1) {
      digitalWrite(LED_KUNING, HIGH);
      digitalWrite(LED_MERAH, LOW);
      tone(BUZZER, 2000, 300);
      if (dfReady) myDFPlayer.play(2);
      safeVirtualWrite(V2, "WASPADA");
      safeVirtualWrite(V7, "Periksa ruangan");
    }
    else {
      digitalWrite(LED_MERAH, LOW);
      digitalWrite(LED_KUNING, LOW);
      noTone(BUZZER);
      if (dfReady) myDFPlayer.play(3);
      safeVirtualWrite(V2, "AMAN");
      safeVirtualWrite(V7, "Tidak ada tindakan");
    }
  }

  bool kipasOn = applyRelayOutputs();
  safeVirtualWrite(V8, kipasOn ? "ON" : "OFF");
  updateConnectionIndicator();
  delay(1000);
}