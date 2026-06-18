#pragma once

#include <stddef.h>
#include <stdint.h>

namespace BallClassifier {

/**
 * @brief Number of sensor features expected by the classifier.
 * @author David Goletta
 * @date 2026-03-25
 *
 * Every prediction function in this module expects a 10-dimensional input
 * vector that matches the AS7341 feature layout used during training.
 */
constexpr size_t kFeatureCount = 10;

/**
 * @brief Number of known ball colors embedded in the classifier.
 * @author David Goletta
 * @date 2026-03-25
 *
 * Every prediction function in this module expects a 10-dimensional input
 * vector that matches the AS7341 feature layout used during training.
 */
constexpr size_t kClassCount = 7;

/**
 * @brief Canonical class labels aligned with the trained centroid table.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr const char* kClassNames[kClassCount] = {
    "orange",
    "purple",
    "blue",
    "green",
    "yellow",
    "pink",
    "red",
};

/**
 * @brief Trained normalized centroids for the supported ball colors.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Rows follow `kClassNames` and columns follow the 10-feature AS7341 layout.
 */
constexpr float kClassCentroids[kClassCount][kFeatureCount] = {
    {0.20083515f, 0.30414975f, 0.28179389f, 0.15131361f, 0.20083393f, 0.30414524f, 0.28179360f, 0.15132641f, 0.72484634f, 0.06561799f},
    {0.16719841f, 0.18881229f, 0.23751035f, 0.23465544f, 0.16724038f, 0.18885139f, 0.23757185f, 0.23471157f, 0.80318034f, 0.06823966f},
    {0.21392725f, 0.16034218f, 0.16942125f, 0.10766981f, 0.21388693f, 0.16031664f, 0.16937882f, 0.10764657f, 0.87920859f, 0.05911622f},
    {0.28745819f, 0.20848741f, 0.18949513f, 0.11843721f, 0.28744831f, 0.20848867f, 0.18948690f, 0.11844186f, 0.80248859f, 0.06283615f},
    {0.24089926f, 0.26075535f, 0.25102525f, 0.14293452f, 0.24091285f, 0.26078788f, 0.25105034f, 0.14294608f, 0.75955827f, 0.06396537f},
    {0.12905555f, 0.27402219f, 0.32322213f, 0.17721341f, 0.12906940f, 0.27400238f, 0.32321057f, 0.17721187f, 0.73531367f, 0.06392914f},
    {0.12004440f, 0.25922963f, 0.37522778f, 0.23220169f, 0.12005179f, 0.25922911f, 0.37519164f, 0.23222431f, 0.66509677f, 0.07079028f}};

/**
 * @brief Global inner radius that maps to 100% confidence.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr float kInnerConfidenceRadius = 0.00157743f;

/**
 * @brief Class-specific outer radii where confidence falls to 0%.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Each entry shares the index of the corresponding `kClassNames` centroid.
 */
constexpr float kOuterConfidenceRadii[kClassCount] = {0.05053299f, 0.10041068f, 0.07482672f, 0.07145156f, 0.05053299f, 0.06519904f, 0.06525675f};

/**
 * @brief Detailed classification output.
 * @author David Goletta
 * @date 2026-03-25
 *
 * `label` contains the returned classifier label, which may be `unknown`.
 * `closestKnownColor` always contains the nearest known class.
 * `confidence` is a distance-based score in the range `[0, 1]` where higher is
 * better. Values at or inside the inner confidence radius are reported as
 * `1.0`, then decrease linearly to `0.0` at the class-specific outer
 * confidence radius.
 */
struct PredictionResult {
    const char* label;
    const char* closestKnownColor;
    float confidence;
    float distance;
    const char* secondClosestKnownColor;
    float secondDistance;
    bool isUnknown;
};

/**
 * @brief Predict the closest known ball color from a floating-point feature vector.
 * @author David Goletta
 * @date 2026-03-25
 *
 * The input vector is normalized internally before classification, so raw sensor
 * magnitudes may be passed directly as long as the feature order matches the
 * model training data.
 *
 * @param input A 10-dimensional feature vector.
 * @return One of `orange`, `purple`, `blue`, `green`, `yellow`, `pink`, `red`,
 *         or `unknown` if the input cannot be normalized.
 */
const char* classifyBallColor(const float input[kFeatureCount]);

/**
 * @brief Predict the closest known ball color from a 16-bit integer feature vector.
 * @author David Goletta
 * @date 2026-03-25
 *
 * This overload is convenient for Arduino sketches that store AS7341 readings in
 * integer arrays.
 *
 * @param input A 10-dimensional feature vector.
 * @return One of `orange`, `purple`, `blue`, `green`, `yellow`, `pink`, `red`,
 *         or `unknown` if the input cannot be normalized.
 */
const char* classifyBallColor(const uint16_t input[kFeatureCount]);

/**
 * @brief Predict the ball color and reject outliers as `unknown`.
 * @author David Goletta
 * @date 2026-03-25
 *
 * The input is normalized internally and compared against the stored class
 * centroids. If the closest centroid is farther than its class-specific outer
 * confidence radius, the function returns `unknown` instead of forcing a known
 * label.
 *
 * @param input A 10-dimensional feature vector.
 * @return One of `orange`, `purple`, `blue`, `green`, `yellow`, `pink`, `red`,
 *         or `unknown`.
 */
const char* classifyBallColorOrUnknown(const float input[kFeatureCount]);

/**
 * @brief Integer-input overload of the unknown-aware classifier.
 * @author David Goletta
 * @date 2026-03-25
 *
 * @param input A 10-dimensional feature vector.
 * @return One of `orange`, `purple`, `blue`, `green`, `yellow`, `pink`, `red`,
 *         or `unknown`.
 */
const char* classifyBallColorOrUnknown(const uint16_t input[kFeatureCount]);

/**
 * @brief Return detailed classification information for a floating-point input.
 * @author David Goletta
 * @date 2026-06-18
 *
 * @param input A 10-dimensional feature vector.
 * @return Detailed prediction data including the nearest known color,
 *         second-nearest known color, both distances, confidence score, and
 *         whether the sample was rejected as `unknown`.
 */
PredictionResult classifyBallColorDetailed(const float input[kFeatureCount]);

/**
 * @brief Return detailed classification information for a 16-bit integer input.
 * @author David Goletta
 * @date 2026-06-18
 *
 * @param input A 10-dimensional feature vector.
 * @return Detailed prediction data including the nearest known color,
 *         second-nearest known color, both distances, confidence score, and
 *         whether the sample was rejected as `unknown`.
 */
PredictionResult classifyBallColorDetailed(const uint16_t input[kFeatureCount]);

}  // namespace BallClassifier