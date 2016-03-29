#configuration
CONFIG +=  def_files_disabled no_mocdepend release stl qt_no_framework
QT_ARCH = arm
QT_EDITION = OpenSource
QT_CONFIG +=  minimal-config small-config medium-config large-config full-config no-pkg-config qt3support accessibility embedded reduce_exports ipv6 clock-gettime clock-monotonic mremap getaddrinfo ipv6ifname getifaddrs inotify png freetype zlib nis multimedia audio-backend svg script scripttools declarative release private_tests

#versioning
QT_VERSION = 4.8.0
QT_MAJOR_VERSION = 4
QT_MINOR_VERSION = 8
QT_PATCH_VERSION = 0

#namespaces
QT_LIBINFIX = 
QT_NAMESPACE = 
QT_NAMESPACE_MAC_CRC = 

QMAKE_RPATHDIR += "/opt/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/lib"
