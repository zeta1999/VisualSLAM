#include "VisualizationToolkit.h"

VisualizationToolkit::VisualizationToolkit(Eigen::Matrix3d calibMat, float baseline){
    this->K = calibMat;
    this->baseline = baseline;

    mProcessed = true;
    mReadyToProcess = false;
}

void VisualizationToolkit::plot2DPoints(cv::Mat image, std::vector<cv::Point2f> points2d){
    if (points2d.empty()){
        std::cout << "plot2DPoints(): No features to plot!" << std::endl;
        return;
    }

    cv::Mat imageCopy;
    if (image.cols == 0){
        imageCopy = cv::Mat(500, 500, CV_8UC3);
    } else{
        cv::cvtColor(image, imageCopy, CV_GRAY2BGR);
    }

    for (auto& pt : points2d){
        cv::circle(imageCopy, pt, 3, cv::Scalar(0,0,255), -1);
    }
    
    cv::pyrDown(imageCopy, imageCopy);
    cv::imshow("Detected features", imageCopy);
}

void VisualizationToolkit::plot2DPoints(cv::Mat image, std::vector<cv::KeyPoint> keypoints){
    std::vector<cv::Point2f> points2d;

    for (auto& kPt : keypoints){
        points2d.push_back(kPt.pt);
    }

    plot2DPoints(image, points2d);
}

void VisualizationToolkit::plotTrajectoryNextStep(cv::Mat& window, int index, Eigen::Vector3d& translGTAccumulated, Eigen::Vector3d& translEstimAccumulatedLeft, Eigen::Vector3d& translEstimAccumulatedRight,
                            Sophus::SE3d groundTruthPose, Sophus::SE3d groundTruthPrevPose, Sophus::SE3d estimPoseLeft, Sophus::SE3d estimPoseRight,  Sophus::SE3d estimPrevPoseLeft, Sophus::SE3d estimPrevPoseRight){
    int offsetX = 300;
    int offsetY = 450;

    Sophus::SE3d poseLeft = estimPoseLeft.inverse();
    Sophus::SE3d prevPoseLeft = estimPrevPoseLeft.inverse();

    Sophus::SE3d poseRight = estimPoseRight.inverse();
    Sophus::SE3d prevPoseRight = estimPrevPoseRight.inverse();

    if (index == 0){
        translGTAccumulated = groundTruthPose.translation();
        translEstimAccumulatedLeft = poseLeft.translation();
        translEstimAccumulatedRight = poseRight.translation();
    } else {
        translGTAccumulated = translGTAccumulated + (groundTruthPose.so3().inverse()*groundTruthPrevPose.so3())*(groundTruthPose.translation() - groundTruthPrevPose.translation());
        translEstimAccumulatedLeft = translEstimAccumulatedLeft + (poseLeft.so3().inverse()*prevPoseLeft.so3())*(poseLeft.translation() - prevPoseLeft.translation());
        translEstimAccumulatedRight = translEstimAccumulatedRight + (poseRight.so3().inverse()*prevPoseRight.so3())*(poseRight.translation() - prevPoseRight.translation());
    }
    //cv::circle(window, cv::Point2d(offsetX + translGTAccumulated[2], offsetX + translGTAccumulated[1]), 3, cv::Scalar(0,0,255), -1);
    //cv::circle(window, cv::Point2f(offsetX + translEstimAccumulated[2], offsetY + translEstimAccumulated[1]), 3, cv::Scalar(0,255,0), -1);
    cv::circle(window, cv::Point2f(offsetX + translGTAccumulated[0], offsetY - translGTAccumulated[2]), 3, cv::Scalar(0,0,255), -1);
    //cv::circle(window, cv::Point2f(offsetX + translEstimAccumulatedLeft[0], offsetY - translEstimAccumulatedLeft[2]), 3, cv::Scalar(0,255,0), -1);
    cv::circle(window, cv::Point2f(offsetX + translEstimAccumulatedRight[0], offsetY - translEstimAccumulatedRight[2]), 3, cv::Scalar(255,0,0), -1);
}

