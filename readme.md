Description
================
Small and fast kernel driver.

Gives you the depth data from kinect sensors.

Also runs on embedded devices like the raspberry pi.


Environment
=========
If you compile the code on the device you want to use it on, you need the kernel source installed and some basic setup. Just search for a guide to compile kernel modules on your OS.

If you want to compile it for a different architecture (PC to pi) you also need a cross compiler. 
Look for a guide to cross compile a kernel and modules for a pi.

Example for the raspberry pi
--------------------------------------------
```Bash
apt-get install build-essential bc ncurses-dev tmux

tmux
cd /usr/src/
wget https://github.com/raspberrypi/linux/archive/rpi-3.10.y.tar.gz
mv linux-rpi-3.10.y linux
ln -s /usr/src/linux /lib/modules/3.10.25+/build
ln -s /usr/src/linux /lib/modules/3.10.25+/source

cd /lib/modules/3.10.25+/build

make mrproper
gzip -dc /proc/config.gz > .config
make modules_prepare 
```

That worked on my pi with the current version of Raspbian. It will take some time, if you loose the connection use "tmux attach".


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

Usage
=====
After loading the modules you should have a new "/dev/videoX" which you can use like a web cam.

For example:
```Bash
camorama -d /dev/video0
vlc v4l:///dev/video0
```

This looks to complicated!
=====================================
But it's not! Try it!
