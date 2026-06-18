#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

namespace BallClassifier {

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