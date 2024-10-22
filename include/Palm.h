#ifndef _PALM_H
#define _PALM_H

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <thread>

#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

#include "AESCoder.h"
#include "Camera.h"
#include "Net.h"

/*
类名：CPalm
入口：CLicense类 激活标志位
     CCamera类 ir_frame
     CNet类特征向量 roi标志位
出口：注册
     识别
     数据库操作
     退出
     绘图
*/
class CPalm {
public:
  void userRegister();
  void userRecognize();
  void userDatabase();
  void userQuit();
  const cv::Mat getFrameWithPoints();

  CPalm();
  ~CPalm();

private:
  void userDataload(std::string &userdata_path);
  void userDataSave(std::string &userdata_path);
  bool userNameCheck(std::string &username);
  int userFeatureMatch(std::vector<float> &feature, float &threshold);
  bool assessImg(cv::Mat &img);
  void membersClear();

private:
  // 用户data
  struct UserData {
    std::string name;
    std::vector<float> feature;
  };

  CCamera *camera_;
  CNet *cnet_;

  // 手掌与roi检测相关成员
  static const int IMG_NUM_REGISTER = 5;  // 注册需要的有效图像数量
  static const int IMG_NUM_RECOGNIZE = 2; // 识别需要的有效图像数量
  static const int PALM_COUNT = 2000;     // 手掌检测轮数
  static const int ROI_COUNT = 1000;      // roi检测轮数
  static const int LEN_FEATURE =
      128; // 特征向量长度   C++11对静态常量的声明方式，不需要在类外初始化

  std::string userdata_path_; // 数据库保存路径
  float match_thres_;         // 匹配阈值
  bool palm_fail_;       // 检测手掌是否成功标志位
  bool img_scalar_fail_; // 图像平均曝光 即进行手掌检测时图像的曝光
  UserData user_;                            // 包含用户名与特征向量
  std::vector<std::vector<float>> features_; // 储存有效的特征

  std::vector<UserData> user_database_; // 数据库储存用户数据
};


#endif