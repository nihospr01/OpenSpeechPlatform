FROM ubuntu

ENV TERM=xterm-256color

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
  apt-utils \
  build-essential \
  crossbuild-essential-arm64 \
  bash-completion \
  screen \
  man-db \
  pkg-config-aarch64-linux-gnu \
  gcc-aarch64-linux-gnu \
  g++-aarch64-linux-gnu \
  cpp-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu \
  flex \
  bison \
  automake \
  libtool \
  git \
  vim \
  bc \
  python \
  libfdt-dev \
  sudo \
  libncurses5-dev \
  dialog \
  android-tools-adb \
  android-tools-fastboot \
  cmake \
  cmake-doc \
  ninja-build \
  lrzip \
  simg2img \
  img2simg \
  initramfs-tools \
  android-tools-mkbootimg \
  ccache \
  distcc \
  libssl-dev

#RUN echo '[ -r /opt/osp/share/scripts/osp.sh ] && source /opt/osp/share/scripts/osp.sh' > /etc/profile.d/ospenv.sh

WORKDIR /opt/osp/var/build
ENV OSP_FUNC=buildHelp

#ENTRYPOINT ["/bin/bash", "-l"]
ENTRYPOINT ["/opt/osp/share/scripts/osp.sh"]
