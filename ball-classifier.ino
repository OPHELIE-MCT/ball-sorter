
#include <Adafruit_AS7341.h>

#include "classification.h"

namespace {

constexpr size_t kRawChannelCount = 12;
constexpr unsigned long kSampleIntervalMs = 500;
constexpr uint8_t kClassifierChannelIndexes[BallClassifier::kFeatureCount] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11};

Adafruit_AS7341 as7341;
unsigned long lastSampleAt = 0;

bool readClassifierFeatures(uint16_t features[BallClassifier::kFeatureCount]) {
    uint16_t rawReadings[kRawChannelCount];
    if (!as7341.readAllChannels(rawReadings)) {
        return false;
    }

    for (size_t index = 0; index < BallClassifier::kFeatureCount; ++index) {
        features[index] = rawReadings[kClassifierChannelIndexes[index]];
    }

    return true;
}

}  // namespace

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(1);
    }

    if (!as7341.begin(57, &Wire1)) {
        Serial.println("Could not find AS7341");
        while (true) {
            delay(10);
        }
    }

    as7341.setATIME(100);
    as7341.setASTEP(100);
    as7341.setGain(AS7341_GAIN_256X);
    as7341.setLEDCurrent(5);
    as7341.enableLED(true);

    Serial.println("AS7341 classifier ready");
}

void loop() {
    if (millis() - lastSampleAt < kSampleIntervalMs) {
        delay(10);
        return;
    }

    lastSampleAt = millis();

    uint16_t features[BallClassifier::kFeatureCount];
    if (!readClassifierFeatures(features)) {
        Serial.println("Error reading AS7341 channels");
        delay(50);
        return;
    }

    Serial.println(BallClassifier::classifyBallColorOrUnknown(features));
}
