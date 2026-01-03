# 第一阶段：构建基础环境
FROM ubuntu:24.04 AS builder-base

# 1. 设置APT镜像源加速
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        wget gnupg ca-certificates \
        clang-19 lld-19 libc++-19-dev libc++abi-19-dev \
        mingw-w64 g++-mingw-w64 \
        tar git curl \
        && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100 && \
    ln -sf /usr/bin/ld.lld-19 /usr/bin/ld.lld

RUN wget -O /usr/local/bin/xmake \
    https://git.fig-lang.cn/PuqiAR/xmake-binary-copy/raw/commit/989d1f2dabb0bc8d5981a5f900c2cf7c2ac78ee4/xmake-bundle-v3.0.5.linux.x86_64 && \
    chmod +x /usr/local/bin/xmake

# 第二阶段：创建最终镜像
FROM ubuntu:24.04

# 【关键修复】在最终镜像中安装CI必需的运行时工具
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        git tar curl ca-certificates \
        && rm -rf /var/lib/apt/lists/*

# 1. 复制编译器、xmake和库文件
COPY --from=builder-base /usr/local/bin/xmake /usr/local/bin/xmake
COPY --from=builder-base /usr/bin/clang++ /usr/bin/clang++
COPY --from=builder-base /usr/bin/clang /usr/bin/clang
COPY --from=builder-base /usr/bin/ld.lld /usr/bin/ld.lld
COPY --from=builder-base /usr/x86_64-w64-mingw32 /usr/x86_64-w64-mingw32
COPY --from=builder-base /usr/lib/llvm-19 /usr/lib/llvm-19

# 2. 创建非root用户
RUN useradd -m -s /bin/bash builder

USER builder
WORKDIR /home/builder

# 3. 验证环境
RUN xmake --version | head -1 && \
    clang++ --version | head -1 && \
    git --version && \
    echo "✅ CI环境就绪"