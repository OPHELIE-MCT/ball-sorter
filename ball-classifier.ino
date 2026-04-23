
#include <Adafruit_AS7341.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TiCoServo.h>
#include <DFRobot_VL6180X.h>

#include "classification.h"

namespace {

// Pins
constexpr uint8_t kDCMotorPin = 3;
constexpr uint8_t kForceRotationPin = 4;
constexpr uint8_t kNeoPixelPin = 6;
constexpr uint8_t kServoPin = 9;
constexpr uint8_t kBottomToFSensorCEPin = 7;
constexpr uint8_t kTopToFSensorCEPin = 8;

// Constants
constexpr uint8_t kDCMotorTargetVoltage = 4;  // 4 Volts over the 5 maximum
constexpr size_t kRawChannelCount = 12;
constexpr uint8_t kClassifierChannelIndexes[BallClassifier::kFeatureCount] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11};
constexpr uint8_t kNeoPixelCount = 24;
constexpr uint8_t kNeoPixelBrightness = 5;
constexpr uint8_t kServoRestAngle = 0;
constexpr uint8_t kServoTriggerAngle = 90;
constexpr uint8_t kToFBallThresholdMm = 18;
constexpr unsigned long kToFDebugIntervalMs = 100;
constexpr unsigned long kErrorAnimationStepMs = 50;

bool sensorErrorState = false;

Adafruit_AS7341 as7341;
Adafruit_NeoPixel strip(kNeoPixelCount, kNeoPixelPin, NEO_GRB + NEO_KHZ800);
DFRobot_VL6180X bottomTofSensor;
DFRobot_VL6180X topTofSensor;
Adafruit_TiCoServo sorterServo;

bool wasToFBallPresent = false;
bool colorSensorSlotHasRedBall = false;
bool tofSensorSlotHasRedBall = false;
uint8_t currentServoAngle = 255;

bool labelsEqual(const char* left, const char* right) {
    return left != nullptr && right != nullptr && strcmp(left, right) == 0;
}

uint32_t colorForLabel(const char* label) {
    if (label == nullptr) {
        return strip.Color(0, 0, 0);
    }

    if (labelsEqual(label, "orange")) {
        return strip.Color(255, 96, 0);
    }
    if (labelsEqual(label, "purple")) {
        return strip.Color(128, 0, 128);
    }
    if (labelsEqual(label, "blue")) {
        return strip.Color(0, 0, 255);
    }
    if (labelsEqual(label, "green")) {
        return strip.Color(0, 255, 0);
    }
    if (labelsEqual(label, "yellow")) {
        return strip.Color(255, 180, 0);
    }
    if (labelsEqual(label, "pink")) {
        return strip.Color(255, 20, 147);
    }
    if (labelsEqual(label, "red")) {
        return strip.Color(255, 0, 0);
    }

    return strip.Color(0, 0, 0);
}

void showDetectedColor(const BallClassifier::PredictionResult& prediction) {
    static const char* lastLabel = nullptr;
    static uint16_t lastLitPixelCount = UINT16_MAX;

    const char* label = prediction.closestKnownColor;
    const long confidencePercent = constrain(static_cast<long>(prediction.confidence * 100.0f), 0L, 100L);
    const uint16_t litPixelCount = static_cast<uint16_t>(map(confidencePercent, 0, 100, 0, strip.numPixels()));

    if (labelsEqual(label, lastLabel) && litPixelCount == lastLitPixelCount) {
        return;
    }

    const uint32_t color = colorForLabel(label);
    for (uint16_t pixel = 0; pixel < strip.numPixels(); ++pixel) {
        strip.setPixelColor(pixel, pixel < litPixelCount ? color : 0);
    }
    strip.show();

    lastLabel = label;
    lastLitPixelCount = litPixelCount;
}

bool tofSeesBall(DFRobot_VL6180X* sensor = nullptr, uint8_t threshold = kToFBallThresholdMm, uint8_t* measuredDistance = nullptr) {
    if (sensor == nullptr) {
        sensor = &bottomTofSensor;
    }
    const uint8_t range = sensor->rangePollMeasurement();
    const uint8_t status = sensor->getRangeResult();
    if (status != VL6180X_NO_ERR) {
        return false;
    }

    if (measuredDistance != nullptr) {
        *measuredDistance = range;
    }
    return range < threshold;
}

