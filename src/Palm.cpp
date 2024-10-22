#include "Palm.h"

CPalm::CPalm() {

  //摄像头初始化
  camera_ = new CCamera;

  //模型初始化
  cnet_ = new CNet;
  
  //加载数据库
  userdata_path_ = R"(../res/data.dat)";
  match_thres_ = 0.92;
  userDataload(userdata_path_);

  // 参数设置
  palm_fail_ = true;
  img_scalar_fail_ = true;

  // user初始化
  user_.name = "";
  user_.feature.resize(128,0.0);
}

CPalm::~CPalm() {
  if (camera_) {
    delete camera_;
    camera_ = nullptr;
  }
  if (cnet_) {
    delete cnet_;
    cnet_ = nullptr;
  }
}

void CPalm::userDataload(std::string& userdata_path) {
  std::ifstream fin;

  // 以二进制形式读取userdata文件
  fin.open(userdata_path, std::ios::in | std::ios::binary);

  if (!fin.is_open()) {

    if (fin.fail() && !fin.bad()) {
      // 文件打开失败或读取失败，但并非文件系统故障或设备故障
      std::cout<<"\n数据文件不存在 已创建"<<"\n";
      std::ofstream createFile(userdata_path);
    }

    // 文件存在，但无法打开
    else {
      std::cout<<"\n数据文件无法打开 请检查"<<"\n";
      exit(-1);
    }
  }

  // 先判断文件是否为空
  if(fin.peek() == std::ifstream::traits_type::eof()) {
    std::cout<<"\n数据文件为空"<<"\n";
    // fin.close();
    return;
  }

  // 读到内存中，储存在user_datadbase_中
  while (true) {
    UserData user;
    int name_length, feature_size;

    fin.read(reinterpret_cast<char *>(&name_length), sizeof(name_length));

    // 注意位置 EOF在执行read函数后才改变，这行代码必须在这里
    if (fin.eof())
      break;

    user.name.resize(name_length);
    fin.read(&user.name[0], name_length);

    fin.read(reinterpret_cast<char *>(&feature_size), sizeof(feature_size));
    user.feature.resize(feature_size);
    fin.read(reinterpret_cast<char *>(user.feature.data()),
             sizeof(float) * feature_size);

    user_database_.push_back(user);
  }

  fin.close();
  std::cout<<"\n数据库加载成功"<<"\n";
}

// 保存更新后的数据库
// 写入二进制数据有学问，不要直接使用sizeof写入结构体，因为UserData成员都是动态扩展内存

void CPalm::userDataSave(std::string &userdata_path) {
  std::ofstream fout;

  // 设置清空原文件数据并以二进制写入
  fout.open(userdata_path, std::ios::out | std::ios::binary);
  if (!fout.is_open()) {
    std::cout << "\n数据保存失败" << '\n';
    exit(1);
  }

  for (const UserData &data : user_database_) {
    // 先写入用户名长度和C风格用户名
    int name_length = data.name.length();
    fout.write(reinterpret_cast<const char *>(&name_length),
               sizeof(name_length));
    fout.write(data.name.c_str(), name_length);

    // 写入特征向量大小和特征向量
    int feature_size = data.feature.size();
    fout.write(reinterpret_cast<const char *>(&feature_size),
               sizeof(feature_size));
    fout.write(reinterpret_cast<const char *>(data.feature.data()),
               sizeof(float) * feature_size);
  }

  fout.close();
}

// 检查输入的用户名是否合法
bool CPalm::userNameCheck(std::string &username) {
  // 不能为纯数字
  if (std::all_of(username.begin(), username.end(),
                  [](char c) { return std::isdigit(c); })) {
    std::cout << "\n用户名不能为纯数字" << '\n';
    return false;
  }

  // 不能过长或过短
  if (username.size() > 12 || username.size() < 2) {
    std::cout << "\n用户名长度不符合要求" << '\n';
    return false;
  }

  // 不能包含除下划线以外的特殊符号
  if (std::any_of(username.begin(), username.end(),
                  [](char c) { return !std::isalnum(c) && c != '_'; })) {
    std::cout << "\n用户名不能包含除下划线以外的特殊符号" << '\n';
    return false;
  }

  // 不能与数据库存在同名注册
  auto it = std::find_if(
      user_database_.begin(), user_database_.end(),
      [&username](const UserData &data) { return data.name == username; });
  if (it != user_database_.end()) {
    std::cout << "\n用户名已存在" << '\n';
    return false;
  }

  return true;
}

