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
    {0.02485464f, 0.05582810f, 0.09236680f, 0.13463066f, 0.20100680f, 0.31319980f, 0.29265762f, 0.17655847f, 0.82642251f, 0.17694675f},
    {0.03810011f, 0.15858067f, 0.18984452f, 0.14202604f, 0.14777390f, 0.17127017f, 0.20698898f, 0.23279177f, 0.82553426f, 0.29556069f},
    {0.02949044f, 0.17843679f, 0.25132016f, 0.22212328f, 0.18100213f, 0.14473508f, 0.14789602f, 0.11191461f, 0.84547396f, 0.22771092f},
    {0.02744866f, 0.07764428f, 0.15419356f, 0.27612205f, 0.25754059f, 0.19694158f, 0.17815955f, 0.13130729f, 0.83336843f, 0.21118593f},
    {0.02503051f, 0.05995961f, 0.10932956f, 0.20891824f, 0.23819440f, 0.25796185f, 0.24618197f, 0.16244732f, 0.83673100f, 0.17253090f},
    {0.02647276f, 0.11157275f, 0.12835225f, 0.09286900f, 0.11502894f, 0.25967777f, 0.33317355f, 0.20856046f, 0.83093299f, 0.19007415f},
    {0.03535927f, 0.07394401f, 0.09990940f, 0.09517364f, 0.12151259f, 0.24107488f, 0.34758088f, 0.26256088f, 0.79017964f, 0.29533214f}};

/**
 * @brief Global inner radius that maps to 100% confidence.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr float kInnerConfidenceRadius = 0.01333871f;

/**
 * @brief Class-specific outer radii where confidence falls to 0%.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Each entry shares the index of the corresponding `kClassNames` centroid.
 */
constexpr float kOuterConfidenceRadii[kClassCount] = {0.05644135f, 0.09430786f, 0.09056486f, 0.06717094f, 0.05644135f, 0.06811938f, 0.06811938f};

}  // namespace BallClassifier