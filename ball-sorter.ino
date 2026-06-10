#include <Adafruit_AS7341.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TiCoServo.h>
#include <DFRobot_VL6180X.h>

#include "classification.h"

// Pins
constexpr uint8_t kDCMotorPin = 3;
constexpr uint8_t kRotateBarrel = 2;
constexpr uint8_t kNeoPixelPin = 7;
constexpr uint8_t kTopToFSensorCEPin = 6;
constexpr uint8_t kBottomToFSensorCEPin = 5;
constexpr uint8_t kServoPin = 9;
constexpr uint8_t kSpiChipSelectPin = 8;
constexpr uint8_t kEmergencyStopPin = 4;
constexpr uint8_t kAlignBarrel = 10;

// Constants
constexpr uint8_t kDCMotorTargetVoltage = 3;
constexpr size_t kRawChannelCount = 12;
constexpr uint8_t kClassifierChannelIndexes[BallClassifier::kFeatureCount] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11};
constexpr uint8_t kNeoPixelCount = 24;
constexpr uint8_t kNeoPixelBrightness = 15;
constexpr uint8_t kGoodBallServoAngle = 55;
constexpr uint8_t kRedBallServoAngle = 105;
constexpr uint8_t kToFBallThresholdMm = 40;
constexpr unsigned long kToFDebugIntervalMs = 100;
constexpr unsigned long kErrorAnimationStepMs = 100;
constexpr uint8_t kErrorTrailCount = 3;
constexpr uint8_t kErrorTrailLength = 5;
constexpr uint8_t kMaxBallCount = 18;
constexpr unsigned long kMotorKickMs = 10;  // milliseconds to run full speed on start
constexpr uint8_t kServoEngageDelayMs = 250;
constexpr float kUnknownBallThreshold = 0.35f;

bool sensorErrorState = false;

Adafruit_AS7341 as7341;
Adafruit_NeoPixel strip(kNeoPixelCount, kNeoPixelPin, NEO_GRB + NEO_KHZ800);
DFRobot_VL6180X bottomTofSensor;
DFRobot_VL6180X topTofSensor;
Adafruit_TiCoServo sorterServo;

bool bottomLastBallPresent = false;
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
    const uint8_t targetAngle = engaged ? kRedBallServoAngle : kGoodBallServoAngle;
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

void pixelErrorAnimation() {
    static uint16_t currentPixel = 0;
    static unsigned long lastUpdateTime = 0;

    const unsigned long now = millis();
    if (now - lastUpdateTime < kErrorAnimationStepMs) {
        return;
    }

    lastUpdateTime = now;
    for (uint16_t pixel = 0; pixel < strip.numPixels(); ++pixel) {
        strip.setPixelColor(pixel, 0);
    }

    const uint16_t pixelCount = strip.numPixels();
    uint8_t pixelBrightness[kNeoPixelCount] = {};

    for (uint8_t trailIndex = 0; trailIndex < kErrorTrailCount; ++trailIndex) {
        const uint16_t trailHead = static_cast<uint16_t>((currentPixel + ((trailIndex * pixelCount) / kErrorTrailCount)) % pixelCount);
        for (uint8_t trailStep = 0; trailStep < kErrorTrailLength && trailStep < pixelCount; ++trailStep) {
            const uint16_t pixel = (trailHead + pixelCount - trailStep) % pixelCount;
            const uint8_t brightnessStep = static_cast<uint8_t>(255 / kErrorTrailLength);
            const uint8_t brightness = static_cast<uint8_t>(255 - (trailStep * brightnessStep));
            pixelBrightness[pixel] = max(pixelBrightness[pixel], brightness);
        }
    }

    for (uint16_t pixel = 0; pixel < pixelCount; ++pixel) {
        strip.setPixelColor(pixel, strip.Color(pixelBrightness[pixel], 0, 0));
    }

    strip.show();
    currentPixel = (currentPixel + 1) % pixelCount;
}

void printDistances(uint8_t topDistance, int8_t bottomDistance) {
    static unsigned long lastPrintTime = 0;
    const unsigned long now = millis();
    if (now - lastPrintTime < kToFDebugIntervalMs) {
        return;
    }
    lastPrintTime = now;
    Serial.println("Top ToF distance: " + String(topDistance) + "/" + String(kToFBallThresholdMm) + "mm, Bottom ToF distance: " + String(bottomDistance) + "/" + String(kToFBallThresholdMm) + "mm");
}

