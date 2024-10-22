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

- 类功能：模型解密，ROI提取，特征提取
- 类入口：CCamera类图像流，AESCoder解密方法
- 类出口：roi图像、roi提取失败标志位、roi特征向量、关键点坐标

### CPalm类

- 类功能：汇总其余所有类，提供注册、识别等方法
- 类入口：CCamera类图像流，CNet类特征向量、roi标志位等
- 类出口：注册、识别、查看数据库、退出方法

### main.cpp

- 主函数



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