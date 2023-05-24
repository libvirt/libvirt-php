# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    apk update
    apk upgrade
    apk add \
        autoconf \
        automake \
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
        php81-dev \
        pkgconf \
        tar \
        xz
    apk list | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
