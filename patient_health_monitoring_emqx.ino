
#include <Wire.h>
#include <DHT.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

// ==================================================
// USER SETTINGS — ENTER YOUR ROTATED CREDENTIALS
// ==================================================
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

const char* MQTT_HOST = "h4d22089.ala.asia-southeast1.emqxsl.com";
const int MQTT_PORT = 8883;
const char* MQTT_USERNAME = "esp32_device001";
const char* MQTT_PASSWORD = "";
const char* MQTT_TOPIC = "health/esp32/device001/telemetry";
const char* DEVICE_ID = "device001";

// ==================================================
// DIGICERT GLOBAL ROOT G2 CERTIFICATE
// ==================================================
static const char* ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpY2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// ==================================================
// PIN DEFINITIONS
// ==================================================
#define DHT_PIN 4
#define DHT_TYPE DHT11

#define HW827_PIN 34

#define SDA_PIN 21
#define SCL_PIN 22

// ==================================================
// SENSOR AND NETWORK OBJECTS
// ==================================================
DHT dht(DHT_PIN, DHT_TYPE);

MAX30105 maxSensor;

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

// ==================================================
// MAX30102 HEART RATE VARIABLES
// ==================================================
float averageBPM = 0;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE] = {0};

byte rateSpot = 0;
byte beatCount = 0;

unsigned long lastMaxBeat = 0;

// ==================================================
// MAX30102 SPO2 VARIABLES
// ==================================================
#define SPO2_BUFFER_SIZE 100

uint32_t redBuffer[SPO2_BUFFER_SIZE];
uint32_t irBuffer[SPO2_BUFFER_SIZE];

int spo2SampleCount = 0;

int32_t spo2Value = 0;
int8_t validSpO2 = 0;

int32_t algorithmHeartRate = 0;
int8_t validAlgorithmHeartRate = 0;

unsigned long lastSpO2Calculation = 0;

// ==================================================
// HW-827 VARIABLES
// ==================================================
float baseline = 0;
float hwBPM = 0;

unsigned long lastHWBeat = 0;
bool pulseDetected = false;

// ==================================================
// GENERAL VARIABLES
// ==================================================
unsigned long lastDHTRead = 0;
unsigned long lastPrint = 0;
unsigned long lastPublish = 0;

float temperature = 0;
float humidity = 0;

long latestIR = 0;
long latestRed = 0;

int latestHWValue = 0;

const unsigned long PUBLISH_INTERVAL_MS = 5000;

// ==================================================
// WIFI CONNECTION
// ==================================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long started = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("\nWi-Fi connection failed. Retrying later.");
  }
}

// ==================================================
// WAIT FOR VALID CLOCK
// ==================================================
bool waitForClock() {
  time_t now = time(nullptr);

  unsigned long started = millis();

  while (now < 1700000000 &&
         millis() - started < 15000) {
    delay(250);
    now = time(nullptr);
  }

  return now >= 1700000000;
}

// ==================================================
// MQTT CONNECTION
// ==================================================
void connectMQTT() {
  if (mqtt.connected() ||
      WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!waitForClock()) {
    Serial.println("Waiting for valid clock for TLS.");
    return;
  }

  Serial.print("Connecting to EMQX...");

  String clientId =
    String("esp32-") +
    DEVICE_ID +
    "-" +
    String((uint32_t)ESP.getEfuseMac(), HEX);

  if (mqtt.connect(
        clientId.c_str(),
        MQTT_USERNAME,
        MQTT_PASSWORD)) {

    Serial.println("connected to EMQX");

  } else {
    Serial.print("MQTT connection failed. State: ");
    Serial.println(mqtt.state());
  }
}

// ==================================================
// MQTT TELEMETRY PUBLISHING
// ==================================================
void publishTelemetry(bool fingerDetected) {
  char payload[320];

  float reportedHeartRate =
    (fingerDetected && averageBPM > 0) ? averageBPM : 0;

  float reportedHW827 =
    hwBPM;

  if (fingerDetected && validSpO2) {

    snprintf(payload, sizeof(payload),
      "{\"deviceId\":\"%s\","
      "\"temperature\":%.1f,"
      "\"humidity\":%.1f,"
      "\"heartRate\":%.1f,"
      "\"hw827HeartRate\":%.1f,"
      "\"spo2\":%ld,"
      "\"fingerDetected\":true}",
      DEVICE_ID,
      temperature,
      humidity,
      reportedHeartRate,
      reportedHW827,
      (long)spo2Value);

  } else {

    snprintf(payload, sizeof(payload),
      "{\"deviceId\":\"%s\","
      "\"temperature\":%.1f,"
      "\"humidity\":%.1f,"
      "\"heartRate\":%.1f,"
      "\"hw827HeartRate\":%.1f,"
      "\"spo2\":null,"
      "\"fingerDetected\":%s}",
      DEVICE_ID,
      temperature,
      humidity,
      reportedHeartRate,
      reportedHW827,
      fingerDetected ? "true" : "false");
  }

  if (mqtt.publish(MQTT_TOPIC, payload)) {
    Serial.print("MQTT published: ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT publish failed.");
  }
}