// Drive the DC motor. When `enable` transitions false->true, perform a short full-speed
// kick to help the motor start, then fall back to target PWM.
void driveMotor(bool enable) {
    static bool motorWasRunning = false;
    static bool motorStarting = false;
    static unsigned long motorStartTime = 0;

    if (enable) {
        if (!motorWasRunning && !motorStarting) {
            // beginning of start: kick motor at full speed
            motorStarting = true;
            motorStartTime = millis();
            analogWrite(kDCMotorPin, 255);
            return;
        }

        if (motorStarting) {
            if (millis() - motorStartTime >= kMotorKickMs) {
                motorStarting = false;
                motorWasRunning = true;
                analogWrite(kDCMotorPin, (kDCMotorTargetVoltage * 255) / 5);
            } else {
                // still in kick period; keep full PWM
                analogWrite(kDCMotorPin, 255);
            }
            return;
        }

        // already running normally
        motorWasRunning = true;
        analogWrite(kDCMotorPin, (kDCMotorTargetVoltage * 255) / 5);
    } else {
        // stop motor immediately and reset states
        analogWrite(kDCMotorPin, 0);
        motorWasRunning = false;
        motorStarting = false;
    }
}

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
    pinMode(kServoPin, OUTPUT);
    sorterServo.attach(kServoPin);
    currentServoAngle = 0;
    setSorterServo(false);

    pinMode(kDCMotorPin, OUTPUT);
    pinMode(kRotateBarrel, INPUT);
    pinMode(kNeoPixelPin, OUTPUT);
    pinMode(kSpiChipSelectPin, OUTPUT);
    pinMode(kEmergencyStopPin, INPUT);
    pinMode(kAlignBarrel, INPUT);

    // Turn off the top sensor to change address of the bottom sensor without conflicts, then turn it back on at the end of setup
    pinMode(kTopToFSensorCEPin, OUTPUT);
    pinMode(kBottomToFSensorCEPin, OUTPUT);
    digitalWrite(kTopToFSensorCEPin, LOW);
    digitalWrite(kBottomToFSensorCEPin, HIGH);
    delay(100);

    while (!(bottomTofSensor.begin())) {
        Serial.println("Could not find bottom VL6180X");
        pixelErrorAnimation();
    }
    bottomTofSensor.setIICAddr(0x30);

    // Turn back on the top sensor and initialize it, now that the bottom sensor has a different address
    digitalWrite(kTopToFSensorCEPin, HIGH);
    delay(100);
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

void loop() {
    static uint8_t ballCount = 0;
    static bool topLastBallPresent = false;
    static bool bottomLastBallPresent = false;
    static bool colorSensorLastBallPresent = false;
    static bool ballCountWasAtMax = false;
    static bool motorWasRunning = false;
    static bool motorStarting = false;
    static unsigned long motorStartTime = 0;
    static bool ESTOPEngaged = false;
    uint8_t topTofDistance = 0;

    if (digitalRead(kEmergencyStopPin) == HIGH) {
        driveMotor(false);
        pixelErrorAnimation();
        ESTOPEngaged = true;
        return;
    }

    if (ESTOPEngaged) {
        // Build a fake ball prediction in greed at full confidence to update the LEDs
        BallClassifier::PredictionResult fakePrediction = {};
        fakePrediction.label = "unknown";
        fakePrediction.closestKnownColor = "green";
        fakePrediction.confidence = 1.0f;
        fakePrediction.distance = 0.0f;
        fakePrediction.isUnknown = true;
        showDetectedColor(fakePrediction);
        ESTOPEngaged = false;
    }

    const bool topBallPresent = tofSeesBall(&topTofSensor, kToFBallThresholdMm, &topTofDistance);
    const bool shouldMotorRun = (topBallPresent && ballCount < kMaxBallCount) || digitalRead(kRotateBarrel) == HIGH;

    driveMotor(shouldMotorRun);
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

    const bool colorSensorBallPresent = prediction.confidence > kUnknownBallThreshold;
    const bool colorSensorSeesRedBall = labelsEqual(prediction.closestKnownColor, "red");
    uint8_t bottomTofDistance = 0;
    const bool bottomBallPresent = tofSeesBall(&bottomTofSensor, kToFBallThresholdMm, &bottomTofDistance);

    // Top rising edge
    if (topBallPresent && !topLastBallPresent) {
        ballCount = min(ballCount + 1, kMaxBallCount);
        Serial.println("Ball count: " + String(ballCount));
    }

    const bool ballCountJustReachedMax = ballCount == kMaxBallCount && !ballCountWasAtMax;

    // Color sensor rising edge
    static bool firstBallSeen = false;
    static unsigned long firstBallSeenTime = 0;
    if (colorSensorBallPresent && !colorSensorLastBallPresent) {
        Serial.println("First ball detected by color sensor");
        firstBallSeenTime = millis();
        if (!firstBallSeen && colorSensorSeesRedBall) setSorterServo(true);
        firstBallSeen = true;
    }

    // Reset firstBallSeen if no ball seen for 5 seconds
    if (firstBallSeen && !colorSensorBallPresent && (millis() - firstBallSeenTime > 750)) {
        firstBallSeen = false;
        Serial.println("Resetting first ball seen state after timeout");
    }

    // Bottom rising edge
    if ((bottomBallPresent && !bottomLastBallPresent) || ballCountJustReachedMax) {
        tofSensorSlotHasRedBall = colorSensorSlotHasRedBall;
        colorSensorSlotHasRedBall = colorSensorSeesRedBall;
    }

    // Bottom falling edge
    if (!bottomBallPresent && bottomLastBallPresent) {
        ballCount = max(0, ballCount - 1);
        Serial.println("Ball count: " + String(ballCount));
        delay(kServoEngageDelayMs);
        if (tofSensorSlotHasRedBall) {
            setSorterServo(false);
            delay(kServoEngageDelayMs / 2);
        }
        setSorterServo(colorSensorSlotHasRedBall);
    }

    // colorSensorLastBallPresent = colorSensorBallPresent;
    topLastBallPresent = topBallPresent;
    bottomLastBallPresent = bottomBallPresent;
    ballCountWasAtMax = ballCount == kMaxBallCount;
    // printDistances(topTofDistance, bottomTofDistance);
}
