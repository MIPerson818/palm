# 使用 Ubuntu 2004作为基础镜像
FROM ubuntu:20.04

# 设置非交互模式
ENV DEBIAN_FRONTEND=noninteractive

# 设置工作目录
WORKDIR /home/workspace

COPY ./sources.list /etc/apt/sources.list

# 更新包列表并安装基础工具
RUN set -eux \
    && apt-get update \
    && apt-get -yq upgrade \
    && apt-get -yq install \
    build-essential \
    wget curl \
    git \
    vim fim\
    sudo \
    lsb-release \
    openssh-server \
    software-properties-common \
    gnupg2 \
    python3-pip \
    libssl-dev \
    ca-certificates \
    ninja-build \
    openssh-server \
    sudo \
    pkg-config \
    valgrind \
    tini \
    clang \
    clangd \
    lldb \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# 安装 Conan 2.x 包管理器
RUN pip3 install Conan2

# 设置 C++ 编译环境，使用 clang 作为默认编译器
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

# 清理临时文件
RUN apt-get clean && rm -rf /var/lib/apt/lists/*


# 配置环境变量
ENV CC=clang
ENV CXX=clang++

# 将容器内的 clangd 与 lldb 设置为 VS Code Remote C++ 插件的调试工具
RUN ln -s /usr/bin/clangd /usr/local/bin/clangd \
    && ln -s /usr/bin/lldb /usr/local/bin/lldb

# 安装完成后展示版本信息
RUN clang --version && lldb --version && cmake --version && conan --version

# 设置 Conan 默认配置
RUN conan profile detect --force

# 清理临时文件
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/MIPerson818/palm.git \
    && cd palm \
    && mkdir build 

# 设置 Tini 作为 init 进程
ENTRYPOINT ["/usr/bin/tini", "--"]

# 设置默认命令
CMD ["/bin/bash", "-l"]

# 指定镜像名称
LABEL version="4.9" description="C++ clangd lldb Cmake Conan2 + OpenCV development environment"

