ARG BASE_IMAGE=docker.io/debian:stable-slim
ARG YASDI_VERSION=1.8.1
FROM $BASE_IMAGE as build
LABEL maintainer="Konstantin Bläsi <kblaesi@gmail.com>"

ENV YASDI_VERSION_MAJOR 1
ENV YASDI_VERSION_MINOR 8
ENV YASDI_VERSION_PATCH 1
ENV YASDI_VERSION $YASDI_VERSION_MAJOR.$YASDI_VERSION_MINOR.$YASDI_VERSION_PATCH

ENV YASDI_SRC /yasdi

# install build dependencies
RUN apt update && apt upgrade -y
RUN apt install -y build-essential cmake

RUN mkdir $YASDI_SRC
COPY ./sdk $YASDI_SRC

# build and install yasdi
RUN mkdir -p $YASDI_SRC/build
WORKDIR $YASDI_SRC/build
RUN cmake -D CMAKE_INSTALL_PREFIX=/usr -D YASDI_DEBUG_OUTPUT=0 $YASDI_SRC/projects/generic-cmake
RUN make -j$(getconf _NPROCESSORS_ONLN)
RUN make install

FROM $BASE_IMAGE as publish
COPY --from=build /usr/bin/yasdishell /usr/bin/

COPY --from=build \
    /usr/lib/libyasdi.so.1.8.1 \
    /usr/lib/libyasdi_drv_ip.so.1.8.1 \
    /usr/lib/libyasdi_drv_serial.so.1.8.1 \
    /usr/lib/libyasdimaster.so.1.8.1  /usr/lib/

RUN ln -s libyasdi.so.1.8.1 /usr/lib/libyasdi.so.1
RUN ln -s libyasdi.so.1 /usr/lib/libyasdi.so

RUN ln -s libyasdi_drv_ip.so.1.8.1 /usr/lib/libyasdi_drv_ip.so.1
RUN ln -s libyasdi_drv_ip.so.1 /usr/lib/libyasdi_drv_ip.so

RUN ln -s libyasdi_drv_serial.so.1.8.1 /usr/lib/libyasdi_drv_serial.so.1
RUN ln -s libyasdi_drv_serial.so.1 /usr/lib/libyasdi_drv_serial.so

RUN ln -s libyasdimaster.so.1.8.1 /usr/lib/libyasdimaster.so.1
RUN ln -s libyasdimaster.so.1 /usr/lib/libyasdimaster.so
