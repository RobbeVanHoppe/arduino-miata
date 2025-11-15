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

using ValueFormatter = void (*)(const SensorReadings&, char*, size_t);

struct MenuPage {
    const char* title;
    const char* units;
    ValueFormatter formatValue;
};

void formatRpmValue(const SensorReadings& readings, char* buffer, size_t len);
void formatWaterTempValue(const SensorReadings& readings, char* buffer, size_t len);
void formatOilPressureValue(const SensorReadings& readings, char* buffer, size_t len);
void formatHandbrakeValue(const SensorReadings& readings, char* buffer, size_t len);

// Add new entries here to expose more menu pages (handy for a rotary encoder).
constexpr MenuPage MENU_PAGES[] = {
    {"Engine", "rpm", formatRpmValue},
    {"Water", "\xB0C", formatWaterTempValue},
    {"Oil", "psi", formatOilPressureValue},
    {"Handbrake", nullptr, formatHandbrakeValue},
};

constexpr size_t MENU_PAGE_COUNT = sizeof(MENU_PAGES) / sizeof(MENU_PAGES[0]);
size_t activeMenuPage = 0;

void setActiveMenuPage(size_t index) {
    if (MENU_PAGE_COUNT == 0) {
        return;
    }
    activeMenuPage = index % MENU_PAGE_COUNT;
}

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

void drawCenteredText(const char* text, int16_t centerY, uint8_t textSize, uint16_t color) {
    if (!text) {
        return;
    }

    tft.setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int16_t x = DISPLAY_CENTER_X - static_cast<int16_t>(w / 2);
    int16_t y = centerY - static_cast<int16_t>(h / 2);
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.print(text);
}

void drawPageIndicators(size_t selectedIndex) {
    if (MENU_PAGE_COUNT <= 1) {
        return;
    }

    const int16_t indicatorY = DISPLAY_CENTER_Y + DISPLAY_RADIUS - 25;
    const int16_t spacing = 18;
    const int16_t totalWidth = (MENU_PAGE_COUNT - 1) * spacing;
    const int16_t startX = DISPLAY_CENTER_X - totalWidth / 2;

    for (size_t i = 0; i < MENU_PAGE_COUNT; ++i) {
        uint16_t color = (i == selectedIndex) ? GC9A01A_WHITE : GC9A01A_DARKGREY;
        uint8_t radius = (i == selectedIndex) ? 5 : 3;
        tft.fillCircle(startX + static_cast<int16_t>(i) * spacing, indicatorY, radius, color);
    }
}

void drawMenuPage(const MenuPage& page, const SensorReadings& readings) {
    tft.fillScreen(GC9A01A_BLACK);
    tft.fillCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, DISPLAY_RADIUS, GC9A01A_BLACK);
    tft.drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, DISPLAY_RADIUS, GC9A01A_DARKGREY);
    tft.drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, DISPLAY_RADIUS - 2, GC9A01A_DARKGREY);

    drawCenteredText(page.title, DISPLAY_CENTER_Y - 70, 2, GC9A01A_CYAN);

    char valueBuffer[32] = {0};
    page.formatValue(readings, valueBuffer, sizeof(valueBuffer));
    drawCenteredText(valueBuffer, DISPLAY_CENTER_Y - 5, 4, GC9A01A_WHITE);

    if (page.units && page.units[0] != '\0') {
        drawCenteredText(page.units, DISPLAY_CENTER_Y + 45, 2, GC9A01A_LIGHTGREY);
    }

    drawPageIndicators(activeMenuPage);
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
}

void updateDisplay(const SensorReadings& readings) {
    if (MENU_PAGE_COUNT == 0) {
        return;
    }
    const MenuPage& page = MENU_PAGES[activeMenuPage];
    drawMenuPage(page, readings);
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

void formatRpmValue(const SensorReadings& readings, char* buffer, size_t len) {
    snprintf(buffer, len, "%4u", readings.rpm);
}

void formatWaterTempValue(const SensorReadings& readings, char* buffer, size_t len) {
    if (isnan(readings.waterTempC)) {
        snprintf(buffer, len, "--.-");
    } else {
        snprintf(buffer, len, "%5.1f", readings.waterTempC);
    }
}

void formatOilPressureValue(const SensorReadings& readings, char* buffer, size_t len) {
    if (isnan(readings.oilPressurePsi)) {
        snprintf(buffer, len, "--.-");
    } else {
        snprintf(buffer, len, "%5.1f", readings.oilPressurePsi);
    }
}

void formatHandbrakeValue(const SensorReadings& readings, char* buffer, size_t len) {
    const char* text = readings.handbrakeEngaged ? "ENGAGED" : "Released";
    snprintf(buffer, len, "%s", text);
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
    setActiveMenuPage(0);
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
