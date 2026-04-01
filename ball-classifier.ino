
#include <Adafruit_AS7341.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobot_VL6180X.h>
#include <Servo.h>

#include "classification.h"

namespace {

constexpr uint8_t kDCMotorPin = 3;
constexpr uint8_t kDCMotorTargetVoltage = 4;  // 4 Volts over the 5 maximum
constexpr size_t kRawChannelCount = 12;
constexpr uint8_t kClassifierChannelIndexes[BallClassifier::kFeatureCount] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11};
constexpr uint8_t kNeoPixelPin = 1;
constexpr uint8_t kNeoPixelCount = 24;
constexpr uint8_t kNeoPixelBrightness = 5;
constexpr uint8_t kServoPin = 2;
constexpr uint8_t kServoRestAngle = 0;
constexpr uint8_t kServoTriggerAngle = 90;
constexpr uint8_t kToFBallThresholdMm = 18;
constexpr unsigned long kToFDebugIntervalMs = 100;

Adafruit_AS7341 as7341;
Adafruit_NeoPixel strip(kNeoPixelCount, kNeoPixelPin, NEO_GRB + NEO_KHZ800);
DFRobot_VL6180X tofSensor;
Servo sorterServo;

bool wasToFBallPresent = false;
bool servoShouldTriggerOnNextBall = false;
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

void showDetectedColor(const char* label) {
    const uint32_t color = colorForLabel(label);
    for (uint16_t pixel = 0; pixel < strip.numPixels(); ++pixel) {
        strip.setPixelColor(pixel, color);
    }
    strip.show();
}

bool tofSeesBall(uint8_t* measuredDistance = nullptr) {
    const uint8_t range = tofSensor.rangePollMeasurement();
    const uint8_t status = tofSensor.getRangeResult();
    if (status != VL6180X_NO_ERR) {
        return false;
    }

    if (measuredDistance != nullptr) {
        *measuredDistance = range;
    }
    return range < kToFBallThresholdMm;
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

    while (!(tofSensor.begin())) {
        Serial.println("Could not find VL6180X");
        delay(1000);
    }

    while (!as7341.begin(57, &Wire1)) {
        Serial.println("Could not find AS7341");
        delay(1000);
    }

    as7341.setATIME(100);
    as7341.setASTEP(100);
    as7341.setGain(AS7341_GAIN_256X);
    as7341.setLEDCurrent(5);
    as7341.enableLED(true);

    Wire.setWireTimeout(2500, true);

    Serial.println("AS7341 classifier ready");

    analogWriteResolution(8);
}

void loop() {
    analogWrite(kDCMotorPin, (kDCMotorTargetVoltage * 255) / 5);
    uint16_t features[BallClassifier::kFeatureCount];
    unsigned long startReadingTime = millis();
    if (!readClassifierFeatures(features)) {
        Serial.println("Error reading AS7341 channels");
        delay(50);
        return;
    }
    unsigned long endReadingTime = millis();

    unsigned long startClassificationTime = micros();
    const BallClassifier::PredictionResult prediction = BallClassifier::classifyBallColorDetailed(features);
    unsigned long endClassificationTime = micros();
    printPrediction(prediction);

    static const char* lastLabel = nullptr;
    // const char* currentLabel = prediction.label;
    const char* currentLabel = prediction.closestKnownColor;
    if (currentLabel != lastLabel) {
        showDetectedColor(currentLabel);
        lastLabel = currentLabel;
    }

    // servoShouldTriggerOnNextBall = !prediction.isUnknown && labelsEqual(prediction.closestKnownColor, "red");
    servoShouldTriggerOnNextBall = labelsEqual(prediction.closestKnownColor, "red");
    uint8_t tofDistance = 0;
    const bool tofBallPresent = tofSeesBall(&tofDistance);
    if (tofBallPresent && !wasToFBallPresent) {
        Serial.println(
            String("ToF rising edge at ") + String(tofDistance) +
            "mm, setting servo " +
            (servoShouldTriggerOnNextBall ? String("to 90") : String("to 0")) +
            " degrees");
        setSorterServo(servoShouldTriggerOnNextBall);
    }
    wasToFBallPresent = tofBallPresent;
}
