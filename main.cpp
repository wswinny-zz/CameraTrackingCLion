#include "Camera.h"

const std::string iCOREfarWallVideoAddr = "http://admin:103iCorE@192.168.103.59/video.cgi?.mjpg"; //icore camera far wall
const std::string iCOREdoorVideoAddr = "http://admin:103iCorE@192.168.103.180/video.cgi?.mjpg"; //icore camera door

const std::string icoreFloorPlanImage = "../../iCORE_floor_plan.jpg";

const float pixelToMeterRatio = 0.01216; //meters in one pixel
const float pixelToFeetRatio = 0.476; //feet in one pixel

int main (int argc, const char * argv[])
{
    cv::Mat icoreFloorPlan;
    cv::namedWindow("floor plan", CV_WINDOW_AUTOSIZE);
    icoreFloorPlan = cv::imread(icoreFloorPlanImage, CV_LOAD_IMAGE_COLOR);

    if(!icoreFloorPlan.data)
    {
        printf("No floor plan data");
        return -1;
    }

    Camera icoreBackWall(640, 480, 60.0f, 20.5, 69, 70, 70, iCOREfarWallVideoAddr);

    while (true)
    {
        std::vector<glm::vec3> peoplePoints = icoreBackWall.getPeoplePoints();

        for(int i = 0; i < peoplePoints.size(); i++) {
            glm::vec3 person = peoplePoints[i];

            int feetPixX = person.x / pixelToMeterRatio;
            int feetPixY = person.y / pixelToMeterRatio;

            cv::circle(icoreFloorPlan, CvPoint(feetPixX + icoreBackWall.getCameraMapOffsetX(), icoreBackWall.getCameraMapOffsetY() + feetPixY), 2, cv::Scalar(255, 0, 0), 4);
        }
        circle(icoreFloorPlan, CvPoint(icoreBackWall.getCameraMapOffsetX(), icoreBackWall.getCameraMapOffsetY()), 2, cv::Scalar(0, 0, 255), 4);

        cv::imshow(icoreBackWall.getVideoAddr(), icoreBackWall.getImg());
        cv::imshow("floor plan", icoreFloorPlan);

        if (cv::waitKey(20) >= 0)
            break;
    }
    return 0;
}