// 使用匹配，返回索引
// XXX: 用户规模很大时，将用户数据加载到内存的方案不可行，应该使用数据库技术
#ifdef USE_MAP
int CPalm::userfeature_match(std::vector<float> feature, float threshold) {
  // 数据库为空的情况
  if (user_database_.size() == 0)
    return -1;

  // 局部变量定义
  int index = -1;
  Eigen::Map<Eigen::VectorXf> e_feature(feature.data(), LEN_FEATURE);
  Eigen::MatrixXf e_database(user_database_.size(), LEN_FEATURE);

  int row = 0;
  for (auto it = user_database_.begin(); it != user_database_.end(); ++it) {
    int col = 0;
    const std::vector<float> &v_temp = it->second;
    for (auto v_it = v_temp.begin(); v_it != v_temp.end(); ++v_it) {
      e_database(row, col) = *v_it;
      ++col;
    }
    ++row;
  }

  // 矩阵乘法计算相似度
  Eigen::VectorXf result = e_feature * e_database.transpose();
  for (int i = 0; i < result.size(); ++i) {
    if ((1 - result[i]) < threshold) {
      index = i;
      break;
    }
  }
  return index;
}
#else
int CPalm::userFeatureMatch(std::vector<float> &feature, float &threshold) {
  // 数据库为空的情况
  if (user_database_.size() == 0)
    return -1;

  // 将vector转化成eigen向量
  Eigen::Map<Eigen::VectorXf> e_feature(feature.data(), this->LEN_FEATURE);

  // 余弦相似度
  int index = -1;
  std::vector<float> v_result;
  for (auto it = user_database_.begin(); it != user_database_.end(); ++it) {
    Eigen::Map<Eigen::VectorXf> temp_feature(it->feature.data(),
                                             this->LEN_FEATURE);
    float result = static_cast<float>(e_feature.dot(temp_feature) /
                                      (e_feature.norm() * temp_feature.norm()));
    v_result.push_back(result);
  }

  // *** 测试用 ***
  // for (auto it = v_result.begin(); it != v_result.end(); ++it) {
  //     std::cout << *it << " ";
  // }
  // std::cout << '\n';

  // 查找最大元素所在索引
  std::vector<float>::iterator max_it =
      std::max_element(v_result.begin(), v_result.end());
  if (max_it != v_result.end()) {
    if (*max_it < match_thres_)
      index = -1;
    index = std::distance(v_result.begin(), max_it);
  }
  return index; // 缺省返回-1，表示查找失败
}
#endif

// 使一些 过程中一直存在但不断更改的变量 恢复缺省值
void CPalm::membersClear() {
  cnet_->roi_img_.setTo(0);
  cnet_->roi_fail_ = true;
  user_.name = "";
  user_.feature.resize(128, 0.0);
  features_.clear();
  palm_fail_ = true;
  img_scalar_fail_ = true;
}

//*************** 接口函数 ***************
// 注册用户
void CPalm::userRegister() {
  // 用户名操作
  std::cout << '\n';
  std::cout << "注意：用户名长度2-12位字符\n"
            << "     字符包含字母、数字、特殊符号\n"
            << "     不能为纯数字\n"
            << "     特殊符号只能包含'_'" << '\n';
  std::cout << '\n';

  std::cout << "\n请输入用户名" << '\n';
  std::cout << ">>";
  std::cin >> user_.name;
  while (!userNameCheck(user_.name)) {
    std::cout << "\n用户名不合法，请重新输入" << '\n';
    std::cout << ">>";
    std::cin >> user_.name;
  }

  // roi检测
  int img_count = 0;  // 有效图像数量
  int palm_count = 0; // 手掌检测次数
  int roi_count = 0;  // roi检测次数

  std::cout << "\n正在检测手掌..." << '\n';

  // ********** 需要检测图像有效性 **********
  // 检测手掌进入 并等待相机曝光
  while (true) {
    // 手掌检测时间过长 退出识别模式
    if (palm_count >= PALM_COUNT) {
      std::cout << "\n未检测到手掌或曝光异常" << '\n';
      return;
    }
    // 检测到手掌且曝光正常退出循环
    if (!palm_fail_ && !img_scalar_fail_)
      break;

    // 必须先检测到手掌 再计算曝光是否正常
    palm_fail_ = cnet_->palmDetect(camera_->ir_frame_);

    // ***********  再计算曝光  ************
    // TODO：需要一个 动态测量曝光正常范围 的方法
    if (!palm_fail_) {
      // 休眠0.5秒，等待对焦和曝光
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      cv::Scalar gray_scalar = cv::mean(camera_->ir_frame_);
      if (gray_scalar.val[0] > 95 || gray_scalar.val[0] < 75)
        img_scalar_fail_ = true;
      else
        img_scalar_fail_ = false;
    }
    ++palm_count;
  }
  std::cout << "\n检测到手掌，请保持不要动" << '\n';

  std::cout << "\n开始注册..." << '\n';

  // roi提取
  while (true) {
    // ROI检测时间过长 退出识别模式
    if (roi_count >= ROI_COUNT) {
      std::cout << "\n没有采集到有效图像" << '\n';
      membersClear();
      return;
    }
    // 检测到一定数量的有效roi 退出循环
    if (img_count >= IMG_NUM_RECOGNIZE)
      break;

    // roi 提取成功
    if (!cnet_->roiDetect(camera_->ir_frame_)) {
      ++img_count;

      // 特征提取
      features_.push_back(cnet_->getFeature());
    }
    ++roi_count;
  }

  // 有效特征取平均
  for (const auto &feature : features_) {
    for (int i = 0; i < LEN_FEATURE; ++i) {
      user_.feature[i] += feature[i];
    }
  }
  for (int i = 0; i < LEN_FEATURE; ++i) {
    user_.feature[i] /= features_.size();
  }
  // *** 测试用 ***
  // for (auto it = user_.feature.begin(); it != user_.feature.end(); ++it) {
  //     std::cout << *it << " ";
  // }
  // std::cout << '\n';

  std::cout << "\n用户 " << user_.name << " 注册成功" << '\n';

  // 更新数据库
  user_database_.push_back(user_);
  userDataSave(userdata_path_);

  std::cout << "\n数据库已更新" << '\n';

  // 清空部分成员属性
  membersClear();
}

