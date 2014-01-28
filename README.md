Classic Mac CRT Interface
=========================

Mac 128/Plus/SE hardware interface using the BeagleBone Black PRU.

Run the virtual frame buffer:

	Xvfb :0 -ac -screen 0 512x384x8 -fbdir /tmp/ -retro

Start the X11 to Mac CRT translation:

	./x11mac /tmp/Xvfb_screen0

And start the mouse/keyboard event to X11 event translation:

	./eventmap /dev/input/event*

Problems?  hudson@trmm.net

