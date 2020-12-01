FROM registry.opensuse.org/opensuse/leap:15.1

RUN zypper update -y && \
    zypper install -y \
           autoconf \
           automake \
           ca-certificates \
           ccache \
           gcc \
           gettext-devel \
           git \
           glibc-locale \
           libtool \
           libvirt-devel \
           libxml2 \
           libxml2-devel \
           libxslt \
           make \
           php-devel \
           php-imagick \
           pkgconfig \
           rpm-build && \
    zypper clean --all && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
