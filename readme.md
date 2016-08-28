Description
================
Small and fast kernel driver.

Gives you the depth data from kinect sensors.

Also runs on embedded devices like the raspberry pi.


Install
===================

PC
--
```Bash
make
make load
```

raspberry pi
-------------
```Bash 
make
make load
```

On PC for raspberry pi:
-----------------------
```Bash
make arm
scp * pi@pi:
ssh pi@pi
make load
```

Environment
=========
If you compile the code on the device you want to use it on, you need the kernel source installed and some basic setup. Just search for a guide to compile kernel modules on your OS.

If you want to compile it for a different architecture (PC to pi) you also need a cross compiler. 
Look for a guide to cross compile a kernel and modules for a pi.

Example for Ubuntu 14.04
--------------------------------

```Bash
sudo apt-get install git build-essential

git clone https://github.com/xxorde/librekinect.git
cd librekinect

make
make load
```

Example for the raspberry pi
--------------------------------------------
```Bash
sudo -s
apt-get install build-essential bc ncurses-dev tmux git

tmux
cd /usr/src/

# find out which kernel you are using – in my case 3.12.20+
uname -r
# get the source – in my case 3.12.y (change if needed) 
wget https://github.com/raspberrypi/linux/archive/rpi-3.12.y.tar.gz
tar xfvz rpi-3.12.y.tar.gz
mv linux-rpi-3.12.y linux

ln -s /usr/src/linux /lib/modules/$(uname -r)/build
ln -s /usr/src/linux /lib/modules/$(uname -r)/source 
cd /usr/src/linux

make mrproper

# If using Kernel >= 4.x.x execute:
sudo modprobe configs

# get your config
gzip -dc /proc/config.gz > .config

# building, that is going to take a while!
make
make modules_prepare
make modules_install

# copy the new kernel image
cp /usr/src/linux/arch/arm/boot/zImage /boot/linux-3.12.y

# choose it
echo "kernel=linux-3.12.y" >> /boot/config.txt

reboot
```
after reboot
```Bash
git clone https://github.com/xxorde/librekinect.git
cd librekinect

make
make load
```
That worked on my pi with the current version of Raspbian. It will take some time, if you loose the connection use "tmux attach".

If you have a faster way, let me know!

Usage
=====
After loading the modules you should have a new "/dev/videoX" which you can use like a web cam.

For example:
```Bash
camorama -d /dev/video0
vlc v4l:///dev/video0
```

This is the standard in linux. You can use it with virtually every program or library, for example OpenCV. 

This looks complicated!
=======================
But it's not! Try it!


Troubleshooting
===============


I have a /dev/video0 but I do not get data
------------------------------------------

1. Check your kinect power supply. It needs additional 12V.

2. Maybe it takes to much from the raspberry's 5V too, try an active USB hub.


"make" fails
-------------


1. Check if you have the needed tools (gcc ect.)

2. Do you have the kernel sources? Are they at the right place?



"make load" fails
------------------

Is the kernel compatible to the sources you use? Compile a kernel and load it.
