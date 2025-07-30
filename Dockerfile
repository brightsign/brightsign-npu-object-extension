# BrightSign YOLO Object Detection - Build Environment
# Multi-stage build for efficient cross-compilation

# Build with:
# docker build --rm --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) --build-arg BRIGHTSIGN_OS_VERSION=9.1.52 -t yolo-build .

FROM ubuntu:20.04 AS base

# Arguments for configuration
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USERNAME=builder
ARG BRIGHTSIGN_OS_VERSION=9.1.52
ARG DEBIAN_FRONTEND=noninteractive

# Configure timezone and locales
ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install build dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    # Cross-compilation tools
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    # Development libraries
    libopencv-dev \
    libboost-all-dev \
    libjpeg-dev \
    libpng-dev \
    # Python for build scripts
    python3 \
    python3-pip \
    # Archive tools
    tar \
    gzip \
    bzip2 \
    xz-utils \
    zip \
    unzip \
    # Utilities
    sudo \
    locales \
    rsync \
    bc \
    file \
    && rm -rf /var/lib/apt/lists/*

# Configure locales
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# Create user with the same UID/GID as the host user
RUN groupadd -g ${GROUP_ID} ${USERNAME} && \
    useradd -m -u ${USER_ID} -g ${USERNAME} -s /bin/bash ${USERNAME} && \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Download and extract BrightSign OS sources if not provided
WORKDIR /opt/brightsign
COPY brightsign-${BRIGHTSIGN_OS_VERSION}-src-*.tar.gz* ./

# Extract BrightSign OS sources if archives are present
RUN if [ -f "brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz" ] && \
       [ -f "brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz" ]; then \
        echo "Extracting BrightSign OS sources..." && \
        tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz && \
        tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz && \
        rm -f brightsign-${BRIGHTSIGN_OS_VERSION}-src-*.tar.gz; \
    else \
        echo "BrightSign OS source archives not found, downloading..." && \
        wget -q https://brightsignbiz.s3.amazonaws.com/firmware/opensource/${BRIGHTSIGN_OS_VERSION%.*}/${BRIGHTSIGN_OS_VERSION}/brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz && \
        wget -q https://brightsignbiz.s3.amazonaws.com/firmware/opensource/${BRIGHTSIGN_OS_VERSION%.*}/${BRIGHTSIGN_OS_VERSION}/brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz && \
        tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-dl.tar.gz && \
        tar -xzf brightsign-${BRIGHTSIGN_OS_VERSION}-src-oe.tar.gz && \
        rm -f brightsign-${BRIGHTSIGN_OS_VERSION}-src-*.tar.gz; \
    fi

# Install SDK if present
WORKDIR /opt
COPY brightsign-x86_64-cobra-toolchain-*.sh* ./
COPY sdk/ ./sdk/

# Install SDK if installer is present, otherwise use existing SDK
RUN if [ -f "brightsign-x86_64-cobra-toolchain-${BRIGHTSIGN_OS_VERSION}.sh" ]; then \
        echo "Installing BrightSign SDK..." && \
        chmod +x brightsign-x86_64-cobra-toolchain-${BRIGHTSIGN_OS_VERSION}.sh && \
        ./brightsign-x86_64-cobra-toolchain-${BRIGHTSIGN_OS_VERSION}.sh -d /opt/sdk -y && \
        rm -f brightsign-x86_64-cobra-toolchain-${BRIGHTSIGN_OS_VERSION}.sh; \
    elif [ -d "sdk/sysroots" ]; then \
        echo "Using existing SDK directory..."; \
    else \
        echo "WARNING: No SDK found. Build may fail."; \
    fi

# Setup environment for cross-compilation
ENV SDK_PATH=/opt/sdk
ENV PATH=${SDK_PATH}/sysroots/x86_64-oesdk-linux/usr/bin:${PATH}
ENV PATH=${SDK_PATH}/sysroots/x86_64-oesdk-linux/usr/bin/aarch64-oe-linux:${PATH}

# Copy RKNN libraries if available
COPY toolkit/rknn-toolkit2/rknn-toolkit-lite2/lib/linux/aarch64/* /opt/rknn-libs/ 2>/dev/null || true
COPY include/librknnrt.so /opt/rknn-libs/ 2>/dev/null || true

# Set working directory
WORKDIR /workspace

# Switch to non-root user
USER ${USERNAME}

# Source SDK environment on entry
RUN echo 'if [ -f /opt/sdk/environment-setup-aarch64-oe-linux ]; then . /opt/sdk/environment-setup-aarch64-oe-linux; fi' >> ~/.bashrc

# Default command
CMD ["/bin/bash"]