// 识别用户
void CPalm::userRecognize() {
  // roi检测
  int img_count = 0;  // 有效图像数量
  int palm_count = 0; // 手掌检测次数
  int roi_count = 0;  // roi检测次数

  std::cout << "\n正在检测手掌..." << '\n';

  // ********** 需要检测图像有效性 **********
  // 检测手掌进入 并等待相机曝光
  while (true) {
    // 手掌检测时间过长 退出识别模式
    if (palm_count >= PALM_COUNT) {
      std::cout << "\n未检测到手掌或曝光异常" << '\n';
      return;
    }
    // 检测到手掌且曝光正常退出循环
    if (!palm_fail_ && !img_scalar_fail_)
      break;

    // 必须先检测到手掌 再计算曝光是否正常
    palm_fail_ = cnet_->palmDetect(camera_->ir_frame_);

    // ***********  再计算曝光  ************
    // TODO：需要一个 动态测量曝光正常范围 的方法
    if (!palm_fail_) {
      // 休眠0.5秒，等待对焦和曝光
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      cv::Scalar gray_scalar = cv::mean(camera_->ir_frame_);
      if (gray_scalar.val[0] > 95 || gray_scalar.val[0] < 75)
        img_scalar_fail_ = true;
      else
        img_scalar_fail_ = false;
      // std::cout << gray_scalar.val[0] << '\n';
    }
    ++palm_count;
  }
  std::cout << "\n检测到手掌，请保持不要动" << '\n';

  // 开始计时
  auto start = std::chrono::high_resolution_clock::now();

  // roi提取
  while (true) {
    // ROI检测时间过长 退出识别模式
    if (roi_count >= ROI_COUNT) {
      std::cout << "\n没有采集到有效图像" << '\n';
      membersClear();
      return;
    }
    // 检测到一定数量的有效roi 退出循环
    if (img_count >= IMG_NUM_RECOGNIZE)
      break;

    // roi 提取成功
    if (!cnet_->roiDetect(camera_->ir_frame_)) {
      ++img_count;

      // 特征提取
      features_.push_back(cnet_->getFeature());
    }
    ++roi_count;
  }

  // 有效特征取平均
  for (const auto &feature : features_) {
    for (int i = 0; i < LEN_FEATURE; ++i) {
      user_.feature[i] += feature[i];
    }
  }
  for (int i = 0; i < LEN_FEATURE; ++i) {
    user_.feature[i] /= features_.size();
  }

  // 特征匹配
  int index = userFeatureMatch(user_.feature, match_thres_);
  auto end = std::chrono::high_resolution_clock::now();
  if (index == -1) {
    std::cout << "\n未找到该用户" << '\n';
    membersClear();
    return;
  }
  std::string temp_name = user_database_[index].name;
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "\n识别成功：用户 " << temp_name << "  用时："
            << duration.count() << " ms" << '\n';

  // 清空部分成员属性
  membersClear();
}

// 数据库操作
void CPalm::userDatabase() {}

// 退出程序
void CPalm::userQuit() {
  // 退出码为0 程序正常退出
  exit(0);
}

// 在每一帧图上画出关键点   用于子线程
const cv::Mat CPalm::getFrameWithPoints() {
  cv::Mat frame = camera_->getFrame("ir").clone();
  if (!cnet_->roi_fail_) {
    cv::circle(frame, cnet_->points_[0], 3, cv::Scalar(0, 0, 255), 4);
    cv::circle(frame, cnet_->points_[1], 3, cv::Scalar(255, 0, 0), 4);
    cv::circle(frame, cnet_->points_[2], 3, cv::Scalar(255, 0, 0), 4);
    cv::polylines(frame, cnet_->roi_rect_, true, cv::Scalar(0, 255, 0), 2);
    // *** 测试用 ***
    cv::imwrite(R"(../../res/whole.jpg)", frame);
    cv::imwrite(R"(../../res/roi.jpg)", cnet_->roi_img_);
  }
  return frame;
}