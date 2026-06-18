#pragma once

#include <stddef.h>
#include <stdint.h>

namespace BallClassifier {

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
    {0.02176572f, 0.05861964f, 0.10047679f, 0.14750951f, 0.23121250f, 0.34431710f, 0.31493234f, 0.17167382f, 0.81148065f, 0.07118026f},
    {0.02719150f, 0.19254453f, 0.22986130f, 0.15292251f, 0.17360606f, 0.20127188f, 0.24925770f, 0.23367468f, 0.83320580f, 0.06595952f},
    {0.02427890f, 0.20775988f, 0.28599793f, 0.24360660f, 0.20450618f, 0.15539665f, 0.16185104f, 0.10542755f, 0.84196078f, 0.05410646f},
    {0.02226513f, 0.07439216f, 0.15796030f, 0.31929674f, 0.29952163f, 0.20818717f, 0.18149197f, 0.11650031f, 0.82684755f, 0.06184162f},
    {0.02180023f, 0.06085411f, 0.11377366f, 0.21952345f, 0.26174571f, 0.28215734f, 0.27063519f, 0.15831067f, 0.82679356f, 0.06833400f},
    {0.02287789f, 0.12235501f, 0.13977485f, 0.09743671f, 0.13304624f, 0.29787000f, 0.36965194f, 0.20599983f, 0.81553970f, 0.06973832f},
    {0.02466214f, 0.07120280f, 0.10178273f, 0.08696458f, 0.13598036f, 0.30313808f, 0.43909980f, 0.27928814f, 0.76763695f, 0.07702627f}};

/**
 * @brief Global inner radius that maps to 100% confidence.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr float kInnerConfidenceRadius = 0.00202114f;

/**
 * @brief Class-specific outer radii where confidence falls to 0%.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Each entry shares the index of the corresponding `kClassNames` centroid.
 */
constexpr float kOuterConfidenceRadii[kClassCount] = {0.05600741f, 0.09871508f, 0.09871508f, 0.08474379f, 0.05600741f, 0.06471307f, 0.06471307f};

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