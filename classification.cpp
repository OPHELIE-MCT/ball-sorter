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
    {0.02360889f, 0.05300347f, 0.08657849f, 0.12634508f, 0.19329950f, 0.34314761f, 0.31442849f, 0.18024287f, 0.81924418f, 0.12456024f},
    {0.03069155f, 0.18051812f, 0.21095546f, 0.14626800f, 0.15374659f, 0.17934228f, 0.21861491f, 0.23882904f, 0.84617488f, 0.15468594f},
    {0.02694505f, 0.19602302f, 0.26674152f, 0.23537682f, 0.18593376f, 0.14749468f, 0.14893032f, 0.10824493f, 0.85123744f, 0.13943029f},
    {0.02537218f, 0.07588825f, 0.15509339f, 0.30296446f, 0.27374518f, 0.20019189f, 0.17326488f, 0.12381758f, 0.83523571f, 0.14018908f},
    {0.02449014f, 0.06105585f, 0.11044985f, 0.21556981f, 0.24748589f, 0.27048428f, 0.25168555f, 0.16209656f, 0.83491084f, 0.12504171f},
    {0.02521246f, 0.11268176f, 0.12687517f, 0.09341421f, 0.11761071f, 0.27671934f, 0.35021745f, 0.21232543f, 0.82831262f, 0.13224743f},
    {0.02950589f, 0.07082329f, 0.09849352f, 0.09086254f, 0.12196415f, 0.26250907f, 0.38154241f, 0.27160976f, 0.79462696f, 0.20263192f},
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
    float distance = 0.0f;
    const int class_index = findNearestClassIndex(input, &distance);
    if (class_index < 0) {
        return {"unknown", "unknown", 0.0f, distance, true};
    }

    const bool is_unknown = distance > kUnknownThreshold;
    return {
        is_unknown ? "unknown" : kClassNames[class_index],
        kClassNames[class_index],
        computeConfidence(distance),
        distance,
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