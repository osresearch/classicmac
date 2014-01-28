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

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include "util.h"


static Display * dpy;

// table from http://lists.freedesktop.org/pipermail/xorg/2006-May/015587.html
static const int keymap[] = {
	  0,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
22,  23,
	 24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
54,  55,
	 56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
70,  71,
	 72,  73,  74,  75,  76,  77,  76,  79,  80,  81,  82,  83,  84,  85,
86,  87,
	 88,  89,  90,  91, 111,  221, 94,  95,  96, 211, 128, 127, 129, 208,
131, 126,
	108, 109, 112, 111, 113, 181,  97,  98,  99, 100, 102, 103, 104, 105,
106, 107,
	239, 160, 174, 176, 222, 157, 123, 110, 139, 134, 209, 210, 133, 115,
116, 117,
	232, 133, 134, 135, 140, 248, 191, 192, 122, 188, 245, 158, 161, 193,
223, 227,
	198, 199, 200, 147, 159, 151, 178, 201, 146, 203, 166, 236, 230, 235,
234, 233,
	163, 204, 253, 153, 162, 144, 164, 177, 152, 190, 208, 129, 130, 231,
209, 210,
	136, 220, 143, 246, 251, 137, 138, 182, 183, 184,  93, 184, 247, 132,
170, 219,
	249, 205, 207, 149, 150, 154, 155, 167, 168, 169, 171, 172, 173, 165,
175, 179,
	180,   0, 185, 186, 187, 118, 119, 120, 121, 229, 194, 195, 196, 197,
148, 202,
	101, 212, 237, 214, 215, 216, 217, 218, 228, 142, 213, 240, 241, 242,
243, 244,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
0,   0
#if 0
	// this doesn't work.  bummer;
	[KEY_RESERVED] = 0,
	[KEY_ESC] = XK_Escape,
	[KEY_1] = XK_1,
	[KEY_2] = XK_2,
	[KEY_3] = XK_3,
	[KEY_4] = XK_4,
	[KEY_5] = XK_5,
	[KEY_6] = XK_6,
	[KEY_7] = XK_7,
	[KEY_8] = XK_8,
	[KEY_9] = XK_9,
	[KEY_0] = XK_0,
	[KEY_MINUS] = XK_minus,
	[KEY_EQUAL] = XK_equal,
	[KEY_BACKSPACE] = XK_BackSpace,
	[KEY_TAB] = XK_Tab,
	[KEY_Q] = XK_Q,
	[KEY_W] = XK_W,
	[KEY_E] = XK_E,
	[KEY_R] = XK_R,
	[KEY_T] = XK_T,
	[KEY_Y] = XK_Y,
	[KEY_U] = XK_U,
	[KEY_I] = XK_I,
	[KEY_O] = XK_O,
	[KEY_P] = XK_P,
	[KEY_LEFTBRACE] = XK_braceleft,
	[KEY_RIGHTBRACE] = XK_braceright,
	[KEY_ENTER] = XK_Return,
	[KEY_LEFTCTRL] = XK_Control_L,
	[KEY_A] = XK_A,
	[KEY_S] = XK_S,
	[KEY_D] = XK_D,
	[KEY_F] = XK_F,
	[KEY_G] = XK_G,
	[KEY_H] = XK_H,
	[KEY_J] = XK_J,
	[KEY_K] = XK_K,
	[KEY_L] = XK_L,
	[KEY_SEMICOLON] = XK_semicolon,
	[KEY_APOSTROPHE] = XK_apostrophe,
	[KEY_GRAVE] = XK_grave,
	[KEY_LEFTSHIFT] = XK_Shift_L,
	[KEY_BACKSLASH] = XK_backslash,
	[KEY_Z] = XK_Z,
	[KEY_X] = XK_X,
	[KEY_C] = XK_C,
	[KEY_V] = XK_V,
	[KEY_B] = XK_B,
	[KEY_N] = XK_N,
	[KEY_M] = XK_M,
	[KEY_COMMA] = XK_comma,
	[KEY_DOT] = XK_period,
	[KEY_SLASH] = XK_slash,
	[KEY_RIGHTSHIFT] = XK_Shift_R,
	[KEY_KPASTERISK] = XK_KP_Multiply,
	[KEY_LEFTALT] = XK_Alt_L,
	[KEY_SPACE] = XK_space,
	[KEY_CAPSLOCK] = XK_Caps_Lock,
	[KEY_F1] = XK_F1,
	[KEY_F2] = XK_F2,
	[KEY_F3] = XK_F3,
	[KEY_F4] = XK_F4,
	[KEY_F5] = XK_F5,
	[KEY_F6] = XK_F6,
	[KEY_F7] = XK_F7,
	[KEY_F8] = XK_F8,
	[KEY_F9] = XK_F9,
	[KEY_F10] = XK_F10,
	[KEY_NUMLOCK] = XK_Num_Lock,
	[KEY_SCROLLLOCK] = XK_Scroll_Lock,
	[KEY_KP7] = XK_KP_7,
	[KEY_KP8] = XK_KP_8,
	[KEY_KP9] = XK_KP_9,
	[KEY_KPMINUS] = XK_KP_Subtract,
	[KEY_KP4] = XK_KP_4,
	[KEY_KP5] = XK_KP_5,
	[KEY_KP6] = XK_KP_6,
	[KEY_KPPLUS] = XK_KP_Add,
	[KEY_KP1] = XK_KP_1,
	[KEY_KP2] = XK_KP_2,
	[KEY_KP3] = XK_KP_3,
	[KEY_KP0] = XK_KP_0,
	[KEY_KPDOT] = XK_KP_Decimal,
	[KEY_ZENKAKUHANKAKU] = XK_Zenkaku_Hankaku,
	//[KEY_102ND] = XK_102ND,
	[KEY_F11] = XK_F11,
	[KEY_F12] = XK_F12,
#if 0
	[KEY_RO] = XK_RO,
	[KEY_KATAKANA] = XK_KATAKANA,
	[KEY_HIRAGANA] = XK_HIRAGANA,
	[KEY_HENKAN] = XK_HENKAN,
	[KEY_KATAKANAHIRAGANA] = XK_KATAKANAHIRAGANA,
	[KEY_MUHENKAN] = XK_MUHENKAN,
	[KEY_KPJPCOMMA] = XK_KPJPCOMMA,
#endif
	[KEY_KPENTER] = XK_KP_Enter,
	[KEY_RIGHTCTRL] = XK_Control_R,
	[KEY_KPSLASH] = XK_KP_Divide,
	[KEY_SYSRQ] = XK_Sys_Req,
	[KEY_RIGHTALT] = XK_Alt_R,
	[KEY_LINEFEED] = XK_Linefeed,
	[KEY_HOME] = XK_Home,
	[KEY_UP] = XK_Up,
	[KEY_PAGEUP] = XK_Page_Up,
	[KEY_LEFT] = XK_Left,
	[KEY_RIGHT] = XK_Right,
	[KEY_END] = XK_End,
	[KEY_DOWN] = XK_Down,
	[KEY_PAGEDOWN] = XK_Page_Down,
	[KEY_INSERT] = XK_Insert,
	[KEY_DELETE] = XK_Delete,
	//[KEY_MACRO] = XK_MACRO,
	//[KEY_MUTE] = XK_MUTE,
	//[KEY_VOLUMEDOWN] = XK_VOLUMEDOWN,
	//[KEY_VOLUMEUP] = XK_VOLUMEUP,
	//[KEY_POWER] = XK_POWER,
	[KEY_KPEQUAL] = XK_KP_Equal,
	//[KEY_KPPLUSMINUS] = XK_KPPLUSMINUS,
	//[KEY_PAUSE] = XK_PAUSE,
	//[KEY_SCALE] = XK_SCALE,
	//[KEY_KPCOMMA] = XK_KPCOMMA,
	//[KEY_HANGEUL] = XK_HANGEUL,
	//[KEY_HANGUEL] = XK_HANGUEL,
	//[KEY_HANJA] = XK_HANJA,
	//[KEY_YEN] = XK_YEN,
	[KEY_LEFTMETA] = XK_Meta_L,
	[KEY_RIGHTMETA] = XK_Meta_R,
#if 0
	[KEY_COMPOSE] = XK_Compose,
	[KEY_STOP] = XK_Stop,
	[KEY_AGAIN] = XK_AGAIN,
	[KEY_PROPS] = XK_PROPS,
	[KEY_UNDO] = XK_UNDO,
	[KEY_FRONT] = XK_FRONT,
	[KEY_COPY] = XK_COPY,
	[KEY_OPEN] = XK_OPEN,
	[KEY_PASTE] = XK_PASTE,
	[KEY_FIND] = XK_FIND,
	[KEY_CUT] = XK_CUT,
	[KEY_HELP] = XK_HELP,
	[KEY_MENU] = XK_MENU,
	[KEY_CALC] = XK_CALC,
	[KEY_SETUP] = XK_SETUP,
	[KEY_SLEEP] = XK_SLEEP,
	[KEY_WAKEUP] = XK_WAKEUP,
	[KEY_FILE] = XK_FILE,
	[KEY_SENDFILE] = XK_SENDFILE,
	[KEY_DELETEFILE] = XK_DELETEFILE,
	[KEY_XFER] = XK_XFER,
	[KEY_PROG1] = XK_PROG1,
	[KEY_PROG2] = XK_PROG2,
	[KEY_WWW] = XK_WWW,
	[KEY_MSDOS] = XK_MSDOS,
	[KEY_COFFEE] = XK_COFFEE,
	[KEY_SCREENLOCK] = XK_SCREENLOCK,
	[KEY_DIRECTION] = XK_DIRECTION,
	[KEY_CYCLEWINDOWS] = XK_CYCLEWINDOWS,
	[KEY_MAIL] = XK_MAIL,
	[KEY_BOOKMARKS] = XK_BOOKMARKS,
	[KEY_COMPUTER] = XK_COMPUTER,
	[KEY_BACK] = XK_BACK,
	[KEY_FORWARD] = XK_FORWARD,
	[KEY_CLOSECD] = XK_CLOSECD,
	[KEY_EJECTCD] = XK_EJECTCD,
	[KEY_EJECTCLOSECD] = XK_EJECTCLOSECD,
	[KEY_NEXTSONG] = XK_NEXTSONG,
	[KEY_PLAYPAUSE] = XK_PLAYPAUSE,
	[KEY_PREVIOUSSONG] = XK_PREVIOUSSONG,
	[KEY_STOPCD] = XK_STOPCD,
	[KEY_RECORD] = XK_RECORD,
	[KEY_REWIND] = XK_REWIND,
	[KEY_PHONE] = XK_PHONE,
	[KEY_ISO] = XK_ISO,
	[KEY_CONFIG] = XK_CONFIG,
	[KEY_HOMEPAGE] = XK_HOMEPAGE,
	[KEY_REFRESH] = XK_REFRESH,
	[KEY_EXIT] = XK_EXIT,
	[KEY_MOVE] = XK_MOVE,
	[KEY_EDIT] = XK_EDIT,
	[KEY_SCROLLUP] = XK_SCROLLUP,
	[KEY_SCROLLDOWN] = XK_SCROLLDOWN,
	[KEY_KPLEFTPAREN] = XK_KPLEFTPAREN,
	[KEY_KPRIGHTPAREN] = XK_KPRIGHTPAREN,
	[KEY_NEW] = XK_NEW,
	[KEY_REDO] = XK_REDO,
	[KEY_F13] = XK_F13,
	[KEY_F14] = XK_F14,
	[KEY_F15] = XK_F15,
	[KEY_F16] = XK_F16,
	[KEY_F17] = XK_F17,
	[KEY_F18] = XK_F18,
	[KEY_F19] = XK_F19,
	[KEY_F20] = XK_F20,
	[KEY_F21] = XK_F21,
	[KEY_F22] = XK_F22,
	[KEY_F23] = XK_F23,
	[KEY_F24] = XK_F24,
	[KEY_PLAYCD] = XK_PLAYCD,
	[KEY_PAUSECD] = XK_PAUSECD,
	[KEY_PROG3] = XK_PROG3,
	[KEY_PROG4] = XK_PROG4,
	[KEY_DASHBOARD] = XK_DASHBOARD,
	[KEY_SUSPEND] = XK_SUSPEND,
	[KEY_CLOSE] = XK_CLOSE,
	[KEY_PLAY] = XK_PLAY,
	[KEY_FASTFORWARD] = XK_FASTFORWARD,
	[KEY_BASSBOOST] = XK_BASSBOOST,
	[KEY_PRINT] = XK_PRINT,
	[KEY_HP] = XK_HP,
	[KEY_CAMERA] = XK_CAMERA,
	[KEY_SOUND] = XK_SOUND,
	[KEY_QUESTION] = XK_QUESTION,
	[KEY_EMAIL] = XK_EMAIL,
	[KEY_CHAT] = XK_CHAT,
	[KEY_SEARCH] = XK_SEARCH,
	[KEY_CONNECT] = XK_CONNECT,
	[KEY_FINANCE] = XK_FINANCE,
	[KEY_SPORT] = XK_SPORT,
	[KEY_SHOP] = XK_SHOP,
	[KEY_ALTERASE] = XK_ALTERASE,
	[KEY_CANCEL] = XK_CANCEL,
	[KEY_BRIGHTNESSDOWN] = XK_BRIGHTNESSDOWN,
	[KEY_BRIGHTNESSUP] = XK_BRIGHTNESSUP,
	[KEY_MEDIA] = XK_MEDIA,
	[KEY_SWITCHVIDEOMODE] = XK_SWITCHVIDEOMODE,
	[KEY_KBDILLUMTOGGLE] = XK_KBDILLUMTOGGLE,
	[KEY_KBDILLUMDOWN] = XK_KBDILLUMDOWN,
	[KEY_KBDILLUMUP] = XK_KBDILLUMUP,
	[KEY_SEND] = XK_SEND,
	[KEY_REPLY] = XK_REPLY,
	[KEY_FORWARDMAIL] = XK_FORWARDMAIL,
	[KEY_SAVE] = XK_SAVE,
	[KEY_DOCUMENTS] = XK_DOCUMENTS,
	[KEY_BATTERY] = XK_BATTERY,
	[KEY_BLUETOOTH] = XK_BLUETOOTH,
	[KEY_WLAN] = XK_WLAN,
	[KEY_UWB] = XK_UWB,
	[KEY_UNKNOWN] = XK_UNKNOWN,
	[KEY_VIDEO_NEXT] = XK_VIDEO_NEXT,
	[KEY_VIDEO_PREV] = XK_VIDEO_PREV,
	[KEY_BRIGHTNESS_CYCLE] = XK_BRIGHTNESS_CYCLE,
	[KEY_BRIGHTNESS_ZERO] = XK_BRIGHTNESS_ZERO,
	[KEY_DISPLAY_OFF] = XK_DISPLAY_OFF,
	[KEY_WIMAX] = XK_WIMAX,
	[KEY_RFKILL] = XK_RFKILL,
	[KEY_MICMUTE] = XK_MICMUTE,
#endif
#endif
};


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
		if (0)
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
				int key = keymap[button];
				if (key != 0)
					XTestFakeKeyEvent(dpy, key, is_press, 0);
				else
					warn("EV_KEY code=%d->%d unhandled\n", ev->code, key);
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
	
	const int num_fds = argc - 1;
	int * fds = calloc(sizeof(*fds), num_fds);
	int max_fd = 0;

	for (int i = 0 ; i < num_fds ; i++)
	{
		const char * const devname = argv[i+1];
		const int fd = open(devname, O_RDONLY, 0666);
		if (fd < 0)
			die("%s: failed to open\n", devname);
		fds[i] = fd;

		printf("%s (fd %d)\n", devname, fd);
		if (fd > max_fd)
			max_fd = fd;
	}

	while (1)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		for (int i = 0 ; i < num_fds ; i++)
		{
			const int fd = fds[i];
			FD_SET(fd, &read_fds);
		}

		int rc = select(max_fd+1, &read_fds, NULL, NULL, NULL);
		if (rc < 0)
			die("select\n");

		for (int i = 0 ; i < num_fds ; i++)
		{
			const int fd = fds[i];
			if (FD_ISSET(fd, &read_fds))
				read_one(fd);
		}
	}
}

