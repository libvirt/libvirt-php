# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.opensuse.org/opensuse/leap:15.5

RUN zypper update -y && \
    zypper install -y \
           autoconf \
           automake \
           bash \
           ca-certificates \
           ccache \
           gcc \
           gettext-devel \
           git \
           glibc-devel \
           glibc-locale \
           libtool \
           libvirt-devel \
           libxml2 \
           libxml2-devel \
           libxslt \
           make \
           php-devel \
           pkgconfig \
           rpm-build \
           tar \
           xz && \
    zypper clean --all && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
