#include "util.h"
#include <opencv2/opencv.hpp>

cv::Mat cutRegion(const cv::Mat &src, cv::Rect region, uint8_t scale) {
  cv::Mat submat(src, region);
  cv::resize(submat, submat, {region.width * scale, region.height * scale}, 0,
             0, cv::INTER_NEAREST);
  return submat;
}
