#include "Camera.h"

CCamera::CCamera() {
  ir_cap_.open(2);
  if (!ir_cap_.isOpened()) {
    std::cout<<0<<std::endl;
    std::cerr << "\nIR摄像头打开失败，请检查"
              << "\n";
    // std::cerr << "IR摄像头打开失败，错误代码: " << cv::getBuildInformation()
    //           << "\n";
    exit(-1);
  }
  // 设置读取图像的帧率、对比度、伽马、编码格式和分辨率
  ir_cap_.set(cv::CAP_PROP_FPS, 30);
  ir_cap_.set(cv::CAP_PROP_CONTRAST, 45);
  ir_cap_.set(cv::CAP_PROP_GAMMA, 90);
  ir_cap_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'V', '2'));
  ir_cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  ir_cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
  ir_cap_ >> ir_frame_; // 读取一帧图像 用作初始化 ir_frame_
  cv::transpose(ir_frame_, ir_frame_);   // 转置
  cv::flip(ir_frame_, ir_frame_, 0);  // 垂直方向翻转

  rgb_cap_.open(0);
  if (!ir_cap_.isOpened()) {
    std::cerr << "\nRGB摄像头打开失败,请检查" << '\n';
    exit(-1); // 程序终止状态码为 -1
  }
  rgb_cap_.set(cv::CAP_PROP_FPS, 30);
  rgb_cap_.set(cv::CAP_PROP_FOURCC,
               cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
  rgb_cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  rgb_cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
  rgb_cap_ >> rgb_frame_; // 读取一帧图像 用作初始化 rgb_frame_
  cv::transpose(rgb_frame_, rgb_frame_);
  cv::flip(rgb_frame_, rgb_frame_, 0);

  std::cout << "\n摄像头初始化完成" << '\n';
}

CCamera::~CCamera() {
  // 析构函数，释放摄像头
  ir_cap_.release();
  rgb_cap_.release();
  cv::destroyAllWindows();
}

// 非初始化时，主动获取 Mat 对象 frame, 得到一帧图像
cv::Mat &CCamera::getFrame(std::string id) {
  if (id == "ir") {
    if (!ir_cap_.isOpened()) {
      std::cerr << "\nIR摄像头未打开,请检查" << '\n';
      exit(-1); // 程序终止状态码为 -1
    }
    if (ir_frame_.empty()) {
      std::cerr << "\nIR摄像头无图像" << '\n';
      exit(-2); // 程序终止状态码为 -2
    }
    ir_cap_ >> ir_frame_;
    cv::transpose(ir_frame_, ir_frame_);
    cv::flip(ir_frame_, ir_frame_, 0);
    return ir_frame_;
  } else if (id == "rgb") {
    if (!rgb_cap_.isOpened()) {
      std::cerr << "\nRGB摄像头未打开,请检查" << '\n';
      exit(-1); // 程序终止状态码为 -1
    }
    if (rgb_frame_.empty()) {
      std::cerr << "\nRGB摄像头无图像" << '\n';
      exit(-2); // 程序终止状态码为 -2
    }
    rgb_cap_ >> rgb_frame_;
    cv::transpose(rgb_frame_, rgb_frame_);
    cv::flip(rgb_frame_, rgb_frame_, 0);
    return rgb_frame_;
  } else {
    std::cerr << "\nid输入错误" << '\n';
    exit(-3); // 程序终止状态码为 -3
  }
}