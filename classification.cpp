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

float classOuterConfidenceRadius(int class_index) {
    if (class_index < 0 || class_index >= static_cast<int>(kClassCount)) {
        return 0.0f;
    }

    return kOuterConfidenceRadii[class_index];
}

float computeConfidence(float distance, int class_index) {
    if (distance < 0.0f) {
        return 0.0f;
    }

    const float outer_confidence_radius = classOuterConfidenceRadius(class_index);
    if (outer_confidence_radius <= 0.0f) {
        return 0.0f;
    }

    if (distance <= kInnerConfidenceRadius) {
        return 1.0f;
    }

    if (distance >= outer_confidence_radius) {
        return 0.0f;
    }

    if (outer_confidence_radius <= kInnerConfidenceRadius) {
        return 0.0f;
    }

    const float normalized_distance =
        (distance - kInnerConfidenceRadius) /
        (outer_confidence_radius - kInnerConfidenceRadius);
    return 1.0f - normalized_distance;
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
    if (class_index < 0 || distance > classOuterConfidenceRadius(class_index)) {
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

PredictionResult classifyBallColorDetailed(const float input[kFeatureCount]) {
    float normalized[kFeatureCount];
    if (!normalizeVector(input, normalized)) {
        return {"unknown", "unknown", 0.0f, INFINITY, "unknown", INFINITY, true};
    }

    int best_index = -1;
    int second_index = -1;
    float best_distance_sq = INFINITY;
    float second_distance_sq = INFINITY;

    for (size_t class_index = 0; class_index < kClassCount; ++class_index) {
        float distance_sq = 0.0f;
        for (size_t feature_index = 0; feature_index < kFeatureCount; ++feature_index) {
            const float delta = normalized[feature_index] - kClassCentroids[class_index][feature_index];
            distance_sq += delta * delta;
        }

        if (distance_sq < best_distance_sq) {
            second_distance_sq = best_distance_sq;
            second_index = best_index;
            best_distance_sq = distance_sq;
            best_index = static_cast<int>(class_index);
        } else if (distance_sq < second_distance_sq) {
            second_distance_sq = distance_sq;
            second_index = static_cast<int>(class_index);
        }
    }

    const float distance = sqrtf(best_distance_sq);
    const float second_distance = sqrtf(second_distance_sq);

    if (best_index < 0) {
        return {"unknown", "unknown", 0.0f, distance, "unknown", second_distance, true};
    }

    const bool is_unknown = distance > classOuterConfidenceRadius(best_index);
    return {
        is_unknown ? "unknown" : kClassNames[best_index],
        kClassNames[best_index],
        computeConfidence(distance, best_index),
        distance,
        second_index >= 0 ? kClassNames[second_index] : "unknown",
        second_distance,
        is_unknown,
    };
}

PredictionResult classifyBallColorDetailed(const uint16_t input[kFeatureCount]) {
    float converted[kFeatureCount];
    for (size_t index = 0; index < kFeatureCount; ++index) {
        converted[index] = static_cast<float>(input[index]);
    }
    return classifyBallColorDetailed(converted);
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