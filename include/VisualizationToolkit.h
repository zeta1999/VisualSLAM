#include <Eigen/Core>
#include <Eigen/Dense>
#include <sophus/se3.hpp>
#include <opencv2/ximgproc/disparity_filter.hpp>
#include <pangolin/pangolin.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/stereo.hpp>
#include <opencv2/core/eigen.hpp>

#include <unistd.h>

void computeAndShowPointCloud(const cv::Mat image_left, const cv::Mat disparity, const float baseline, Eigen::Matrix3d K);
void visualizeAllPoses(std::vector<Sophus::SE3d> historyPoses, Eigen::Matrix3d K);
void plot2DPoints(cv::Mat image, std::vector<cv::Point2f> points2d);
void plot2DPoints(cv::Mat image, std::vector<cv::KeyPoint> keypoints);
void plotTrajectoryNextStep(cv::Mat& window, int index, Eigen::Vector3d& translGTAccumulated, Eigen::Vector3d& translEstimAccumulated,
                            Sophus::SE3d groundTruthPose, Sophus::SE3d groundTruthPrevPose, Sophus::SE3d pose, Eigen::Matrix3d& cumR);
