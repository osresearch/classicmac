classicmac
==========

Mac 128/Plus/SE hardware interface using the BeagleBone Black PRU.

Xvfb :0 -ac -screen 0 512x384x8 -fbdir /tmp/frames/ -classic
./x11mac /tmp/frames/Xvfb_screen0
