#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include <math.h>
#include <cstring>

// GPIO configuration – adjust to match your wiring
constexpr int LIGHTS_PIN = 2;
constexpr int WINDOWS_PIN = 3;
constexpr int TACH_PIN = 26;
constexpr int WATER_TEMP_PIN = 34;
constexpr int OIL_PRESSURE_PIN = 35;
constexpr int HANDBRAKE_PIN = 27;
constexpr int TFT_CS_PIN = 5;
constexpr int TFT_DC_PIN = 16;
constexpr int TFT_RST_PIN = 17;
constexpr int TFT_BACKLIGHT_PIN = 4;
constexpr int TFT_SDA_PIN = 23;  // GC9A01 data/MOSI
constexpr int TFT_SCL_PIN = 18;  // GC9A01 clock/SCK

// Analog sensor calibration (change to match your specific senders)
constexpr float ANALOG_REFERENCE_V = 3.3f;
constexpr int ADC_RESOLUTION = 4095;
constexpr float WATER_TEMP_SENSOR_V_MIN = 0.5f;
constexpr float WATER_TEMP_SENSOR_V_MAX = 4.5f;
constexpr float WATER_TEMP_MIN_C = 40.0f;
constexpr float WATER_TEMP_MAX_C = 140.0f;
constexpr float OIL_PRESSURE_SENSOR_V_MIN = 0.5f;
constexpr float OIL_PRESSURE_SENSOR_V_MAX = 4.5f;
constexpr float OIL_PRESSURE_MIN_PSI = 0.0f;
constexpr float OIL_PRESSURE_MAX_PSI = 100.0f;
constexpr uint8_t HANDBRAKE_ACTIVE_LEVEL = LOW;

// Tachometer calibration
constexpr float TACH_PULSES_PER_REV = 2.0f;   // number of pulses per crank revolution

// Timing
constexpr unsigned long SENSOR_SAMPLE_INTERVAL_MS = 200;
constexpr unsigned long RPM_SAMPLE_INTERVAL_MS = 500;
constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 250;
constexpr unsigned long BLE_NOTIFY_INTERVAL_MS = 500;
constexpr float SENSOR_FILTER_ALPHA = 0.2f;  // 0..1, higher responds faster

// BLE definitions
#define SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define COMMAND_CHARACTERISTIC_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define TELEMETRY_CHARACTERISTIC_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

struct SensorReadings {
    float waterTempC = NAN;
    float oilPressurePsi = NAN;
    uint16_t rpm = 0;
    bool handbrakeEngaged = false;
};

BLEServer* pServer = nullptr;
BLECharacteristic* pCommandCharacteristic = nullptr;
BLECharacteristic* pTelemetryCharacteristic = nullptr;
bool deviceConnected = false;
SensorReadings currentReadings;

Adafruit_GC9A01A tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

volatile uint32_t tachPulseCount = 0;
portMUX_TYPE tachMux = portMUX_INITIALIZER_UNLOCKED;

// Forward declarations
void updateDisplay(const SensorReadings& readings);
void updateTelemetryCharacteristic(const SensorReadings& readings);

float analogReadVoltage(uint8_t pin) {
    uint16_t raw = analogRead(pin);
    return (static_cast<float>(raw) / static_cast<float>(ADC_RESOLUTION)) * ANALOG_REFERENCE_V;
}

float mapSensorValue(float voltage, float vMin, float vMax, float outMin, float outMax) {
    voltage = constrain(voltage, vMin, vMax);
    float range = vMax - vMin;
    if (fabsf(range) < 0.0001f) {
        return outMin;
    }
    float normalized = (voltage - vMin) / range;
    return outMin + normalized * (outMax - outMin);
}

float filterValue(float previous, float newValue, float alpha) {
    if (isnan(previous)) {
        return newValue;
    }
    return previous + alpha * (newValue - previous);
}

float readWaterTemperatureC(float previousValue) {
    float voltage = analogReadVoltage(WATER_TEMP_PIN);
    float temperature = mapSensorValue(
        voltage,
        WATER_TEMP_SENSOR_V_MIN,
        WATER_TEMP_SENSOR_V_MAX,
        WATER_TEMP_MIN_C,
        WATER_TEMP_MAX_C
    );
    return filterValue(previousValue, temperature, SENSOR_FILTER_ALPHA);
}

float readOilPressurePsi(float previousValue) {
    float voltage = analogReadVoltage(OIL_PRESSURE_PIN);
    float pressure = mapSensorValue(
        voltage,
        OIL_PRESSURE_SENSOR_V_MIN,
        OIL_PRESSURE_SENSOR_V_MAX,
        OIL_PRESSURE_MIN_PSI,
        OIL_PRESSURE_MAX_PSI
    );
    return filterValue(previousValue, pressure, SENSOR_FILTER_ALPHA);
}

bool readHandbrakeEngaged() {
    return digitalRead(HANDBRAKE_PIN) == HANDBRAKE_ACTIVE_LEVEL;
}

void IRAM_ATTR handleTachPulse() {
    portENTER_CRITICAL_ISR(&tachMux);
    tachPulseCount++;
    portEXIT_CRITICAL_ISR(&tachMux);
}

uint16_t sampleRpm() {
    uint32_t pulses = 0;
    portENTER_CRITICAL(&tachMux);
    pulses = tachPulseCount;
    tachPulseCount = 0;
    portEXIT_CRITICAL(&tachMux);

    if (pulses == 0 || TACH_PULSES_PER_REV <= 0.0f) {
        return 0;
    }

    float pulsesPerMinute = (static_cast<float>(pulses) * 60000.0f) / static_cast<float>(RPM_SAMPLE_INTERVAL_MS);
    float rpm = pulsesPerMinute / TACH_PULSES_PER_REV;
    return static_cast<uint16_t>(rpm + 0.5f);
}

