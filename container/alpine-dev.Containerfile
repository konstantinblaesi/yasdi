ARG BASE_IMAGE=docker.io/alpine:3.15.5
FROM $BASE_IMAGE

LABEL maintainer="Konstantin Bläsi <kblaesi@gmail.com>"
LABEL org.opencontainers.image.source https://github.com/konstantinblaesi/yasdi
LABEL org.opencontainers.image.description YASDI image containing yasdishell and the library

ENV YASDI_SRC /yasdi

RUN apk update && apk upgrade
# install build dependencies
RUN apk update && apk add --no-cache binutils cmake make musl-dev gcc

RUN mkdir $YASDI_SRC
COPY ./sdk $YASDI_SRC

# build and install yasdi
RUN mkdir -p $YASDI_SRC/build
WORKDIR $YASDI_SRC/build
RUN cmake -D CMAKE_INSTALL_PREFIX=/usr -D CMAKE_C_FLAGS="-DLIBC_MUSL" -D YASDI_DEBUG_OUTPUT=0 $YASDI_SRC/projects/generic-cmake
RUN make -j$(getconf _NPROCESSORS_ONLN)
RUN make install
