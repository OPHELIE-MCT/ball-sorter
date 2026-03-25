#include "classification.h"

#include <math.h>

#if !defined(ARDUINO)
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>
using std::cerr;
using std::cout;
using std::endl;
#endif

namespace BallClassifier {

constexpr const char* kClassNames[kClassCount] = {
    "orange",
    "purple",
    "blue",
    "green",
    "yellow",
    "pink",
    "red",
};

constexpr float kClassCentroids[kClassCount][kFeatureCount] = {
    {0.03345186f, 0.06443642f, 0.11018858f, 0.15927181f, 0.24699159f, 0.41288048f, 0.40755482f, 0.23557218f, 0.69032972f, 0.16546870f},
    {0.03331396f, 0.16402638f, 0.19694636f, 0.13951787f, 0.14536540f, 0.17069034f, 0.21331795f, 0.24569738f, 0.85664309f, 0.15336730f},
    {0.02361696f, 0.16460909f, 0.25607176f, 0.20692375f, 0.17004769f, 0.12111093f, 0.12636331f, 0.09358379f, 0.88517465f, 0.10753922f},
    {0.02227928f, 0.06136749f, 0.13538175f, 0.29839583f, 0.27594141f, 0.17964997f, 0.15522270f, 0.10862595f, 0.85503151f, 0.10799595f},
    {0.02389210f, 0.05851387f, 0.11042215f, 0.21294934f, 0.24649723f, 0.25616601f, 0.25444123f, 0.16426991f, 0.83849450f, 0.11980524f},
    {0.02694435f, 0.11844697f, 0.14965602f, 0.10187495f, 0.12837095f, 0.28060895f, 0.37585415f, 0.22685230f, 0.80306144f, 0.13746750f},
    {0.02230814f, 0.04583629f, 0.06554801f, 0.05692507f, 0.08495678f, 0.19977754f, 0.35467541f, 0.25824000f, 0.85585423f, 0.13315284f},
};

bool normalizeVector(const float input[kFeatureCount], float output[kFeatureCount]) {
    float squared_norm = 0.0f;
    for (size_t index = 0; index < kFeatureCount; ++index) {
        squared_norm += input[index] * input[index];
    }

    if (squared_norm <= 0.0f) {
        for (size_t index = 0; index < kFeatureCount; ++index) {
            output[index] = 0.0f;
        }
        return false;
    }

    const float inverse_norm = 1.0f / sqrtf(squared_norm);
    for (size_t index = 0; index < kFeatureCount; ++index) {
        output[index] = input[index] * inverse_norm;
    }
    return true;
}

int findNearestClassIndex(const float input[kFeatureCount], float* outDistance = nullptr) {
    float normalized[kFeatureCount];
    if (!normalizeVector(input, normalized)) {
        if (outDistance != nullptr) {
            *outDistance = INFINITY;
        }
        return -1;
    }

    int best_index = -1;
    float best_distance_sq = INFINITY;

    for (size_t class_index = 0; class_index < kClassCount; ++class_index) {
        float distance_sq = 0.0f;
        for (size_t feature_index = 0; feature_index < kFeatureCount; ++feature_index) {
            const float delta = normalized[feature_index] - kClassCentroids[class_index][feature_index];
            distance_sq += delta * delta;
        }

        if (distance_sq < best_distance_sq) {
            best_distance_sq = distance_sq;
            best_index = static_cast<int>(class_index);
        }
    }

    if (outDistance != nullptr) {
        *outDistance = sqrtf(best_distance_sq);
    }
    return best_index;
}

const char* classifyBallColor(const float input[kFeatureCount]) {
    const int class_index = findNearestClassIndex(input);
    if (class_index < 0) {
        return "unknown";
    }
    return kClassNames[class_index];
}

const char* classifyBallColorOrUnknown(const float input[kFeatureCount]) {
    float distance = 0.0f;
    const int class_index = findNearestClassIndex(input, &distance);
    if (class_index < 0 || distance > kUnknownThreshold) {
        return "unknown";
    }
    return kClassNames[class_index];
}

const char* classifyBallColor(const uint16_t input[kFeatureCount]) {
    float converted[kFeatureCount];
    for (size_t index = 0; index < kFeatureCount; ++index) {
        converted[index] = static_cast<float>(input[index]);
    }
    return classifyBallColor(converted);
}

const char* classifyBallColorOrUnknown(const uint16_t input[kFeatureCount]) {
    float converted[kFeatureCount];
    for (size_t index = 0; index < kFeatureCount; ++index) {
        converted[index] = static_cast<float>(input[index]);
    }
    return classifyBallColorOrUnknown(converted);
}

}  // namespace BallClassifier

#if !defined(ARDUINO)

namespace {

void skipSpaces(const char*& cursor) {
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }
}

bool parseFeatureVector(const char* text, float output[BallClassifier::kFeatureCount]) {
    if (text == nullptr) {
        return false;
    }

    const char* cursor = text;
    skipSpaces(cursor);
    if (*cursor != '[') {
        return false;
    }
    ++cursor;

    for (size_t index = 0; index < BallClassifier::kFeatureCount; ++index) {
        skipSpaces(cursor);

        char* end = nullptr;
        errno = 0;
        const float value = std::strtof(cursor, &end);
        if (end == cursor || errno == ERANGE) {
            return false;
        }
        output[index] = value;
        cursor = end;

        skipSpaces(cursor);
        if (index + 1 < BallClassifier::kFeatureCount) {
            if (*cursor != ',') {
                return false;
            }
            ++cursor;
        } else {
            if (*cursor != ']') {
                return false;
            }
            ++cursor;
        }
    }

    skipSpaces(cursor);
    return *cursor == '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: classification \"[221, 453, 650, 564, 842, 1982, 3522, 2562, 8504, 1324]\"" << endl;
        return 1;
    }

    float features[BallClassifier::kFeatureCount];
    if (!parseFeatureVector(argv[1], features)) {
        cerr << "Invalid input vector. Expected exactly 10 numeric values in bracket notation." << endl;
        return 2;
    }

    const char* color = BallClassifier::classifyBallColorOrUnknown(features);
    cout << color << endl;
    return 0;
}

#endif