#include <unity.h>
#include <cmath>

#include "esp32_dash/sensors/WaterSensor.h"
#include "esp32_dash/display/DisplayManager.h"
#include "esp32_dash/display/pages/WaterTempPage.h"
#include "Arduino.h"

namespace {
const WaterSensor::Config kConfig{
    .analogPin = 1,
    .referenceVoltage = 3.3f,
    .adcResolution = 4095,
    .pullupResistorOhms = 4700.0f,
    .sampleIntervalMs = 500,
    .samples = 4,
    .changeThresholdC = 0.5f,
};
}

void test_interpolate_clamps_to_curve_bounds() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, WaterSensor::interpolateWaterTemp(6000.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, WaterSensor::interpolateWaterTemp(100.0f));
}

void test_interpolate_between_points() {
    const float tempAt900 = WaterSensor::interpolateWaterTemp(900.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, tempAt900);

    const float tempAt1500 = WaterSensor::interpolateWaterTemp(1500.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32.0f, tempAt1500);
}

void test_describe_water_status_ranges() {
    TEST_ASSERT_TRUE(WaterSensor::describeWaterStatus(75.0f) == "Warming up");
    TEST_ASSERT_TRUE(WaterSensor::describeWaterStatus(85.0f) == "");
    TEST_ASSERT_TRUE(WaterSensor::describeWaterStatus(110.0f) == "Hot!");
}

void test_set_enabled_updates_status_and_refresh() {
    WaterTempPage page;
    DisplayManager display;
    WaterSensor sensor(kConfig, page, display);

    sensor.setEnabled(false);

    TEST_ASSERT_TRUE(display.refreshRequested);
    TEST_ASSERT_TRUE(page.lastStatusMessage == "Sleeping");
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_interpolate_clamps_to_curve_bounds);
    RUN_TEST(test_interpolate_between_points);
    RUN_TEST(test_describe_water_status_ranges);
    RUN_TEST(test_set_enabled_updates_status_and_refresh);
    return UNITY_END();
}