void setSorterServo(bool engaged) {
    const uint8_t targetAngle = engaged ? kServoTriggerAngle : kServoRestAngle;
    if (targetAngle == currentServoAngle) {
        return;
    }

    currentServoAngle = targetAngle;
    sorterServo.write(targetAngle);
}

void printPrediction(const BallClassifier::PredictionResult& prediction) {
    if (prediction.isUnknown) {
        Serial.print(prediction.label);
        Serial.print(" (");
        Serial.print(prediction.closestKnownColor);
        Serial.print(", confidence: ");
        Serial.print(prediction.confidence, 3);
        Serial.println(')');
        return;
    }

    Serial.print(prediction.label);
    Serial.print(" (confidence: ");
    Serial.print(prediction.confidence, 3);
    Serial.println(')');
}

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

    Wire.begin();

    strip.begin();
    strip.setBrightness(kNeoPixelBrightness);
    strip.clear();
    strip.show();

    sorterServo.attach(kServoPin);
    currentServoAngle = 0;
    setSorterServo(false);

    // Turn off the top sensor to change address of the bottom sensor without conflicts, then turn it back on at the end of setup
    pinMode(kTopToFSensorCEPin, OUTPUT);
    pinMode(kBottomToFSensorCEPin, OUTPUT);
    digitalWrite(kTopToFSensorCEPin, LOW);
    digitalWrite(kBottomToFSensorCEPin, HIGH);

    while (!(bottomTofSensor.begin())) {
        Serial.println("Could not find bottom VL6180X");
        pixelErrorAnimation();
    }
    bottomTofSensor.setIICAddr(0x30);

    // Turn back on the top sensor and initialize it, now that the bottom sensor has a different address
    digitalWrite(kTopToFSensorCEPin, HIGH);
    while (!(topTofSensor.begin())) {
        Serial.println("Could not find top VL6180X");
        pixelErrorAnimation();
    }

    while (!as7341.begin(57, &Wire)) {
        Serial.println("Could not find AS7341");
        pixelErrorAnimation();
    }

    as7341.setATIME(100);
    as7341.setASTEP(100);
    as7341.setGain(AS7341_GAIN_256X);
    as7341.setLEDCurrent(5);
    as7341.enableLED(true);

    Serial.println("AS7341 classifier ready");
}

void pixelErrorAnimation() {
    static uint16_t currentPixel = 0;
    static unsigned long lastUpdateTime = 0;

    const unsigned long now = millis();
    if (now - lastUpdateTime < kErrorAnimationStepMs) {
        return;
    }

    lastUpdateTime = now;
    const uint16_t previousPixel = currentPixel == 0 ? strip.numPixels() - 1 : currentPixel - 1;

    strip.setPixelColor(previousPixel, 0);
    strip.setPixelColor(currentPixel, strip.Color(255, 0, 0));
    strip.show();
    currentPixel = (currentPixel + 1) % strip.numPixels();
}

void loop() {
    if (tofSeesBall(&topTofSensor, kToFBallThresholdMm * 2) || digitalRead(kForceRotationPin) == HIGH) {
        analogWrite(kDCMotorPin, (kDCMotorTargetVoltage * 255) / 5);
    } else {
        analogWrite(kDCMotorPin, 0);
    }
    uint16_t features[BallClassifier::kFeatureCount];
    unsigned long startReadingTime = millis();
    if (!readClassifierFeatures(features)) {
        Serial.println("Error reading AS7341 channels (" + String(millis() - startReadingTime) + " ms)");
        return;
    }
    unsigned long endReadingTime = millis();

    unsigned long startClassificationTime = micros();
    const BallClassifier::PredictionResult prediction = BallClassifier::classifyBallColorDetailed(features);
    unsigned long endClassificationTime = micros();
    // printPrediction(prediction);

    showDetectedColor(prediction);

    const bool colorSensorSeesRedBall = labelsEqual(prediction.closestKnownColor, "red");
    uint8_t tofDistance = 0;
    const bool tofBallPresent = tofSeesBall(&bottomTofSensor, kToFBallThresholdMm, &tofDistance);
    if (!tofBallPresent && wasToFBallPresent) {
        tofSensorSlotHasRedBall = colorSensorSlotHasRedBall;
        colorSensorSlotHasRedBall = colorSensorSeesRedBall;

        Serial.println("ToF rising edge at " + String(tofDistance) + "mm, setting servo " + (tofSensorSlotHasRedBall ? "to 90" : "to 0") + " degrees");
        setSorterServo(tofSensorSlotHasRedBall);
    }
    wasToFBallPresent = tofBallPresent;
}