// ==================================================
// MAX30102 SAMPLE PROCESSING
// ==================================================
void processMAX30102() {
  maxSensor.check();

  while (maxSensor.available()) {

    latestRed = maxSensor.getRed();
    latestIR = maxSensor.getIR();

    // Fill the first 100 samples.
    if (spo2SampleCount < SPO2_BUFFER_SIZE) {

      redBuffer[spo2SampleCount] = (uint32_t)latestRed;
      irBuffer[spo2SampleCount] = (uint32_t)latestIR;

      spo2SampleCount++;

    } else {

      // Maintain a rolling 100-sample buffer.
      for (int i = 0; i < SPO2_BUFFER_SIZE - 1; i++) {
        redBuffer[i] = redBuffer[i + 1];
        irBuffer[i] = irBuffer[i + 1];
      }

      redBuffer[SPO2_BUFFER_SIZE - 1] = (uint32_t)latestRed;
      irBuffer[SPO2_BUFFER_SIZE - 1] = (uint32_t)latestIR;
    }

    // Existing MAX30102 heart-rate detection.
    if (checkForBeat(latestIR)) {

      unsigned long now = millis();

      if (lastMaxBeat > 0) {

        float interval = now - lastMaxBeat;
        float bpm = 60000.0 / interval;

        if (bpm > 40 && bpm < 200) {

          rates[rateSpot++] = (byte)bpm;
          rateSpot %= RATE_SIZE;

          if (beatCount < RATE_SIZE) {
            beatCount++;
          }

          float sum = 0;

          for (byte i = 0; i < beatCount; i++) {
            sum += rates[i];
          }

          averageBPM = sum / beatCount;
        }
      }

      lastMaxBeat = now;
    }

    maxSensor.nextSample();
  }

  bool fingerDetected = latestIR > 50000;

  // Calculate SpO2 once per second after collecting 100 samples.
  if (fingerDetected &&
      spo2SampleCount == SPO2_BUFFER_SIZE &&
      millis() - lastSpO2Calculation >= 1000) {

    lastSpO2Calculation = millis();

    maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      SPO2_BUFFER_SIZE,
      redBuffer,
      &spo2Value,
      &validSpO2,
      &algorithmHeartRate,
      &validAlgorithmHeartRate
    );

    if (!validSpO2 ||
        spo2Value < 70 ||
        spo2Value > 100) {
      validSpO2 = 0;
    }

  } else if (!fingerDetected) {

    validSpO2 = 0;
    spo2SampleCount = 0;

    averageBPM = 0;
    beatCount = 0;
    rateSpot = 0;
    lastMaxBeat = 0;
  }
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);

  dht.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(HW827_PIN, ADC_11db);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!maxSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring!");
    while (true) {
      delay(1000);
    }
  }

  // Configure MAX30102:
  // LED power, sample average, LED mode, sample rate,
  // pulse width, ADC range.
  maxSensor.setup(0x1F, 4, 2, 100, 411, 4096);

  maxSensor.setPulseAmplitudeRed(0x1F);
  maxSensor.setPulseAmplitudeIR(0x1F);
  maxSensor.setPulseAmplitudeGreen(0);

  secureClient.setCACert(ROOT_CA);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  Serial.println("All sensors initialized!");

  connectWiFi();
}

// ==================================================
// MAIN LOOP
// ==================================================
void loop() {
  unsigned long currentTime = millis();

  // -------- WIFI --------
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // -------- MQTT --------
  if (!mqtt.connected()) {

    static unsigned long lastMqttAttempt = 0;

    if (millis() - lastMqttAttempt >= 5000) {
      lastMqttAttempt = millis();
      connectMQTT();
    }

  } else {
    mqtt.loop();
  }

  // -------- DHT11 --------
  if (currentTime - lastDHTRead >= 2000) {

    lastDHTRead = currentTime;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity = h;
    }
  }

  // -------- MAX30102 --------
  processMAX30102();

  bool fingerDetected = latestIR > 50000;

  // -------- HW-827 --------
  latestHWValue = analogRead(HW827_PIN);

  if (baseline == 0) {
    baseline = latestHWValue;
  }

  baseline = baseline * 0.99 + latestHWValue * 0.01;

  float threshold = baseline + 120;

  if (latestHWValue > threshold && !pulseDetected) {

    unsigned long now = millis();

    if (lastHWBeat > 0) {

      unsigned long interval = now - lastHWBeat;

      if (interval > 300 && interval < 2000) {
        hwBPM = 60000.0 / interval;
      }
    }

    lastHWBeat = now;
    pulseDetected = true;
  }

  if (latestHWValue < baseline + 40) {
    pulseDetected = false;
  }

  // -------- SERIAL MONITOR --------
  if (currentTime - lastPrint >= 1000) {

    lastPrint = currentTime;

    Serial.println("\n========== SENSOR DATA ==========");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("MAX30102 IR: ");
    Serial.println(latestIR);

    Serial.print("MAX30102 Red: ");
    Serial.println(latestRed);

    if (fingerDetected && averageBPM > 0) {
      Serial.print("MAX30102 Heart Rate: ");
      Serial.print(averageBPM);
      Serial.println(" BPM");
    } else {
      Serial.println("MAX30102 Heart Rate: Not available");
    }

    Serial.print("SpO2: ");

    if (fingerDetected && validSpO2) {
      Serial.print(spo2Value);
      Serial.println(" %");
    } else {
      Serial.println("Not available");
    }

    Serial.print("HW-827 Analog: ");
    Serial.println(latestHWValue);

    Serial.print("HW-827 Heart Rate: ");

    if (hwBPM > 0) {
      Serial.print(hwBPM);
      Serial.println(" BPM");
    } else {
      Serial.println("Waiting for pulse...");
    }

    Serial.println("=================================");
  }

  // -------- MQTT TELEMETRY --------
  if (mqtt.connected() &&
      currentTime - lastPublish >= PUBLISH_INTERVAL_MS) {

    lastPublish = currentTime;

    publishTelemetry(fingerDetected);
  }
}