void VisualizationToolkit::replotTrajectory(cv::Mat& window, int index, Eigen::Vector3d& translGTAccumulated,
                      Eigen::Vector3d& translEstimAccumulatedLeft, Eigen::Vector3d& translEstimAccumulatedRight,
                      std::vector<Sophus::SE3d> cumPosesLeft, std::vector<Sophus::SE3d> cumPosesRight,
                      std::vector<Sophus::SE3d> groundTruthPoses){
    if (index <= 0 || groundTruthPoses.empty() || cumPosesLeft.empty() || cumPosesRight.empty()){
        std::cerr << "Data is empty or not valid for re-plotting" << std::endl;
        return;
    }

    assert(groundTruthPoses.size() > index);
    assert(cumPosesLeft.size() == cumPosesRight.size());
    assert(index + 1 == cumPosesLeft.size());

    cv::Mat newWindow = cv::Mat::zeros(window.cols, window.rows, CV_8UC3);
    Eigen::Vector3d newtranslGTAccumulated = groundTruthPoses[0].translation();;
    Eigen::Vector3d newtranslEstimAccumulatedLeft = cumPosesLeft[0].translation();
    Eigen::Vector3d newtranslEstimAccumulatedRight = cumPosesRight[0].translation();

    int offsetX = 300;
    int offsetY = 450;

    for (int i = 1; i <= index; i++){
        Sophus::SE3d poseLeft = cumPosesLeft[i].inverse();
        Sophus::SE3d prevPoseLeft = cumPosesLeft[i-1].inverse();
        Sophus::SE3d poseRight = cumPosesRight[i].inverse();
        Sophus::SE3d prevPoseRight = cumPosesRight[i-1].inverse();

        newtranslGTAccumulated = newtranslGTAccumulated + (groundTruthPoses[i].so3().inverse()*groundTruthPoses[i-1].so3())*(groundTruthPoses[i].translation() - groundTruthPoses[i -1].translation());
        newtranslEstimAccumulatedLeft = newtranslEstimAccumulatedLeft + (poseLeft.so3().inverse()*prevPoseLeft.so3())*(poseLeft.translation() - prevPoseLeft.translation());
        newtranslEstimAccumulatedRight = newtranslEstimAccumulatedRight + (poseRight.so3().inverse()*prevPoseRight.so3())*(poseRight.translation() - prevPoseRight.translation());
        cv::circle(newWindow, cv::Point2f(offsetX + newtranslGTAccumulated[0], offsetY - newtranslGTAccumulated[2]), 3, cv::Scalar(0,0,255), -1);
        //cv::circle(newWindow, cv::Point2f(offsetX + newtranslEstimAccumulatedLeft[0], offsetY - newtranslEstimAccumulatedLeft[2]), 3, cv::Scalar(0,255,0), -1);
        cv::circle(newWindow, cv::Point2f(offsetX + newtranslEstimAccumulatedRight[0], offsetY - newtranslEstimAccumulatedRight[2]), 3, cv::Scalar(255,0,0), -1);
    }

    translEstimAccumulatedLeft = newtranslEstimAccumulatedLeft;
    translEstimAccumulatedRight = newtranslEstimAccumulatedRight;
    translGTAccumulated = newtranslGTAccumulated;

    newWindow.copyTo(window);

}

void VisualizationToolkit::getDataForPointCloudVisualization(cv::Mat& image_left, cv::Mat& disparity){
    {
        std::unique_lock<std::mutex> lock(mReadWriteMutex);
        mCondVar.wait(lock, [this]{return mReadyToProcess;});

        image.copyTo(image_left);
        this->disparity.copyTo(disparity);

        mProcessed = true;
    }

    mCondVar.notify_one();
}

void VisualizationToolkit::setDataForPointCloudVisualization(cv::Mat image_left, cv::Mat disparity){
    {
        std::unique_lock<std::mutex> lock(mReadWriteMutex);
        mCondVar.wait(lock, [this]{return mProcessed;});

        image_left.copyTo(image);
        disparity.copyTo(this->disparity);

        mReadyToProcess = true;
    }

    mCondVar.notify_one();
}

