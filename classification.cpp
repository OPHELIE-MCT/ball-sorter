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
    {0.02471679f, 0.05792461f, 0.09417178f, 0.13419147f, 0.21128522f, 0.33761907f, 0.31582336f, 0.17493438f, 0.81449222f, 0.12926551f},
    {0.03330838f, 0.17555412f, 0.20551367f, 0.14268240f, 0.16079400f, 0.18252489f, 0.22742041f, 0.23741927f, 0.83719416f, 0.19315491f},
    {0.02962298f, 0.19219952f, 0.26372702f, 0.23007654f, 0.20131277f, 0.15574808f, 0.16283440f, 0.11020639f, 0.84384747f, 0.15537278f},
    {0.02584480f, 0.07519508f, 0.15438012f, 0.29884138f, 0.28781175f, 0.20452919f, 0.18271240f, 0.12163016f, 0.83048283f, 0.13362884f},
    {0.02512496f, 0.06194002f, 0.11142278f, 0.20896040f, 0.25200360f, 0.27065723f, 0.26372346f, 0.16028157f, 0.83081137f, 0.13109795f},
    {0.02632298f, 0.11293832f, 0.12699069f, 0.09264121f, 0.12411866f, 0.28161966f, 0.36198505f, 0.20892863f, 0.82092214f, 0.13577263f},
    {0.02975261f, 0.07025229f, 0.09586762f, 0.08774140f, 0.12762836f, 0.26919229f, 0.39895494f, 0.26458829f, 0.78714126f, 0.19916887f},
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

float computeConfidence(float distance) {
    if (distance < 0.0f) {
        return 0.0f;
    }

    if (kUnknownThreshold <= 0.0f) {
        return 0.0f;
    }

    return 1.0f / (1.0f + (distance / kUnknownThreshold));
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

    const bool is_unknown = distance > kUnknownThreshold;
    return {
        is_unknown ? "unknown" : kClassNames[best_index],
        kClassNames[best_index],
        computeConfidence(distance),
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