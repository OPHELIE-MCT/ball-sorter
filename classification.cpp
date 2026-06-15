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
    {0.02762132f, 0.05708832f, 0.09083026f, 0.12717756f, 0.18756885f, 0.32414027f, 0.30357795f, 0.18190649f, 0.81753467f, 0.19414449f},
    {0.04038878f, 0.15962973f, 0.19003590f, 0.13858044f, 0.13784796f, 0.15709745f, 0.19301812f, 0.21909429f, 0.82386473f, 0.33147492f},
    {0.03471983f, 0.18562622f, 0.25435946f, 0.22345415f, 0.17553088f, 0.13938649f, 0.14646511f, 0.11162463f, 0.83805086f, 0.25144541f},
    {0.03164719f, 0.07985267f, 0.15866718f, 0.28245873f, 0.25811087f, 0.19311825f, 0.17597194f, 0.13279932f, 0.82170828f, 0.24477970f},
    {0.02774926f, 0.06027171f, 0.10684610f, 0.20315530f, 0.23159661f, 0.25766887f, 0.24820025f, 0.16561124f, 0.83537353f, 0.18973450f},
    {0.03159665f, 0.11569036f, 0.13216050f, 0.10013867f, 0.11856438f, 0.25289301f, 0.32484581f, 0.20953952f, 0.82651635f, 0.21805942f},
    {0.03566784f, 0.07328996f, 0.09889431f, 0.09356423f, 0.11404587f, 0.21675596f, 0.33670340f, 0.26230335f, 0.79111326f, 0.32680237f},
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