void VisualizationToolkit::computeAndShowPointCloud() {
    std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>> pointcloud;
    cv::Mat image_left, disparity;

    double fx = K(0,0);
    double fy = K(1,1);
    double cx = K(0,2);
    double cy = K(1,2);

    pangolin::CreateWindowAndBind("Point Cloud Viewer", 1024, 768);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pangolin::OpenGlRenderState s_cam(
            pangolin::ProjectionMatrix(1024, 768, 500, 500, 512, 389, 0.1, 1000),
            pangolin::ModelViewLookAt(0, -0.1, -1.8, 0, 0, 0, 0.0, -1.0, 0.0)
    );

    pangolin::View &d_cam = pangolin::CreateDisplay()
            .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0, -1024.0f / 768.0f)
            .SetHandler(new pangolin::Handler3D(s_cam));

    while (pangolin::ShouldQuit() == false) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        d_cam.Activate(s_cam);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        getDataForPointCloudVisualization(image_left, disparity);

        glPointSize(2);
        glBegin(GL_POINTS);

        for (int v = 0; v < image_left.rows; v+=2){
            for (int u = 0; u < image_left.cols; u+=2) {
                double z = fx*baseline/(disparity.at<float>(v,u));
                double x = (u - cx)*z / fx;
                double y = (v - cy)*z / fy;

                Eigen::Vector4d p(x, y, z,
                               image_left.at<uchar>(v, u) / 255.0); // first three components are XYZ and the last is color
                glColor3f(p[3], p[3], p[3]);
                glVertex3d(p[0], p[1], p[2]);
            }
        }

        glEnd();
        pangolin::FinishFrame();
        usleep(5000);   // sleep 5 ms
    }
    return;
}

void VisualizationToolkit::visualizeAllPoses(std::vector<Sophus::SE3d> historyPoses, Eigen::Matrix3d K){
    // create pangolin window and plot the trajectory
    pangolin::CreateWindowAndBind("VisualSLAM Viewer", 1024, 768);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pangolin::OpenGlRenderState s_cam(
                pangolin::ProjectionMatrix(1024, 768, 500, 500, 512, 389, 0.1, 1000),
                pangolin::ModelViewLookAt(0, -0.1, -1.8, 0, 0, 0, 0.0, 1.0, 0.0)
                );

    pangolin::View &d_cam = pangolin::CreateDisplay()
            .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0, -1024.0f / 768.0f)
            .SetHandler(new pangolin::Handler3D(s_cam));

    double fx = K(0,0);
    double fy = K(1,1);
    double cx = K(0,2);
    double cy = K(1,2);

    while (pangolin::ShouldQuit() == false){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        d_cam.Activate(s_cam);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // draw poses
        float sz = 0.1;
        int width = 640, height = 480;
        for (auto &Tcw: historyPoses){
            glPushMatrix();
            Sophus::Matrix4f m = Tcw.inverse().matrix().cast<float>();
            glMultMatrixf((GLfloat *) m.data());
            glColor3f(1, 0, 0);
            glLineWidth(2);
            glBegin(GL_LINES);
            glVertex3f(0, 0, 0);
            glVertex3f(sz * (0 - cx) / fx, sz * (0 - cy) / fy, sz);
            glVertex3f(0, 0, 0);
            glVertex3f(sz * (0 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(0, 0, 0);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(0, 0, 0);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (0 - cy) / fy, sz);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (0 - cy) / fy, sz);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(sz * (0 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(sz * (0 - cx) / fx, sz * (height - 1 - cy) / fy, sz);
            glVertex3f(sz * (0 - cx) / fx, sz * (0 - cy) / fy, sz);
            glVertex3f(sz * (0 - cx) / fx, sz * (0 - cy) / fy, sz);
            glVertex3f(sz * (width - 1 - cx) / fx, sz * (0 - cy) / fy, sz);
            glEnd();
            glPopMatrix();
        }

        pangolin::FinishFrame();
        usleep(5000);   // sleep 5 ms
    }
}

void VisualizationToolkit::showPointCloud(const std::vector<cv::Point3f> points3D) {

    if (points3D.empty()) {
        throw std::runtime_error("Point cloud is empty!");
    }

    pangolin::CreateWindowAndBind("Point Cloud Viewer", 1024, 768);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pangolin::OpenGlRenderState s_cam(
            pangolin::ProjectionMatrix(1024, 768, 500, 500, 512, 389, 0.1, 1000),
            pangolin::ModelViewLookAt(0, -0.1, -1.8, 0, 0, 0, 0.0, -1.0, 0.0)
    );

    pangolin::View &d_cam = pangolin::CreateDisplay()
            .SetBounds(0.0, 1.0, pangolin::Attach::Pix(175), 1.0, -1024.0f / 768.0f)
            .SetHandler(new pangolin::Handler3D(s_cam));

    while (pangolin::ShouldQuit() == false) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        d_cam.Activate(s_cam);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glPointSize(2);
        glBegin(GL_POINTS);

        for (auto &p: points3D) {
            glColor3f(0.0, 0.0, 1.0);
            glVertex3d(p.x, p.y, p.z);
        }

        glEnd();
        pangolin::FinishFrame();
        usleep(5000);   // sleep 5 ms
    }
    return;
}
