What does it do?
================

It gives you the depth data from a kinect sensor.
Its small fast and runs in kernel space, that’s why it works on many low power devices like the raspberry pi!


How to use? Install
===================

PC
–--
```Bash
make
make load
```

raspberry pi
–-------------
```Bash 
make
make load
```

On PC for raspberry pi:
–---------------------------
```Bash
make arm
scp * pi@pi:
ssh pi@pi
make load
```

But how?
–----------
If you compile the code on the device you want to use it on, you need the kernel source installed and some basic setup. Just search for a guide to compile kernel modules on your OS.

If you want to compile it for a different architecture (PC to pi) you also need a cross compiler. 
Look for a guide to crosscompile a kernel and modules for a pi.

You want an example for the pi? Here you go!
–------------------------------------------
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

Usage
=====

After loading the two modules you should have a new "/dev/videoX" which you can use like a web cam.

For example:
```Bash
camorama -d /dev/video0
vlc v4l:///dev/video0
```

This looks crappy and is to complicated!
=====================================
You are right!
When I have some time and maybe find help I try to get it in the kernel. Than all you have to do is plug your USB cable in.
But to do so the gspca framework has to be changed and I have to merge my work with the original kinect.c

I want it now!
-------------
No problem!
Please send my about 5 Instagramm or 0.25 Whatsapp.

Do you have pre compiled modules?
=============================
Yes, I am going to upload them after I have added some "special" features :P

Just kidding, but they might not work for you.

If you are using Raspbian and have a 3.10.y Kernel you may be lucky.
Do not forget to load videodev with ```modprobe videodev```.
