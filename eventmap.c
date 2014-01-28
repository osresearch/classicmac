/** \file
 * Map /dev/input/event* to XTestEvents.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <X11/extensions/XTest.h>
#include <X11/Xlib.h>
#include "util.h"


static Display * dpy;


static void
read_one(
	int fd
)
{
	const int max_evs = 8;
	struct input_event evs[max_evs];
	const ssize_t rlen = read(fd, evs, sizeof(evs));
	if (rlen < 0)
		die("read failed");

	const int num_ev = rlen / sizeof(*evs);

	int mouse_valid = 0;
	int mx = 0;
	int my = 0;

	for (int i = 0 ; i < num_ev ; i++)
	{
		const struct input_event * const ev = &evs[i];
		printf("%d: type=%02x code=%x val=%d\n",
			i,
			ev->type,
			ev->code,
			ev->value
		);

		switch (ev->type)
		{
		case EV_SYN:
		case EV_MSC:
			break;
		case EV_REL:
			mouse_valid = 1;
			if (ev->code == REL_X)
				mx = ev->value;
			else
			if (ev->code == REL_Y)
				my = ev->value;
			else
				warn("EV_REL code=%d unhandled\n", ev->code);
			break;
		case EV_KEY:
		{
			int button = ev->code;
			int is_press = ev->value;
			if (button > 110)
			{
				button = (button & 0xF) + 1;
				XTestFakeButtonEvent(dpy, button, is_press, 0);
			} else {
				warn("EV_KEY code=%d unhandled\n", ev->code);
			}
			break;
		}
		default:
			warn("type %d code=%d unhandled\n", ev->type, ev->code);
			break;
		}
	}

	if (mouse_valid)
	{
		// no screen argument?
		int rc = XTestFakeRelativeMotionEvent(dpy, mx, my, 0);
	}

	XFlush(dpy);
}


int
main(
	int argc,
	char ** argv
)
{
	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("Unable to open display\n");
	
	int * fds = calloc(sizeof(*fds), argc-1);

	for (int i = 1 ; i < argc ; i++)
	{
		const char * const devname = argv[i];
		const int fd = open(devname, O_RDONLY, 0666);
		if (fd < 0)
			die("%s: failed to open\n", devname);
		fds[i-1] = fd;
	}

	// only handle one now
	while (1)
	{
		read_one(fds[0]);
	}
}

