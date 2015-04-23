#!/bin/sh
sudo rmmod kinect > /dev/null 2>&1
sudo rmmod gspca > /dev/null 2>&1
sudo rmmod gspca_main  > /dev/null 2>&1
sudo rmmod gspca_kinect > /dev/null 2>&1
sudo rmmod *.ko > /dev/null 2>&1
echo "tried to unload all related drivers (and possibly more ;)"
return 0;
