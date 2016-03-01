//
// Created by wswinny on 2/24/2016.
//

#ifndef CAMERATRACKING_CAMERA_H
#define CAMERATRACKING_CAMERA_H

#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <stdio.h>
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

class Camera {

private:
    const float FRUSTDIST = 0.5f;

    float imageWidth = 640;
    float imageHeight = 480;

    float fov = 60.0f;

    float cameraPitch = 20.5;
    float cameraYaw = 69;

    int cameraMapOffsetX = 70;
    int cameraMapOffsetY = 70;

    std::string videoAddr;

    cv::VideoCapture cap;
    cv::Mat img;
    cv::HOGDescriptor hog;

    //Calculate coordinate of pixel in space assuming that the camera is facing down the
    //X axis with Z upwards and Y 90deg to the left, pixels start from lowerLeft
    glm::vec3 frustrumCoord(int x, int y)
    {
        float aspectRatio = imageWidth/imageHeight;
        float frustWidth = FRUSTDIST * glm::tan(glm::radians(fov/2)) * 2;
        float frustHeight = frustWidth * (1/aspectRatio);

        float percentY = x/imageWidth; //Here the coordinates are translated from image coordinates to the local axis for a +x camera
        float percentZ = (imageHeight-y)/imageHeight; //x->y, y->z and possibly top left to lower left conversion

        float yPoint = percentY * frustWidth;
        float zPoint = percentZ * frustHeight;

        float realFrustDist = sqrt(yPoint*yPoint + zPoint*zPoint + FRUSTDIST*FRUSTDIST);

        return glm::vec3(realFrustDist, yPoint-frustWidth/2, zPoint-frustHeight/2);
    }

    glm::vec3 getPointFromC(int x, int y){
        //unit vector distance from PY (CENTER OF CAMERA)
        /*float x = glm::cos(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
        float y = glm::sin(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
        float z = glm::sin(glm::radians(cPitch));*/


        glm::mat4 cameraRot = glm::rotate(glm::mat4(1.0f), glm::radians(cameraYaw), glm::vec3(0,0,1)); //Yaw rotate
        cameraRot = glm::rotate(cameraRot, glm::radians(cameraPitch), glm::vec3(0,1,0)); //Pitch rotate


        //glm::mat4 frustTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0, FRUSTWIDTH/2, -FRUSTHEIGHT/2));
        glm::vec3 frustCoord = frustrumCoord(x, y);
        frustCoord = glm::vec3(cameraRot * glm::vec4(frustCoord, 1));
        //frustCoord = glm::vec3(frustTranslate * glm::vec4(frustCoord, 1));

        return frustCoord;
    }

    glm::vec3 getIntersect(int x, int y)
    {
        glm::vec3 camera = glm::vec3(0, 0, 3.2f);
        glm::vec3 point = getPointFromC(x, y) + camera;
        glm::vec3 p0 = glm::vec3(0,0,0);
        glm::vec3 normal = glm::vec3(0,0,1);

        glm::vec3 ray = point - camera;

        float d = glm::dot((p0 - point), normal)/glm::dot(ray, normal);
        glm::vec3 isec = d*ray + point;
        return isec;
    }

public:

    Camera(float imageWidth, float imageHeight, float fov, float cameraPitch, float cameraYaw, int cameraMapOffsetX, int cameraMapOffsetY, std::string videoAddr)
    {
        this->imageWidth        = imageWidth;
        this->imageHeight       = imageHeight;
        this->fov               = fov;
        this->cameraPitch       = cameraPitch;
        this->cameraYaw         = cameraYaw;
        this->cameraMapOffsetX  = cameraMapOffsetX;
        this->cameraMapOffsetY  = cameraMapOffsetY;
        this->videoAddr         = videoAddr;

        cap.open(videoAddr);

        hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
        cv::namedWindow(videoAddr, CV_WINDOW_AUTOSIZE);
    }

    std::vector<glm::vec3> getPeoplePoints()
    {
        std::vector<glm::vec3> peoplePoints;

        cap >> img;
        if (!img.data)
            return peoplePoints;

        cv::FileStorage fs_in;
        fs_in.open( "out_camera_data.xml", cv::FileStorage::READ );

        cv::Mat out1, out2, undist, cameraMatrix, distMatrix;

        undist.create(img.size(), img.type());
        fs_in["Camera_Matrix"] >> cameraMatrix;
        fs_in["Distortion_Coefficients"] >> distMatrix;

        cv::initUndistortRectifyMap(cameraMatrix, distMatrix, cv::Mat(), cameraMatrix, img.size(), CV_16SC2, out1, out2);
        cv::remap(img, undist, out1, out2, cv::INTER_LINEAR);

        img = undist;

        std::vector<cv::Rect> found, found_filtered;
        hog.detectMultiScale(img, found, 0, cv::Size(4,4), cv::Size(6,6), 1.30, 4);

        for (int i = 0; i < found.size(); i++)
        {
            cv::Rect r = found[i];
            r.x += cvRound(r.width * 0.1);
            r.width = cvRound(r.width * 0.8);
            r.y += cvRound(r.height * 0.06);
            r.height = cvRound(r.height * 0.9);

            float feetX = r.x + (r.width / 2.0);
            float feetY = r.y + r.height;

            rectangle(this->img, r.tl(), r.br(), cv::Scalar(0,255,0), 2);

            glm::vec3 person = getIntersect((int)feetX, (int)feetY);
            peoplePoints.push_back(person);
        }

        return peoplePoints;
    }

    float getImageWidth() const {
        return imageWidth;
    }

    void setImageWidth(float imageWidth) {
        Camera::imageWidth = imageWidth;
    }

    float getImageHeight() const {
        return imageHeight;
    }

    void setImageHeight(float imageHeight) {
        Camera::imageHeight = imageHeight;
    }

    float getFov() const {
        return fov;
    }

    void setFov(float fov) {
        Camera::fov = fov;
    }

    float getCameraPitch() const {
        return cameraPitch;
    }

    void setCameraPitch(float cameraPitch) {
        Camera::cameraPitch = cameraPitch;
    }

    float getCameraYaw() const {
        return cameraYaw;
    }

    void setCameraYaw(float cameraYaw) {
        Camera::cameraYaw = cameraYaw;
    }

    int getCameraMapOffsetY() const {
        return cameraMapOffsetY;
    }

    void setCameraMapOffsetY(int cameraMapOffsetY) {
        Camera::cameraMapOffsetY = cameraMapOffsetY;
    }

    int getCameraMapOffsetX() const {
        return cameraMapOffsetX;
    }

    void setCameraMapOffsetX(int cameraMapOffsetX) {
        Camera::cameraMapOffsetX = cameraMapOffsetX;
    }

    const std::string &getVideoAddr() const {
        return videoAddr;
    }

    void setVideoAddr(const std::string &videoAddr) {
        Camera::videoAddr = videoAddr;
    }

    const cv::Mat &getImg() const {
        return img;
    }
};


#endif //CAMERATRACKING_CAMERA_H
