#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <stdio.h>
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

/*
 <Camera_Matrix type_id="opencv-matrix">
  <rows>3</rows>
  <cols>3</cols>
  <dt>d</dt>
  <data>
    1.9982570939581169e+003 0. 3.1950000000000000e+002 0.
    1.9982570939581169e+003 2.3950000000000000e+002 0. 0. 1.</data></Camera_Matrix>
<Distortion_Coefficients type_id="opencv-matrix">
  <rows>5</rows>
  <cols>1</cols>
  <dt>d</dt>
  <data>
    6.9509977809186782e-003 -8.9345795644874160e-001 0. 0.
    1.7167890387968214e+001</data></Distortion_Coefficients>
 */


using namespace cv;

const float WIDTH = 640;
const float HEIGHT = 480;

const float FOV = 60.0f;
const float f = 2.8/1000;

const float FRUSTDIST = 0.5f;

const float ASPECTRATIO = WIDTH/HEIGHT;
const float FRUSTWIDTH = FRUSTDIST * glm::tan(glm::radians(FOV/2)) * 2;
const float FRUSTHEIGHT = FRUSTWIDTH * (1/ASPECTRATIO);

float cPitch = 20.5;
float cYaw = 69;

const std::string videoAddr = "http://admin:103iCorE@192.168.103.59/video.cgi?.mjpg"; //icore camera far wall
//const std::string videoAddr = "http://admin:103iCorE@192.168.103.180/video.cgi?.mjpg"; //icore camera door
//const std::string videoAddr = "http://plazacam.studentaffairs.duke.edu/mjpg/video.mjpg"; //duke university plaza

const std::string icoreFloorPlanImage = "../../iCORE_floor_plan.jpg";
const int cameraPixelX = 70;
const int cameraPixelY = 70;

const float pixelToMeterRatio = 0.01216; //meters in one pixel
const float pixelToFeetRatio = 0.476; //feet in one pixel

//Calculate coordinate of pixel in space assuming that the camera is facing down the
//X axis with Z upwards and Y 90deg to the left, pixels start from lowerLeft
glm::vec3 frustrumCoord(int x, int y)
{
    const Point center = {WIDTH/2,HEIGHT/2};

    float percentY = x/WIDTH; //Here the coordinates are translated from image coordinates to the local axis for a +x camera
    float percentZ = (HEIGHT-y)/HEIGHT; //x->y, y->z and possibly top left to lower left conversion

    float yPoint = percentY * FRUSTWIDTH;
    float zPoint = percentZ * FRUSTHEIGHT;


    float realFrustDist = sqrt(yPoint*yPoint + zPoint*zPoint + FRUSTDIST*FRUSTDIST);

    return glm::vec3(realFrustDist, yPoint-FRUSTWIDTH/2, zPoint-FRUSTHEIGHT/2);
}

glm::vec3 getPointFromC(int x, int y){
    //unit vector distance from PY (CENTER OF CAMERA)
    /*float x = glm::cos(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
    float y = glm::sin(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
    float z = glm::sin(glm::radians(cPitch));*/


    glm::mat4 cameraRot = glm::rotate(glm::mat4(1.0f), glm::radians(cYaw), glm::vec3(0,0,1)); //Yaw rotate
    cameraRot = glm::rotate(cameraRot, glm::radians(cPitch), glm::vec3(0,1,0)); //Pitch rotate


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
    printf("camera: %f, %f, %f\nintersect: %f, %f, %f\nD: %f\nFrust: %f, %f\n", point.x, point.y, point.z, isec.x, isec.y, isec.z, d, FRUSTWIDTH, FRUSTHEIGHT);
    return isec;
}


int main (int argc, const char * argv[])
{
    VideoCapture cap;
    if(!cap.open(videoAddr))
    {
        printf("Error opening video stream\n");
        return -1;
    }

    Mat icoreFloorPlan;
    Mat img;
    HOGDescriptor hog;
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());

    namedWindow("video capture", CV_WINDOW_AUTOSIZE);
    namedWindow("floor plan", CV_WINDOW_AUTOSIZE);

    icoreFloorPlan = imread(icoreFloorPlanImage, CV_LOAD_IMAGE_COLOR);

//    CvMat* intrinsics = cvCreateMat(3, 3, CV_64FC1);
//
//
//    IplImage tmp=img;
//
//    CvArr* mapx = cvCreateImage(cvGetSize(&tmp),IPL_DEPTH_32F, 1);
//    CvArr* mapy = cvCreateImage(cvGetSize(&tmp), IPL_DEPTH_32F, 1);
//    cvInitUndistortMap(intrinsics, dist_coeffs, mapx, mapy)
//    cvRemap(src, dst, mapx, mapy, cv.CV_INTER_LINEAR + cv.CV_WARP_FILL_OUTLIERS,  cv.ScalarAll(0))



    if(!icoreFloorPlan.data)
    {
        printf("No floor plan data");
        return -1;
    }

    while (true)
    {
        cap >> img;
        if (!img.data)
            continue;

//
//        out1.create(3,3, CV_64F);
//        out2.create(3,3, CV_64F);


        FileStorage fs_in;
        fs_in.open( "out_camera_data.xml", FileStorage::READ );


        cv::Mat out1, out2, undist, cameraMatrix, distMatrix;

        undist.create(img.size(), img.type());
        fs_in["Camera_Matrix"] >> cameraMatrix;
        fs_in["Distortion_Coefficients"] >> distMatrix;


        cv::initUndistortRectifyMap(cameraMatrix, distMatrix, cv::Mat(), cameraMatrix, img.size(), CV_16SC2, out1, out2);
        cv::remap(img, undist, out1, out2, INTER_LINEAR);

        img = undist;
        //icoreFloorPlan = imread(icoreFloorPlanImage, CV_LOAD_IMAGE_COLOR);
        if(!icoreFloorPlan.data)
        {
            printf("No floor plan data");
            return -1;
        }

        std::vector<Rect> found, found_filtered;
        hog.detectMultiScale(img, found, 0, Size(4,4), Size(6,6), 1.30, 4);

        size_t i, j;
        for (i = 0; i < found.size(); i++)
        {
            Rect r = found[i];

            for (j = 0; j < found.size(); j++)
            {
                if (j != i && (r & found[j]) == r)
                    break;
            }

            if (j == found.size())
                found_filtered.push_back(r);
        }

        //printf("\x1B[2J\x1B[H"); //clears the console in unix systems

        for (i = 0; i < found_filtered.size(); i++)
        {
            Rect r = found_filtered[i];
            r.x += cvRound(r.width*0.1);
            r.width = cvRound(r.width*0.8);
            r.y += cvRound(r.height*0.06);
            r.height = cvRound(r.height*0.9);

            float feetX = r.x + (r.width / 2.0);
            float feetY = r.y + r.height;

            glm::vec3 person = getIntersect((int)feetX, (int)feetY);
            int feetPixX = person.x / pixelToMeterRatio;
            int feetPixY = person.y / pixelToMeterRatio;

            circle(icoreFloorPlan, CvPoint(feetPixX + cameraPixelX, cameraPixelY + feetPixY), 2, cv::Scalar(255, 0, 0), 4);
            rectangle(img, r.tl(), r.br(), cv::Scalar(0,255,0), 2);
        }

        circle(icoreFloorPlan, CvPoint(cameraPixelX, cameraPixelY), 2, cv::Scalar(0, 0, 255), 4);

        imshow("video capture", img);
        imshow("undist", undist);
        imshow("floor plan", icoreFloorPlan);

        if (waitKey(20) >= 0)
            break;
    }
    return 0;
}