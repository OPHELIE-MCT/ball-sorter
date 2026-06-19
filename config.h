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
    {0.02571996f, 0.05558182f, 0.08985539f, 0.12684258f, 0.19457753f, 0.33001336f, 0.31572317f, 0.17730208f, 0.81586525f, 0.16900504f},
    {0.03767006f, 0.16597840f, 0.20504474f, 0.14480785f, 0.16069140f, 0.17830906f, 0.22106529f, 0.22249336f, 0.82295215f, 0.27342528f},
    {0.03225281f, 0.18288436f, 0.25697871f, 0.22612363f, 0.19387617f, 0.15067985f, 0.15867614f, 0.11070326f, 0.83933887f, 0.21477118f},
    {0.02974796f, 0.07994235f, 0.15954077f, 0.29033299f, 0.28103707f, 0.20431882f, 0.18835005f, 0.12930099f, 0.81755013f, 0.20407707f},
    {0.02653280f, 0.06135845f, 0.10973079f, 0.20550641f, 0.24505144f, 0.26864980f, 0.26051484f, 0.16178934f, 0.82842669f, 0.16940991f},
    {0.02825247f, 0.11026300f, 0.12715154f, 0.09380945f, 0.11972057f, 0.26438629f, 0.34978859f, 0.21159681f, 0.82280725f, 0.18359122f},
    {0.03352903f, 0.07436671f, 0.10289470f, 0.09315729f, 0.12993008f, 0.25155929f, 0.36280190f, 0.25266635f, 0.78520158f, 0.28619185f}};

/**
 * @brief Global inner radius that maps to 100% confidence.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr float kInnerConfidenceRadius = 0.00492303f;

/**
 * @brief Class-specific outer radii where confidence falls to 0%.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Each entry shares the index of the corresponding `kClassNames` centroid.
 */
constexpr float kOuterConfidenceRadii[kClassCount] = {0.06398578f, 0.08886425f, 0.08886425f, 0.07590962f, 0.06398578f, 0.06318841f, 0.06318841f};

}  // namespace BallClassifier