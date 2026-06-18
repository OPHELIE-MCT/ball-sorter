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
    {0.02169298f, 0.05664181f, 0.09614969f, 0.14072534f, 0.22713903f, 0.34907800f, 0.32309881f, 0.16979918f, 0.80962973f, 0.07141441f},
    {0.02794417f, 0.19117656f, 0.22611716f, 0.15256511f, 0.17841870f, 0.20471936f, 0.25235743f, 0.22734047f, 0.83359656f, 0.06534983f},
    {0.02460770f, 0.20431762f, 0.28196595f, 0.23879666f, 0.20621582f, 0.15332507f, 0.16068811f, 0.10165419f, 0.84614037f, 0.05457441f},
    {0.02313950f, 0.07679282f, 0.16046357f, 0.30370673f, 0.30151654f, 0.21696627f, 0.19413745f, 0.12065391f, 0.82549325f, 0.06251663f},
    {0.02176820f, 0.05706867f, 0.10604704f, 0.21260082f, 0.26420960f, 0.29285543f, 0.27761435f, 0.15665997f, 0.82347285f, 0.06733014f},
    {0.02303680f, 0.11911179f, 0.13486184f, 0.09379920f, 0.12998600f, 0.29549506f, 0.37173629f, 0.20295360f, 0.81834172f, 0.07085590f},
    {0.02474595f, 0.07140204f, 0.10049877f, 0.08705818f, 0.13869958f, 0.30738161f, 0.44317074f, 0.27117006f, 0.76624515f, 0.07631318f}};

/**
 * @brief Global inner radius that maps to 100% confidence.
 * @author David Goletta
 * @date 2026-06-18
 */
constexpr float kInnerConfidenceRadius = 0.00097118f;

/**
 * @brief Class-specific outer radii where confidence falls to 0%.
 * @author David Goletta
 * @date 2026-06-18
 *
 * Each entry shares the index of the corresponding `kClassNames` centroid.
 */
constexpr float kOuterConfidenceRadii[kClassCount] = {0.05533810f, 0.09826826f, 0.09826826f, 0.08230344f, 0.05533810f, 0.06368442f, 0.06368442f};

}  // namespace BallClassifier