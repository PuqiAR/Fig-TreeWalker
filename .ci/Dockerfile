# 第一阶段：构建基础环境
FROM ubuntu:24.04 AS builder-base

# 1. 设置APT镜像源加速（可选）
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

# 2. 一次性安装所有系统依赖和工具链
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        wget gnupg ca-certificates \
        clang-19 lld-19 libc++-19-dev libc++abi-19-dev \
        mingw-w64 g++-mingw-w64 \
        tar git curl \
        && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100 && \
    # 创建 lld 的符号链接，确保 ld.lld 可用
    ln -sf /usr/bin/ld.lld-19 /usr/bin/ld.lld

# 3. 安装xmake
RUN wget -O /usr/local/bin/xmake \
    https://git.fig-lang.cn/PuqiAR/xmake-binary-copy/raw/commit/989d1f2dabb0bc8d5981a5f900c2cf7c2ac78ee4/xmake-bundle-v3.0.5.linux.x86_64 && \
    chmod +x /usr/local/bin/xmake

# 第二阶段：创建最终镜像
FROM ubuntu:24.04

# 1. 仅复制必要的运行时文件
COPY --from=builder-base /usr/local/bin/xmake /usr/local/bin/xmake
COPY --from=builder-base /usr/bin/clang++ /usr/bin/clang++
COPY --from=builder-base /usr/bin/clang /usr/bin/clang
COPY --from=builder-base /usr/bin/ld.lld /usr/bin/ld.lld
# 复制MinGW交叉编译器的库和头文件（关键！）
COPY --from=builder-base /usr/x86_64-w64-mingw32 /usr/x86_64-w64-mingw32
# 复制标准C++库（确保目录存在）
COPY --from=builder-base /usr/lib/llvm-19 /usr/lib/llvm-19

# 2. 创建非root用户
RUN useradd -m -s /bin/bash builder

# 3. 设置默认用户
USER builder
WORKDIR /home/builder

# 4. 验证安装（可选）
RUN xmake --version | head -1 && \
    clang++ --version | head -1 && \
    echo "构建环境就绪"