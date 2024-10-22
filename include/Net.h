#ifndef _NET_H
#define _NET_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "AESCoder.h"

#define PI 3.14159265

// 前向声明 CPalm 类
class CPalm;

/*
类名：CNet
入口：CCamera类 ir_frame
     AESCoder类 解密方法
出口：获取roi图以及标志位
     获取特征向量
     获取关键点
*/
class CNet {

  friend class CPalm; // 这里可以安全地声明 CPlam 为友元类
private:
  bool palmDetect(cv::Mat &ir_frame);
  bool roiDetect(cv::Mat &ir_frame);
  std::vector<float> getFeature();
  CNet();
  ~CNet();

private:
  bool decryptModel(std::string model_path, std::vector<uchar> &model_vec);
  void readModel();
  std::vector<cv::String> getOutlayerNames(const cv::dnn::Net net);
  void getYoloOutput(cv::Mat &ir_frame);
  void extractRoi(cv::Mat& ir_frame, float coordinate[3][2]);
  std::vector<float> normalize(const std::vector<float> &code);
  cv::Mat addChannels(cv::Mat &img);

private:
  cv::Mat roi_img_; // roi图像
  bool roi_fail_;   // roi获取失败标志位 true代表获取失败
  std::vector<float> feature_; // 特征向量
  std::vector<cv::Point> points_; // 关键点坐标 两个手指和手掌的中心坐标
  std::vector<cv::Point> roi_rect_; // roi矩形四个顶点

  float conf_thres_; // 置信度阈值
  float nms_thres_;  // 非极大抑制阈值

  int init_img_size_; // 原始图像大小
  int roi_img_size_;  // roi图像大小

  cv::Mat blob_roi_;          // roi模型输入
  std::vector<cv::Mat> outs_; // roi模型输出
  cv::Mat blob_feature_;      // feature模型输入

  cv::dnn::Net roi_net_, feature_net_;
  std::string roi_define_path_, roi_model_path_, feature_model_path_;
  std::vector<uchar> roi_define_vec_, roi_net_vec_, feature_net_vec_;
  
};

#endif