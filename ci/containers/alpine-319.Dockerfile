# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM docker.io/library/alpine:3.19

RUN apk update && \
    apk upgrade && \
    apk add \
        autoconf \
        automake \
        busybox \
        ca-certificates \
        ccache \
        gcc \
        gettext \
        git \
        libtool \
        libvirt-dev \
        libxml2-dev \
        libxml2-utils \
        libxslt \
        make \
        musl-dev \
        php83-dev \
        pkgconf \
        tar \
        xz && \
    apk list | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
