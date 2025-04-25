#include <opencv2/core.hpp>

/**
 * @brief Cuts out a region and scales it up
 *
 * @param src The image to cut from
 * @param region The region
 * @param scale The scale factor
 */
cv::Mat cutRegion(const cv::Mat &src, cv::Rect region, uint8_t scale);