void initDisplay() {
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextWrap(false);
    if (TFT_BACKLIGHT_PIN >= 0) {
        pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
        digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
    }

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(2);
    tft.setCursor(25, 10);
    tft.print("Miata Telemetry");
    tft.drawFastHLine(10, 40, 220, GC9A01A_DARKGREY);
}

void drawDataRow(int16_t top, const char* label, const char* value) {
    const int16_t height = 45;
    tft.fillRoundRect(10, top, 220, height, 8, GC9A01A_BLACK);
    tft.drawRoundRect(10, top, 220, height, 8, GC9A01A_DARKGREY);

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(2);
    tft.setCursor(18, top + 15);
    tft.print(label);

    tft.setTextSize(3);
    tft.setCursor(18, top + 28);
    tft.print(value);
}

void updateDisplay(const SensorReadings& readings) {
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%4u rpm", readings.rpm);
    drawDataRow(55, "Engine", buffer);

    if (isnan(readings.waterTempC)) {
        snprintf(buffer, sizeof(buffer), "--.- C");
    } else {
        snprintf(buffer, sizeof(buffer), "%5.1f C", readings.waterTempC);
    }
    drawDataRow(105, "Water", buffer);

    if (isnan(readings.oilPressurePsi)) {
        snprintf(buffer, sizeof(buffer), "--.- psi");
    } else {
        snprintf(buffer, sizeof(buffer), "%5.1f psi", readings.oilPressurePsi);
    }
    drawDataRow(155, "Oil", buffer);

    drawDataRow(205, "Handbrake", readings.handbrakeEngaged ? "ENGAGED" : "Released");
}

void updateTelemetryCharacteristic(const SensorReadings& readings) {
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"rpm\":%u,\"water_c\":%.2f,\"oil_psi\":%.2f,\"handbrake\":%s}",
             readings.rpm,
             isnan(readings.waterTempC) ? -1.0f : readings.waterTempC,
             isnan(readings.oilPressurePsi) ? -1.0f : readings.oilPressurePsi,
             readings.handbrakeEngaged ? "true" : "false");

    pTelemetryCharacteristic->setValue(payload);
    pTelemetryCharacteristic->notify();
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println("Client disconnected");
        server->getAdvertising()->start();
    }
};

void handleLightsCommand(bool on) {
    digitalWrite(LIGHTS_PIN, on ? HIGH : LOW);
    Serial.printf("Lights %s\n", on ? "ON" : "OFF");
}

void handleWindowsCommand(bool up) {
    digitalWrite(WINDOWS_PIN, up ? HIGH : LOW);
    Serial.printf("Windows %s\n", up ? "UP" : "DOWN");
}

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("Received: ");
        Serial.println(rxValue.c_str());

        if (rxValue == "LIGHTS:ON") {
            handleLightsCommand(true);
        } else if (rxValue == "LIGHTS:OFF") {
            handleLightsCommand(false);
        } else if (rxValue == "WINDOWS:UP") {
            handleWindowsCommand(true);
        } else if (rxValue == "WINDOWS:DOWN") {
            handleWindowsCommand(false);
        } else {
            Serial.println("Unknown command");
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Miata brain...");

    pinMode(LIGHTS_PIN, OUTPUT);
    pinMode(WINDOWS_PIN, OUTPUT);
    digitalWrite(LIGHTS_PIN, LOW);
    digitalWrite(WINDOWS_PIN, LOW);

    pinMode(HANDBRAKE_PIN, INPUT_PULLUP);
    pinMode(TACH_PIN, INPUT_PULLUP);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);

    attachInterrupt(digitalPinToInterrupt(TACH_PIN), handleTachPulse, RISING);

    initDisplay();
    updateDisplay(currentReadings);

    Serial.println("Starting BLE...");
    BLEDevice::init("ESP32-Control");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);

    pCommandCharacteristic = pService->createCharacteristic(
        COMMAND_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCommandCharacteristic->setCallbacks(new MyCallbacks());
    pCommandCharacteristic->setValue("READY");

    pTelemetryCharacteristic = pService->createCharacteristic(
        TELEMETRY_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTelemetryCharacteristic->addDescriptor(new BLE2902());
    pTelemetryCharacteristic->setValue("{}");

    pService->start();

    BLEAdvertising* advertising = pServer->getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE device ready, advertising telemetry service");
}

void loop() {
    static unsigned long lastSensorSample = 0;
    static unsigned long lastRpmSample = 0;
    static unsigned long lastDisplayUpdate = 0;
    static unsigned long lastBleNotify = 0;

    unsigned long now = millis();

    if (now - lastSensorSample >= SENSOR_SAMPLE_INTERVAL_MS) {
        currentReadings.waterTempC = readWaterTemperatureC(currentReadings.waterTempC);
        currentReadings.oilPressurePsi = readOilPressurePsi(currentReadings.oilPressurePsi);
        currentReadings.handbrakeEngaged = readHandbrakeEngaged();
        lastSensorSample = now;
    }

    if (now - lastRpmSample >= RPM_SAMPLE_INTERVAL_MS) {
        currentReadings.rpm = sampleRpm();
        lastRpmSample = now;
    }

    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        updateDisplay(currentReadings);
        lastDisplayUpdate = now;
    }

    if (deviceConnected && now - lastBleNotify >= BLE_NOTIFY_INTERVAL_MS) {
        updateTelemetryCharacteristic(currentReadings);
        lastBleNotify = now;
    }

    delay(10);
}
