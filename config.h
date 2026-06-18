#pragma once

#include <stddef.h>

namespace BallClassifier {

/**
 * @brief Number of input features consumed by the color classifier.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr size_t kFeatureCount = 10;

/**
 * @brief Number of known classes handled by the color classifier.
 * @author David Goletta
 * @date 2026-06-18
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

}  // namespace BallClassifier