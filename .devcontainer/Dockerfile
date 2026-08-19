FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Standard build toolchain + Python/matplotlib (if using matplotlib-cpp for Task 4 plots)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    g++ \
    gdb \
    git \
    ninja-build \
    python3 \
    python3-pip \
    python3-matplotlib \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Non-root user setup
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -s /bin/bash \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
WORKDIR /workspace