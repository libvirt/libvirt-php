# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool dockerfile fedora-rawhide libvirt+dist,libvirt-php
#
# https://gitlab.com/libvirt/libvirt-ci/-/commit/740f5254f607de914a92d664196d045149edb45a
FROM registry.fedoraproject.org/fedora:rawhide

RUN dnf install -y nosync && \
    echo -e '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf update -y --nogpgcheck fedora-gpg-keys && \
    nosync dnf update -y && \
    nosync dnf install -y \
        autoconf \
        automake \
        ca-certificates \
        ccache \
        gcc \
        gettext-devel \
        git \
        glibc-langpack-en \
        libtool \
        libvirt-devel \
        libxml2 \
        libxml2-devel \
        libxslt \
        make \
        php-devel \
        php-pecl-imagick \
        pkgconfig \
        rpm-build && \
    nosync dnf autoremove -y && \
    nosync dnf clean all -y && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
