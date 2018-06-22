// * Class BundleAdjuster is responsible for optimizations of 3D structure and camera pose


#include <Eigen/Core>
#include <Eigen/Dense>
#include <sophus/se3.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/stereo.hpp>
#include <opencv2/core/eigen.hpp>

#include <string>
#include <vector>

#include <ceres/ceres.h>
#include <ceres/rotation.h>
#include <chrono>

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

struct ReprojectionError3D
{
        ReprojectionError3D(double observed_u, double observed_v)
                :observed_u(observed_u), observed_v(observed_v)
                {}

        template <typename T>
        bool operator()(const T* const camera, const T* const camera_T, const T* point, T* residuals) const
        {
                T p[3];
                /***** code from hk technology */
                ceres::QuaternionRotatePoint(camera, point, p);
                p[0] += camera_T[0];
                p[1] += camera_T[1];
                p[2] += camera_T[2];

                T fx = T(718.856);
                T fy = T(718.856);
                T cx = T(607.1928);
                T cy = T(185.2157);

                T xp = p[0]*fx*1./ p[2] +cx;
                T yp = p[1]*fy*1./ p[2] +cy;

                residuals[0] = xp - T(observed_u);
                residuals[1] = yp - T(observed_v);
                return true;
        }

        static ceres::CostFunction* Create(const double observed_x,
                                           const double observed_y)
        {
          return (new ceres::AutoDiffCostFunction<
                  ReprojectionError3D, 2, 4, 3, 3>(
                        new ReprojectionError3D(observed_x,observed_y)));
        }

        double observed_u;
        double observed_v;
};

class BundleAdjuster {
public:
        BundleAdjuster();
        Sophus::SE3d optimizeLocalPoseBA_ceres(std::vector<cv::Point3f> p3d,std::vector<cv::Point2f> p2d,Eigen::Matrix3d K,Sophus::SE3d pose);
};
