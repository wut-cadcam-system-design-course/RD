FROM ubuntu:22.04

# install dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata
RUN apt-get install -y --no-install-recommends \
      curl \
      gcc \
      g++ \
      gdb \
      make \
      ninja-build \
      valgrind \
      autoconf \
      automake \
      libtool \
      libssl-dev \
      ca-certificates \
      libfreetype6-dev \
      libx11-dev \
      libxrandr-dev \
      libfontconfig1-dev \
      libglu1-mesa-dev \
      freeglut3-dev \
      libxinerama-dev \
      libxcursor-dev \
      libxi-dev \
      libgl1-mesa-dev \
      libglew-dev \
      libglm-dev \
      x11-apps \
      xauth \
      libglfw3-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# install cmake 3.27
WORKDIR /root
RUN mkdir temp
WORKDIR /root/temp
RUN curl -OL https://github.com/Kitware/CMake/releases/download/v3.27.4/cmake-3.27.4.tar.gz
RUN tar -xzvf cmake-3.27.4.tar.gz

WORKDIR /root/temp/cmake-3.27.4
RUN ./bootstrap --prefix=/usr -- -DCMAKE_BUILD_TYPE:STRING=Release
RUN make -j$(nproc)
RUN make install

WORKDIR /root
RUN rm -rf temp

# project setup
ENV APP_DIR=/usr/work/rd
WORKDIR $APP_DIR
COPY . .

RUN chmod +x ./external/occt-install.sh && \
     if [ ! -d "$APP_DIR/external/occt-install" ]; then ./occt-install.sh; fi

ENV OpenCASCADE_DIR=${APP_DIR}/external/occt-install/lib/cmake/opencascade

