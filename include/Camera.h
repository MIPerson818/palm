#ifndef _CAMERA_H
#define _CAMERA_H


#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

class CPalm;
/*
类名：CCamera
入口：
出口：
*/
class CCamera {
  friend class CPalm;

private:
  cv::Mat& getFrame(std::string id); //
  CCamera();
  ~CCamera();

private:
  cv::VideoCapture ir_cap_;
  cv::VideoCapture rgb_cap_;
  cv::Mat ir_frame_;
  cv::Mat rgb_frame_;
};

#endif
