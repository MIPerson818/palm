#include "Net.h"

CNet::CNet() {
  conf_thres_ = 0.5;
  nms_thres_ = 0.4;
  init_img_size_ = 416;
  roi_img_size_ = 112;

  roi_define_path_ = R"(../res/model.c)";  // 存放包含模型结构（如层次、连接等）的字符串的位置 
  roi_model_path_ = R"(../res/weight.w)";  // 模型权重（即训练好的参数）的字节数组
  feature_model_path_ = R"(../res/model.p)";

  readModel();

  roi_img_ = cv::Mat::zeros(roi_img_size_, roi_img_size_, CV_8U);
  roi_fail_ = true;
  feature_ = std::vector<float>(128, 0.0);
  points_ = std::vector<cv::Point>(3,cv::Point(0.0,0.0));
}

CNet::~CNet() {}

// 对模型进行解密 成功返回true 否则false
bool CNet::decryptModel(std::string model_path, std::vector<uchar> &model_vec) {
  std::ifstream fin(model_path);
  if(!fin.is_open()) return false;

  // 流迭代器读取文件内容，将其存储在 cipher_text 字符串中，表示加密的文本。
  std::string cipher_text((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
  fin.close();

  std::string decrypted_text = AESCoder::aesDecrypt(cipher_text);
  // model_vec.assign(decrypted_text.begin(), decrypted_text.end());
  copy(decrypted_text.begin(), decrypted_text.end(), model_vec.begin()); // copy函数将decrypted_text的内容复制到model_vec中

  return true;
}

void CNet::readModel() {
  // 解密模型文件
  bool decrypt_define_label = decryptModel(roi_define_path_, roi_define_vec_);
  bool decrypt_roi_label = decryptModel(roi_model_path_, roi_net_vec_);
  bool decrypt_feature_label =
      decryptModel(feature_model_path_, feature_net_vec_);

  if (!(decrypt_define_label && decrypt_roi_label && decrypt_feature_label)) {
    std::cerr << "\n模型解密失败！\n";
    exit(-4);
  }
  // 解密完成后读取两个模型的结构，并进行设置
  roi_net_ = cv::dnn::readNetFromDarknet(roi_define_vec_, roi_net_vec_);
  // OpenCV 提供了多种后端计算选项，如 OpenCV、OpenVINO、CUDA 等。
  roi_net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
  roi_net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

  feature_net_ = cv::dnn::readNetFromTensorflow(feature_net_vec_);
}

// 获取模型所有输出层名 用作前向预测形参
std::vector<cv::String> CNet::getOutlayerNames(const cv::dnn::Net net) {
  std::vector<cv::String> names;
  if (names.empty()) {
    std::vector<int> out_layers = net.getUnconnectedOutLayers();

    std::vector<cv::String> layersNames = net.getLayerNames();
    for (auto out_layer : out_layers) 
      names.push_back(layersNames[out_layer - 1]);
  }
  return names;
}

// opencv进行模型推理，得到推理结果outs
void CNet::getYoloOutput(cv::Mat &ir_frame_) {
  // 将原始图像处理得到模型输入需要的格式：进行了颜色变换 bgr->rgb，缩放，归一化·
  cv::dnn::blobFromImage(ir_frame_, blob_roi_, 1 / 255.0,
                         cv::Size(init_img_size_, init_img_size_),
                         cv::Scalar(0, 0, 0), true, false);
  roi_net_.setInput(blob_roi_);

  // 前向推理
  roi_net_.forward(outs_,this->getOutlayerNames(roi_net_));
}

// 利用yolo检测到的手掌与关键点建立坐标系 得到roi图像
// XXX：角度接近90或-90时出现错误
void CNet::extractRoi(cv::Mat &ir_frame, float coordinate[3][2]) {
  cv::Mat gray_img;
  cv::cvtColor(ir_frame, gray_img, cv::COLOR_BGR2GRAY);

  int W = gray_img.cols;
  int H = gray_img.rows;

  // (x1, y1) (x2, y2)为手指中间的两个点  (x3, y3)为手掌中心点
  float x1 = coordinate[1][0];
  float y1 = coordinate[1][1];
  float x2 = coordinate[2][0];
  float y2 = coordinate[2][1];
  float x3 = coordinate[0][0];
  float y3 = coordinate[0][1];

  float x4, y4; // ROI四个角点中下边两个点其中一个
  float x5, y5; // ROI四个角点中上边两个点其中一个
  float k2, b2;

  float x0 = (x1 + x2) / 2;
  float y0 = (y1 + y2) / 2;

  float unit_len = sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
  float k1 = (y1 - y2) / ((x1 - x2) + 1e-9);
  float b1 = y1 - k1 * x1;

  if (y1 == y2) {
    x4 = x0;
    y4 = y0 + unit_len / 2;
    k2 = 0;
  } else {
    k2 = (-1) / (k1 + 1e-9);
    b2 = y0 - k2 * x0;

    float aa1 = 1 + pow(k2, 2);
    ;
    float bb1 = 2 * (-x0 + k2 * b2 - k2 * y0);
    float cc1 = pow((x0), 2) + pow((b2 - y0), 2) - pow(unit_len, 2) / 4;
    float temp = sqrt(pow((bb1), 2) - 4 * aa1 * cc1);
    float x4_1 = (-bb1 + temp) / (2 * aa1);
    float y4_1 = k2 * x4_1 + b2;
    float x4_2 = (-bb1 - temp) / (2 * aa1);
    float y4_2 = k2 * x4_2 + b2;

    // 将x4赋值为 使roi尽可能小的点
    if ((pow((x3 - x4_1), 2) + pow((y3 - y4_1), 2)) <
        (pow((x3 - x4_2), 2) + pow((y3 - y4_2), 2))) {
      x4 = x4_1;
      y4 = y4_1;
    } else {
      x4 = x4_2;
      y4 = y4_2;
    }
  }

  float b4 = y4 - k1 * x4;
  float aa2 = 1 + pow(k1, 2);
  float bb2 = 2 * (-x4 + k1 * b4 - k1 * y4);
  float cc2 = pow((x4), 2) + pow((b4 - y4), 2) - pow(unit_len, 2);

  float x5_1 = (-bb2 + sqrt(pow((bb2), 2) - 4 * aa2 * cc2)) / (2 * aa2);
  float y5_1 = k1 * x5_1 + b4;
  float x5_2 = (-bb2 - sqrt(pow((bb2), 2) - 4 * aa2 * cc2)) / (2 * aa2);
  float y5_2 = k1 * x5_2 + b4;

  // 将x5赋值为 使roi尽可能小的点
  if (x5_1 < x5_2) {
    x5 = x5_1;
    y5 = y5_1;
  } else {
    x5 = x5_2;
    y5 = y5_2;
  }
  if (y1 == y2) {
      cv::Mat roi = gray_img(cv::Rect(int(x5), int(y5), int(unit_len * 2), int(unit_len * 2)));
      cv::resize(roi, roi_img_, cv::Size(roi_img_size_, roi_img_size_), 0, 0,cv::INTER_CUBIC);

      // 计算矩形的四个顶点，该条件不涉及旋转
      cv::Point topRight(x5 + unit_len * 2, y5);
      cv::Point bottomRight(x5 + unit_len * 2, y5 + unit_len * 2);
      cv::Point bottomLeft(x5, y5 + unit_len * 2);
      roi_rect_ = {cv::Point(x5, y5), topRight, bottomRight, bottomLeft};

      roi_fail_ = false;
  } else {
      float angle = atan(1 / k2);
      float angle_degree = -angle * 180 / PI;
      cv::Point2f center(W / 2, H / 2);
      cv::Mat M(2, 3, CV_32FC1);
      M = getRotationMatrix2D(center, angle_degree, 1); // 旋转矩阵
      cv::warpAffine(gray_img, gray_img, M,
                    cv::Size(W, H)); // 仿射变换——这里为旋转

      int new_x5 = static_cast<int>(x5 * std::cos(angle) - y5 * std::sin(angle) -
                                    (W / 2) * std::cos(angle) +
                                    (H / 2) * std::sin(angle) + W / 2);
      int new_y5 = static_cast<int>(x5 * std::sin(angle) + y5 * std::cos(angle) -
                                    (W / 2) * std::sin(angle) -
                                    (H / 2) * std::cos(angle) + H / 2);
      if (new_x5 < 0 || new_y5 < 0 || (new_x5 + unit_len * 2 > W) ||
          (new_y5 + unit_len * 2 > H)) {
        roi_img_ = cv::Mat::zeros(roi_img_size_, roi_img_size_, CV_8UC1);
        roi_fail_ = true;
      } else {
        cv::Mat roi = gray_img(cv::Rect(new_x5, new_y5, int(unit_len * 2), int(unit_len * 2)));
        cv::resize(roi, roi_img_, cv::Size(roi_img_size_, roi_img_size_), 0, 0,cv::INTER_CUBIC);

        // 计算旋转前的矩形四个顶点
        cv::Point topRight(x5 + unit_len * 2 * std::cos(-angle),
                          y5 + unit_len * 2 * std::sin(-angle));
        cv::Point bottomRight(topRight.x - unit_len * 2 * std::sin(-angle),
                              topRight.y + unit_len * 2 * std::cos(-angle));
        cv::Point bottomLeft(x5 - unit_len * 2 * std::sin(-angle),
                            y5 + unit_len * 2 * std::cos(-angle));
        roi_rect_ = {cv::Point(x5, y5), topRight, bottomRight, bottomLeft};

        roi_fail_ = false;
      }
  }
}

/**
 * @brief 将输入向量进行归一化处理。
 *
 * 该函数接受一个浮点数向量 `code`，计算其 2-范数（即向量的欧几里得长度），
 * 并将向量中的每个元素除以该范数，从而归一化向量。归一化后的向量将具有
 * 单位长度（即长度为1），适用于各种需要规范化输入特征的应用场景。
 *
 * @param code 输入的浮点数向量，表示待归一化的数据。
 * @return std::vector<float> 归一化后的浮点数向量。
 */
std::vector<float> CNet::normalize(const std::vector<float> &code) {
  
  std::vector<float> normalized_code(code.size());

  float norm = 0.0;
  for (const float &value : code)
    norm += value * value;
  norm = sqrt(norm);

  // 避免2-范数为0的情况
  if (norm == 0.0)
    norm = 1e-5;

  for (int i = 0; i < code.size(); i++)
    normalized_code[i] = code[i] / norm;

  return normalized_code;
}

// 将单通发哦灰度图转化为三通道灰度图 三个通道相同 就是方便后序直接使用三通道输入函数
cv::Mat CNet::addChannels(cv::Mat &img) {
  cv::Mat temp_img(img.rows, img.cols, CV_8UC3);
  cv::Mat channels[] = {img, img, img};
  cv::merge(channels, 3, temp_img);
  return temp_img;
}

//***************************接口函数****************************
// 手掌检测，如果训练一个轻量的手掌检测网络会更好
// 检测到手掌返回 false
bool CNet::palmDetect(cv::Mat &ir_frame) {
  // 首先进行 yolo 得到 outs_
  getYoloOutput(ir_frame);

  // 遍历 outs_ 寻找手掌
  for (const cv::Mat &outs : this->outs_) {
    // 随后遍历每一个对象的所有检测框 每一行都是一个检测框 [x, y, w, h, __, 手指关键点置信度, 手掌关键点置信度]
    for (int i = 0; i < outs.rows; ++i) {
      cv::Mat detection = outs.row(i);
      cv::Mat scores = detection.colRange(5, outs.cols);
      cv::Point classIdPoint;
      double confidence;
      // 该函数需要 double 数据类型
      cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);

      if (static_cast<float>(confidence) > conf_thres_) {
        if(classIdPoint.x == 1) return false;
      }
    }
  }
  return true;
}

// roi检测+提取核心程序 返回roi提取是否成功 即roi_fail_标志位
bool CNet::roiDetect(cv::Mat &ir_frame) {
  // 局部变量定义
  float coordinate[3][2];
  int dfg = 0, pc = 0;
  std::vector<int> classIds;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;

  // 首先进行 yolo 得到 outs_
  getYoloOutput(ir_frame);

  // 首先遍历每一个检测对象
  for (const cv::Mat &outs : this->outs_) {
    // 随后遍历每个对象的所有检测框 每一行都是一个检测框 [x, y, w, h, __,
    // 手指关键点置信度, 手掌关键点置信度]
    for (int i = 0; i < outs.rows; ++i) {
      cv::Mat detection = outs.row(i);
      cv::Mat scores = detection.colRange(5, outs.cols);
      cv::Point classIdPoint;
      double confidence;
      // 该函数需要 double 数据类型
      cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);

      if (static_cast<float>(confidence) > conf_thres_) {
        int centerX =static_cast<int>(detection.at<float>(0, 0) * ir_frame.cols);
        int centerY =static_cast<int>(detection.at<float>(0, 1) * ir_frame.rows);
        int width = static_cast<int>(detection.at<float>(0, 2) * ir_frame.cols);
        int height =static_cast<int>(detection.at<float>(0, 3) * ir_frame.rows);
        int left = static_cast<int>(centerX - width / 2);
        int top = static_cast<int>(centerY - height / 2);

        classIds.push_back(classIdPoint.x);
        confidences.push_back(static_cast<float>(confidence));
        boxes.push_back(cv::Rect(left, top, width, height));
      }
    }
  }

  // nms非极大值抑制 去除重复框
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, confidences, conf_thres_, nms_thres_, indices);

  for (const int &idx : indices) {
    cv::Rect box = boxes[idx];
    // 如果已经找到了1个手掌和2个手指，则跳出循环
    if (pc == 1 && dfg == 2)
      break;
    // 如果检测到手掌，则记录坐标点
    if (classIds[idx] == 1 && pc < 1) {
      coordinate[pc][0] = box.x + box.width / 2;
      coordinate[pc][1] = box.y + box.height / 2;
      points_[pc] = cv::Point(coordinate[pc][0], coordinate[pc][1]);
      ++pc;
    }
    // 如果检测到手指，则记录坐标点
    else if (classIds[idx] != 1 && dfg < 2) {
      coordinate[dfg][0] = box.x + box.width / 2;
      coordinate[dfg][1] = box.y + box.height / 2;
      points_[dfg] = cv::Point(coordinate[dfg][0], coordinate[dfg][1]);
      ++dfg;
    }
  }
  //********** 手掌有无判断 *********
  // 没有检测到三个关键点
  if (pc != 1 || dfg != 2) {
    this->roi_fail_ = true;
    this->roi_img_ =
        cv::Mat::zeros(roi_img_size_, roi_img_size_, ir_frame.type());
    points_ = std::vector<cv::Point>(3, cv::Point(0.0, 0.0));
  }
  // 检测到三个关键点
  else {
    // XXX：距离筛选不合理
    // 距离筛选
    float dis = sqrt(pow((coordinate[2][0] - coordinate[1][0]), 2) +
                     pow((coordinate[2][1] - coordinate[1][1]), 2));
    if (dis > 80) {
      // std::cout << "\n手掌放置位置太近，请将手掌放至距离摄像头15-25cm处！\n"
      // << '\n';
      roi_fail_ = true;
    } else if (dis < 50) {
      // std::cout << "\n手掌放置位置太远，请将手掌放至距离摄像头15-25cm处！\n"
      // << '\n';
      roi_fail_ = true;
    } else {
      // 距离合格
      extractRoi(ir_frame, coordinate);

      // roi初步提取成功
      if (!roi_fail_) {
        // 曝光筛选
        cv::Scalar gray_scalar = cv::mean(roi_img_);
        if (gray_scalar.val[0] > 250 || gray_scalar.val[0] < 90) {
          roi_fail_ = true;
          // std::cout << "\n图像曝光异常，请调整摄像头位置\n" << '\n';
        }
        // roi提取合格
        else {
          // 将灰度roi转化成三通道灰度格式 准备用于推理模型输入
          // roi_img_ = add_channels(roi_img_);
          cv::cvtColor(roi_img_, roi_img_, cv::COLOR_BGR2GRAY);
        }
      }
    }
  }
  return roi_fail_;
}

std::vector<float> CNet::getFeature() {
  // 处理roi_img_成模型需要的输入格式
  cv::dnn::blobFromImage(roi_img_, blob_feature_, 1.0 / 255.0, cv::Size(roi_img_size_, roi_img_size_),
                         cv::Scalar(0, 0, 0), true, false);

  // 推理模型得到特征向量
  feature_net_.setInput(blob_feature_);
  feature_ =  feature_net_.forward();

  // 归一化特征向量
  feature_ = normalize(feature_);
  return feature_;
}















