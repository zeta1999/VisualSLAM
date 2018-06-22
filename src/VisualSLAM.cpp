#include "VisualSLAM.h"

VisualSLAM::VisualSLAM() {
    K = Eigen::Matrix3d::Identity();
}

Sophus::SE3d VisualSLAM::getPose(int index) {
    if (index < 0 || index >= historyPoses.size()){
        throw std::runtime_error("VisualSLAM::getPose() : Index out of bounds");
    }

    return historyPoses.at(index);
}

int VisualSLAM::getNumberPoses() const{
    return historyPoses.size();
}

Eigen::Matrix3d VisualSLAM::getCameraMatrix() const {
    return K;
}

double VisualSLAM::getFocalLength() const {
    return (K(0,0) + K(1,1)) / 2.0;
}

void VisualSLAM::readCameraIntrisics(std::string camera_file_path){
	std::ifstream file;
	file.open(camera_file_path, std::ifstream::in);

	if (!file){
		throw std::runtime_error("Cannot read the file with camera intrinsics");
	}

	std::string prefix;
	double data[12];

	file >> prefix;
	for (int i = 0; i < 12; i++){
		file >> data[i];
	}

	// fx, fy, cx, cy
    K(0,0) = data[0];
    K(1,1) = data[5];
    K(0,2) = data[2];
    K(1,2) = data[6];

}

void VisualSLAM::readGroundTruthData(std::string fileName, int numberFrames, std::vector<Sophus::SE3d>& groundTruthData){
    std::ifstream inFile;
    inFile.open(fileName, std::ifstream::in);

    if (!inFile){
        throw std::runtime_error("readGroundTruthData() : Cannot read the file with ground truth data");
    }
    if (numberFrames <= 0){
        throw std::runtime_error("readGroundTruthData() : Number of frames is non-positive!");
    }

    groundTruthData.clear();

    int i = 0;
    while(i < numberFrames && !inFile.eof()){
        double rotationElements[9], translationElements[3];
        int k = 0;
        for (int j = 1; j <= 12; j++){
            if (j % 4 == 0){
                inFile >> translationElements[j / 4 - 1];
            } else {
                inFile >> rotationElements[k++];
            }
        }
        cv::Mat R_CV = cv::Mat(3,3, CV_64F, rotationElements);
        Eigen::Matrix3d R_Eigen;
        cv::cv2eigen(R_CV, R_Eigen);
        Sophus::SE3d newPose = Sophus::SE3d(Eigen::Quaterniond(R_Eigen), Eigen::Vector3d(translationElements));
        groundTruthData.push_back(newPose);
        i++;
    }
}

Sophus::SE3d VisualSLAM::performFrontEndStep(cv::Mat image_left, cv::Mat image_right, std::vector<cv::KeyPoint>& keyPointsPrevFrame, cv::Mat& descriptorsPrevFrame){
    cv::Mat descriptorsCurrentFrame;
    std::vector<cv::KeyPoint> keyPointsCurrentFrame;

    VO.extractORBFeatures(image_left, keyPointsCurrentFrame, descriptorsCurrentFrame);
    cv::Mat disparityCurrentFrame = VO.getDisparityMap(image_left, image_right);

    Sophus::SE3d pose;
    if (keyPointsPrevFrame.empty()){
      keyPointsPrevFrame = keyPointsCurrentFrame;
      descriptorsCurrentFrame.copyTo(descriptorsPrevFrame);

      return pose;
    }

    std::vector<cv::DMatch> matches = VO.findGoodORBFeatureMatches(keyPointsPrevFrame, keyPointsCurrentFrame, descriptorsPrevFrame, descriptorsCurrentFrame);

    std::vector<cv::Point2f> p2d_prevFrame, p2d_currFrame;

    VO.get2D2DCorrespondences(keyPointsPrevFrame, keyPointsCurrentFrame, matches, p2d_prevFrame, p2d_currFrame);

    std::vector<cv::Point3f> p3d_currFrame = VO.get3DCoordinates(p2d_currFrame, disparityCurrentFrame, K);

    VO.estimatePose3D2D(p3d_currFrame, p2d_prevFrame, K, pose);

    descriptorsCurrentFrame.copyTo(descriptorsPrevFrame);

    keyPointsPrevFrame.clear();
    keyPointsPrevFrame = keyPointsCurrentFrame;

    return pose;
}

Sophus::SE3d VisualSLAM::performFrontEndStepWithTracking(cv::Mat image_left, cv::Mat image_right, std::vector<cv::Point2f>& pointsCurrentFrame, std::vector<cv::Point2f>& pointsPreviousFrame, cv::Mat& prevImageLeft){
    int max_features = 500;
    cv::TermCriteria termcrit(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,20,0.03);
    cv::Size subPixWinSize(10,10);

    cv::Mat disparity_map = VO.getDisparityMap(image_left, image_right);
    Sophus::SE3d pose;

    if (pointsPreviousFrame.empty()){
        std::cout << "INIT" << std::endl;
        cv::goodFeaturesToTrack(image_left, pointsCurrentFrame, max_features, 0.01, 10, cv::Mat(), 3, 3, 0, 0.04);
        cv::cornerSubPix(image_left, pointsCurrentFrame, subPixWinSize, cv::Size(-1,-1), termcrit);
        pointsPreviousFrame.clear();
        pointsPreviousFrame = pointsCurrentFrame;

        image_left.copyTo(prevImageLeft);
        return pose;
    }

    int thresholdNumberFeatures = 200;
    bool init = false;

    VO.trackFeatures(prevImageLeft, image_left, pointsPreviousFrame, pointsCurrentFrame, thresholdNumberFeatures, init);

    std::vector<cv::Point3f> points3DCurrentFrame = VO.get3DCoordinates(pointsCurrentFrame, disparity_map, K);

    //VO.estimatePose2D2D(pointsPreviousFrame, pointsCurrentFrame, K, pose);
    VO.estimatePose3D2D(points3DCurrentFrame, pointsPreviousFrame, K, pose);

    if (init){
        pointsCurrentFrame.clear();
        cv::goodFeaturesToTrack(image_left, pointsCurrentFrame, max_features, 0.01, 10, cv::Mat(), 3, 3, 0, 0.04);
        cv::cornerSubPix(image_left, pointsCurrentFrame, subPixWinSize, cv::Size(-1,-1), termcrit);
    }

    pointsPreviousFrame.clear();
    pointsPreviousFrame = pointsCurrentFrame;

    image_left.copyTo(prevImageLeft);
    return pose;
}