#include <iostream>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <stdio.h>
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

using namespace cv;

const float WIDTH = 640;
const float HEIGHT = 480;

const float FOV = 60.0f;

const float FRUSTDIST = 1.0f;

const float ASPECTRATIO = WIDTH/(float)HEIGHT;
const float FRUSTWIDTH = FRUSTDIST * glm::tan(glm::radians(FOV/2)) * 2;
const float FRUSTHEIGHT = FRUSTWIDTH * (1/ASPECTRATIO);

float cPitch = 30;
float cYaw = (90 - 66.33);

//const std::string videoAddr = "http://admin:103iCorE@192.168.103.59/video.cgi?.mjpg"; //icore camera far wall
//const std::string videoAddr = "http://admin:103iCorE@192.168.103.180/video.cgi?.mjpg"; //icore camera door
const std::string videoAddr = "http://plazacam.studentaffairs.duke.edu/mjpg/video.mjpg"; //duke university plaza


//Calculate coordinate of pixel in space assuming that the camera is facing down the
//X axis with Z upwards and Y 90deg to the left, pixels start from lowerLeft
glm::vec3 frustrumCoord(int x, int y)
{
    float percentY = x/WIDTH; //Here the coordinates are translated from image coordinates to the local axis for a +x camera
    float percentZ = (HEIGHT-y)/HEIGHT; //x->y, y->z and possibly top left to lower left conversion

    float yPoint = percentY * FRUSTWIDTH;
    float zPoint = percentZ * FRUSTHEIGHT;

    return glm::vec3(FRUSTDIST, yPoint, zPoint);
}

glm::vec3 getPointFromC(int x, int y){
    //unit vector distance from PY (CENTER OF CAMERA)
    /*float x = glm::cos(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
    float y = glm::sin(glm::radians(cYaw))*glm::cos(glm::radians(cPitch));
    float z = glm::sin(glm::radians(cPitch));*/


    glm::mat4 cameraRot = glm::rotate(glm::mat4(1.0f), glm::radians(cYaw), glm::vec3(0,0,1)); //Yaw rotate
    cameraRot = glm::rotate(cameraRot, glm::radians(cPitch), glm::vec3(0,1,0)); //Pitch rotate


    glm::mat4 frustTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0, FRUSTWIDTH/2, -FRUSTHEIGHT/2));
    glm::vec3 frustCoord = frustrumCoord(x, y);
    frustCoord = glm::vec3(frustTranslate * glm::vec4(frustCoord, 1));
    frustCoord = glm::vec3(cameraRot * glm::vec4(frustCoord, 1));

    return frustCoord;
}

glm::vec3 getIntersect(int x, int y)
{
    glm::vec3 camera = glm::vec3(0, 0, 3.3f);
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

    Mat img;
    HOGDescriptor hog;
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());

    namedWindow("video capture", CV_WINDOW_AUTOSIZE);

    while (true)
    {
        cap >> img;
        if (!img.data)
            continue;

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

        printf("\x1B[2J\x1B[H"); //clears the console in unix systems

        for (i = 0; i < found_filtered.size(); i++)
        {
            Rect r = found_filtered[i];
            r.x += cvRound(r.width*0.1);
            r.width = cvRound(r.width*0.8);
            r.y += cvRound(r.height*0.06);
            r.height = cvRound(r.height*0.9);

            float feetX = r.x + (r.width / 2.0);
            float feetY = r.y + r.height;

            //printf("x: %08.2f y: %08.2f \n", feetX, feetY);
            getIntersect((int)feetX, (int)feetY);

            rectangle(img, r.tl(), r.br(), cv::Scalar(0,255,0), 2);
        }

        imshow("video capture", img);

        if (waitKey(20) >= 0)
            break;
    }
    return 0;
}