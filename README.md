# Palm-Local

## 项目拆解
实现掌纹的注册：
    调用摄像头；
    调用detect模型；
    保存批量的用户数据；
    数据加密保存本地；

掌纹识别阶段：
    调用摄像头；
    调用detect模型；
    数据解密；
    调用class模型，与检测的用户数据做度量学习；

## 目录说明
.
|-- bin --存放可执行文件
|-- build --Cmake构建地址
|-- conan --Conan库文件路径地址
|-- include --头文件
|-- src --源文件
|-- lib --二进制中间件
|-- res --资源储存，包含模型、数据文件
|-- conanfile.txt 
|-- CMakeLists.txt
`-- README.md

## 程序架构

### CCmera类

- 类功能：初始化摄像头，获取图像帧
- 类入口：无
- 类出口：根据字符串选择，获取原始图像

### AESCoder类

- 类功能：提供静态加密解密方法
- 类入口：无
- 类出口：提供静态方法，无需实例化

### CNet类

- 类功能：模型解密，加载网络，ROI提取，特征提取
- 类入口：CCamera类图像流，AESCoder解密方法
- 类出口：roi图像、roi提取失败标志位、roi特征向量、关键点坐标

### CPalm类

- 类功能：汇总其余所有类，提供注册、识别等方法
- 类入口：CCamera类图像流，CNet类特征向量、roi标志位等
- 类出口：注册、识别、查看数据库、退出方法

### main.cpp

- 主函数
- 用户输入选择包括：
  - 注册用户
  - 识别用户
  - 查看数据库
  - 退出程序

## 各模块函数功能

### CCmera类

1. **摄像头初始化**：构造函数 `CCamera()` 中打开两个摄像头（一个红外摄像头和一个RGB摄像头），并设置相应的参数（如帧率、对比度、分辨率等）。
2. **图像读取与处理**：通过 `getFrame(std::string id)` 方法，可以根据传入的参数（"ir" 或 "rgb"）来获取对应摄像头的图像帧，并进行图像的转置和翻转处理。
3. **资源释放**：析构函数 `~CCamera()` 在对象销毁时释放摄像头和销毁所有窗口，确保资源得到妥善处理。

### AESCOder类

1. **字符转换为字节**: `char2Byte` 函数将字符数组转换为字节数组，以便用于 AES 加密和解密。
2. **AES 加密**: `aesEncrypt` 函数接受一个明文字符串，并返回对应的密文。函数内部先将密钥和初始向量（IV）转换为字节格式，随后使用 AES 加密算法和 CBC 模式进行加密。
3. **AES 解密**: `aesDecrypt` 函数接受密文字符串，并返回解密得到的明文。与加密过程类似，首先转换密钥和 IV，然后进行解密操作。

### CNet类

1. **构造函数 CNet::CNet()**:
   - 初始化默认参数，如置信度阈值（`conf_thres_`）、非极大值抑制阈值（`nms_thres_`）、输入图像大小（`init_img_size_`）等。
   - 指定模型文件的路径，并调用 `readModel()` 函数读取和解密模型。
2. **模型解密和读取**:
   - `readModel()` 函数通过调用 `decryptModel()` 函数，解密各个模型文件（结构和权重），并在解密成功后利用 OpenCV 的 DNN 模块 readNetFromDarknet & readNetFromTensorflow 分别读取 .cfg .weights文件 和 .pb文件 并配置这些模型。
3. **获取输出层名称**:
   - `getOutlayerNames()` 函数负责获取模型的输出层名称，以便在推理时使用。
4. **模型推理**:
   - `getYoloOutput()` 函数将输入图像处理为模型需要的格式（如颜色变换和归一化），并进行前向推理，获取输出结果。
5. **手掌和手指检测**:
   - `palmDetect()` 函数利用 YOLO 模型进行手掌检测，若检测到手掌，返回 `false`（即检测成功）。
6. **ROI（Region of Interest）提取**:
   - `roiDetect()` 函数负责提取手掌和手指的ROI，进行一些有效性检查，如关键点是否齐全、距离过滤等。
7. **特征提取和处理**:
   - `getFeature()` 函数利用特征提取网络对提取的ROI进行推理，生成特征向量，并进行归一化处理。

### CPalm类

1. **用户注册 (userRegister)**：
   - 提示用户输入用户名，并检查用户名的合法性。
   - 进行手掌检测，确认是否有手掌进入并且相机曝光正常。
   - 提取区域（ROI）并进行特征提取，最后将用户特征保存到数据库中。
2. **用户识别 (userRecognize)**：
   - 检测手掌并确认曝光正常。
   - 提取ROI并进行特征提取。
   - 将提取的特征与数据库中的特征进行匹配，识别用户。
3. **数据加载与保存**：
   - 使用二进制方式从文件中加载用户数据（userdata）。
   - 允许在注册新用户后保存更新的数据到同一文件中。
4. **用户名检查 (userNameCheck)**：
   - 对用户输入用户名进行合法性检查，包括长度、字符类型及是否重复。
5. **特征匹配**：
   - 采用余弦相似度等方法进行用户特征的匹配，确定识别的用户。
6. **内部会员清理 (membersClear)**：
   - 在完成某些操作后，重置一些状态变量，确保下次操作时的环境干净。

### main.cpp

1. **多线程处理**: 通过线程技术同时展示图像并接收用户输入，保证界面响应流畅。
2. **使用智能指针**: 通过`std::unique_ptr`管理`CPalm`对象，有效避免内存泄漏。
3. **线程安全**: 采用`std::ref`确保在多线程环境中安全地使用同一个对象。

## 项目构建

### 安装

安装 clang++ llDB Conan  cmake

- 第一次运行时配置环境（build_type可选Debug或Release）：
- - conan install . -c tools.system.package_manager:mode=install --output-folder=conan --build=missing build_type=Debug 

- 之后每次编译只需执行：
- - conan install . --output-folder=conan --build=missing build_type=Debug  

### 编译

- - source conan/conanbuild.sh   # 相当于激活环境，将库添加到环境变量
- - cmake -B build -S .  -DCMAKE_BUILD_TYPE=Debug 
- - cmake --build build -j12  #多核编译


### 运行
- - source conan/conanrun.sh   # 相当于激活环境，将库添加到环境变量
- - ./bin/test

### 结束

- - source conan/deactivate_conanbuild.sh   # 恢复到原来环境

## 改进方向

### 程序角度

- 数据库直接加载到栈区，此方案只适用于小规模用户
- 使用cuda或tensorRT或openvivo加速推理
- 加入UI
- 使用枚举类型统一管理错误码
- 加入手模图片

### 算法角度

- 增加更加轻量级的手掌判断模型  用于ROI提取之前进行判断画面中有无手掌
- 距离在关键点提取到之后才能计算，不合理
- 对提取到的ROI进行图像质量评估，需要相关算法，提升图像质量
- 改善特征提取模型，使提取到的特征更加丰富