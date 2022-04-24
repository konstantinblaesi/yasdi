ARG ALPINE_TAG=3.15.4
ARG YASDI_VERSION=1.8.1
FROM docker.io/alpine:$ALPINE_TAG as build
LABEL maintainer="Konstantin Bläsi <kblaesi@gmail.com>"

ENV YASDI_VERSION_MAJOR 1
ENV YASDI_VERSION_MINOR 8
ENV YASDI_VERSION_PATCH 1
ENV YASDI_VERSION $YASDI_VERSION_MAJOR.$YASDI_VERSION_MINOR.$YASDI_VERSION_PATCH

ENV YASDI_SRC /yasdi

# install build dependencies
RUN apk update && apk add --no-cache binutils cmake make musl-dev gcc

RUN mkdir $YASDI_SRC
COPY ./sdk $YASDI_SRC

# build and install yasdi
# debug build: cmake -D YASDI_DEBUG_OUTPUT=1 ..
RUN mkdir -p $YASDI_SRC/projects/generic-cmake/build && cd $YASDI_SRC/projects/generic-cmake/build
WORKDIR $YASDI_SRC/projects/generic-cmake/build
RUN cmake -D CMAKE_INSTALL_PREFIX=/usr -D CMAKE_C_FLAGS="-DLIBC_MUSL" -D YASDI_DEBUG_OUTPUT=0 ..
RUN make -j$(getconf _NPROCESSORS_ONLN)
RUN make install

FROM alpine:$ALPINE_TAG as publish
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
