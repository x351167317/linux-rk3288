#!/bin/bash -e

# Directory contains the target rootfs
TARGET_ROOTFS_DIR="binary"

if [ ! $ARCH ]; then
	ARCH='arm64'
fi
if [ ! $VERSION ]; then
	VERSION="debug"
fi

if [ ! -e linaro-stretch-alip-*.tar.gz ]; then
	echo "\033[36m Run mk-base-debian.sh first \033[0m"
fi

finish() {
	sudo umount $TARGET_ROOTFS_DIR/dev
	exit -1
}
trap finish ERR

echo -e "\033[36m Extract image \033[0m"
sudo tar -xpf linaro-stretch-alip-*.tar.gz

echo -e "\033[36m Copy overlay to rootfs \033[0m"
sudo mkdir -p $TARGET_ROOTFS_DIR/packages
sudo cp -rf packages/$ARCH/* $TARGET_ROOTFS_DIR/packages
# some configs
sudo cp -rf overlay/* $TARGET_ROOTFS_DIR/
# bt,wifi,audio firmware
sudo cp -rf overlay-firmware/* $TARGET_ROOTFS_DIR/
if [ "$VERSION" == "debug" ] || [ "$VERSION" == "jenkins" ]; then
	# adb, video, camera  test file
	sudo cp -rf overlay-debug/* $TARGET_ROOTFS_DIR/
fi

if [ "$ARCH" == "arm64"  ]; then
    sudo cp -rf overlay-debug/usr/local/share/adb/adbd-64 $TARGET_ROOTFS_DIR/usr/local/sbin/adbd
    sudo cp -rf overlay-debug/usr/local/share/adb/S60adbd $TARGET_ROOTFS_DIR/usr/local/sbin
    sudo cp -rf overlay-debug/usr/local/share/adb/libcutils.so $TARGET_ROOTFS_DIR/usr/lib
    sudo cp -rf overlay-debug/usr/local/share/adb/libcrypto.so.1.0.0 $TARGET_ROOTFS_DIR/usr/lib
fi

if [ "$VERSION" == "jenkins" ]; then
	# network
	sudo cp -b /etc/resolv.conf $TARGET_ROOTFS_DIR/etc/resolv.conf
fi

echo -e "\033[36m Change root.....................\033[0m"
sudo cp /usr/bin/qemu-aarch64-static $TARGET_ROOTFS_DIR/usr/bin/
sudo mount -o bind /dev $TARGET_ROOTFS_DIR/dev

cat <<EOF | sudo chroot $TARGET_ROOTFS_DIR

chmod o+x /usr/lib/dbus-1.0/dbus-daemon-launch-helper
apt-get update

#---------------conflict workaround --------------
apt-get remove -y xserver-xorg-input-evdev

apt-get install -y libxfont1 libinput-bin libinput10 libwacom-common libwacom2 libunwind8 xserver-xorg-input-libinput

#---------------Xserver--------------
echo -e "\033[36m Setup Xserver.................... \033[0m"
dpkg -i  /packages/xserver/*
apt-get install -f -y

#---------------Video--------------
echo -e "\033[36m Setup Video.................... \033[0m"
apt-get install -y gstreamer1.0-plugins-base gstreamer1.0-tools gstreamer1.0-alsa \
	gstreamer1.0-plugins-good  gstreamer1.0-plugins-bad alsa-utils

dpkg -i  /packages/video/mpp/*.deb
dpkg -i  /packages/video/gstreamer/*.deb
dpkg -i  /packages/workaround/*.deb
apt-get install -f -y

#------------------libdrm------------
dpkg -i  /packages/libdrm/*.deb
apt-get install -f -y

#---------------Qt-Video--------------
dpkg -l | grep lxde
if [ "$?" -eq 0 ]; then
	# if target is base, we won't install qt
	apt-get install  -y libqt5opengl5 libqt5qml5 libqt5quick5 libqt5widgets5 libqt5gui5 libqt5core5a qml-module-qtquick2 \
		libqt5multimedia5 libqt5multimedia5-plugins libqt5multimediaquick-p5
#	dpkg -i  /packages/video/qt/*
	apt-get install -f -y
else
	echo "won't install qt"
fi

#---------------Others--------------
#---------FFmpeg---------
#apt-get install -y libsdl2-2.0-0 libcdio-paranoia1 libjs-bootstrap libjs-jquery
#dpkg -i  /packages/others/ffmpeg/*
#---------FFmpeg---------
#dpkg -i  /packages/others/mpv/*
#apt-get install -f -y

#---------------Debug-------------- 
if [ "$VERSION" == "debug" ] || [ "$VERSION" == "jenkins" ] ; then
	apt-get install -y sshfs openssh-server bash-completion
fi

#---------------Custom Script-------------- 
systemctl enable rockchip.service
systemctl mask systemd-networkd-wait-online.service
systemctl mask NetworkManager-wait-online.service
rm /lib/systemd/system/wpa_supplicant@.service

#---------------get accelerated back for chromium v61--------------
ln -s /usr/lib/aarch64-linux-gnu/libGLESv2.so /usr/lib/chromium/libGLESv2.so
ln -s /usr/lib/aarch64-linux-gnu/libEGL.so /usr/lib/chromium/libEGL.so

#---------------Clean-------------- 
rm -rf /var/lib/apt/lists/*

EOF

sudo umount $TARGET_ROOTFS_DIR/dev
