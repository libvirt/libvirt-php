# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    dnf update -y
    dnf install -y \
        autoconf \
        automake \
        ca-certificates \
        ccache \
        gawk \
        gcc \
        gettext-devel \
        git \
        glibc-devel \
        glibc-langpack-en \
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
        xz
    rpm -qa | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
