#!/bin/sh
sudo rmmod kinect
sudo rmmod gspca
sudo rmmod gspca_main
sudo rmmod gspca_kinect
sudo rmmod *.ko
return 0;
