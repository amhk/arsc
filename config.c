#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "arsc.h"
#include "common.h"
#include "config.h"

/* Constants come from frameworks/base/include/android/configuration.h */
enum {
	CONFIG_ORIENTATION_ANY  = 0x0000,
	CONFIG_ORIENTATION_PORT = 0x0001,
	CONFIG_ORIENTATION_LAND = 0x0002,
	CONFIG_ORIENTATION_SQUARE = 0x0003,

	CONFIG_TOUCHSCREEN_ANY  = 0x0000,
	CONFIG_TOUCHSCREEN_NOTOUCH  = 0x0001,
	CONFIG_TOUCHSCREEN_STYLUS  = 0x0002,
	CONFIG_TOUCHSCREEN_FINGER  = 0x0003,

	CONFIG_DENSITY_DEFAULT = 0,
	CONFIG_DENSITY_LOW = 120,
	CONFIG_DENSITY_MEDIUM = 160,
	CONFIG_DENSITY_TV = 213,
	CONFIG_DENSITY_HIGH = 240,
	CONFIG_DENSITY_XHIGH = 320,
	CONFIG_DENSITY_XXHIGH = 480,
	CONFIG_DENSITY_XXXHIGH = 640,
	CONFIG_DENSITY_ANY = 0xfffe,
	CONFIG_DENSITY_NONE = 0xffff,

	CONFIG_KEYBOARD_ANY  = 0x0000,
	CONFIG_KEYBOARD_NOKEYS  = 0x0001,
	CONFIG_KEYBOARD_QWERTY  = 0x0002,
	CONFIG_KEYBOARD_12KEY  = 0x0003,

	CONFIG_NAVIGATION_ANY  = 0x0000,
	CONFIG_NAVIGATION_NONAV  = 0x0001,
	CONFIG_NAVIGATION_DPAD  = 0x0002,
	CONFIG_NAVIGATION_TRACKBALL  = 0x0003,
	CONFIG_NAVIGATION_WHEEL  = 0x0004,

	CONFIG_KEYSHIDDEN_ANY = 0x0000,
	CONFIG_KEYSHIDDEN_NO = 0x0001,
	CONFIG_KEYSHIDDEN_YES = 0x0002,
	CONFIG_KEYSHIDDEN_SOFT = 0x0003,

	CONFIG_NAVHIDDEN_ANY = 0x0000,
	CONFIG_NAVHIDDEN_NO = 0x0001,
	CONFIG_NAVHIDDEN_YES = 0x0002,

	CONFIG_SCREENSIZE_ANY  = 0x00,
	CONFIG_SCREENSIZE_SMALL = 0x01,
	CONFIG_SCREENSIZE_NORMAL = 0x02,
	CONFIG_SCREENSIZE_LARGE = 0x03,
	CONFIG_SCREENSIZE_XLARGE = 0x04,

	CONFIG_SCREENLONG_ANY = 0x00,
	CONFIG_SCREENLONG_NO = 0x1,
	CONFIG_SCREENLONG_YES = 0x2,

	CONFIG_UI_MODE_TYPE_ANY = 0x00,
	CONFIG_UI_MODE_TYPE_NORMAL = 0x01,
	CONFIG_UI_MODE_TYPE_DESK = 0x02,
	CONFIG_UI_MODE_TYPE_CAR = 0x03,
	CONFIG_UI_MODE_TYPE_TELEVISION = 0x04,
	CONFIG_UI_MODE_TYPE_APPLIANCE = 0x05,
	CONFIG_UI_MODE_TYPE_WATCH = 0x06,

	CONFIG_UI_MODE_NIGHT_ANY = 0x00,
	CONFIG_UI_MODE_NIGHT_NO = 0x1,
	CONFIG_UI_MODE_NIGHT_YES = 0x2,

	CONFIG_SCREEN_WIDTH_DP_ANY = 0x0000,

	CONFIG_SCREEN_HEIGHT_DP_ANY = 0x0000,

	CONFIG_SMALLEST_SCREEN_WIDTH_DP_ANY = 0x0000,

	CONFIG_LAYOUTDIR_ANY  = 0x00,
	CONFIG_LAYOUTDIR_LTR  = 0x01,
	CONFIG_LAYOUTDIR_RTL  = 0x02,

	CONFIG_MCC = 0x0001,
	CONFIG_MNC = 0x0002,
	CONFIG_LOCALE = 0x0004,
	CONFIG_TOUCHSCREEN = 0x0008,
	CONFIG_KEYBOARD = 0x0010,
	CONFIG_KEYBOARD_HIDDEN = 0x0020,
	CONFIG_NAVIGATION = 0x0040,
	CONFIG_ORIENTATION = 0x0080,
	CONFIG_DENSITY = 0x0100,
	CONFIG_SCREEN_SIZE = 0x0200,
	CONFIG_VERSION = 0x0400,
	CONFIG_SCREEN_LAYOUT = 0x0800,
	CONFIG_UI_MODE = 0x1000,
	CONFIG_SMALLEST_SCREEN_SIZE = 0x2000,
	CONFIG_LAYOUTDIR = 0x4000,
};

/* Constants come from frameworks/base/include/androidfw/ResourceTypes.h */
enum {
	MASK_KEYSHIDDEN = 0x0003,
	MASK_NAVHIDDEN = 0x000c,
	MASK_SCREENSIZE = 0x0f,
	MASK_SCREENLONG = 0x30,
	MASK_LAYOUTDIR = 0xC0,
	MASK_UI_MODE_TYPE = 0x0f,
	MASK_UI_MODE_NIGHT = 0x30,
};

static void append(char *buf, const char *fmt, ...)
{
	va_list ap;
	size_t len = strlen(buf);

	if (len > 0) {
		strncat(buf, "-", CONFIG_LEN - 1);
		len = strlen(buf);
	}
	va_start(ap, fmt);
	vsnprintf(buf + len, CONFIG_LEN - len - 1, fmt, ap);
	va_end(ap);
}

void config_to_string(const struct arsc_config *config, char buf[CONFIG_LEN])
{
	uint16_t x;

	memset(buf, 0, CONFIG_LEN);

	/* imsi */
	if (config->mcc)
		append(buf, "mcc%d", dtohs(config->mcc));
	if (config->mnc)
		append(buf, "mnc%d", dtohs(config->mnc));

	/* locale */
	if (config->language) {
		uint16_t l = dtohs(config->language);
		append(buf, "%c%c", l & 0xff, (l >> 8) & 0xff);
	}
	if (config->country) {
		uint16_t c = dtohs(config->country);
		append(buf, "%c%c", c & 0xff, (c >> 8) & 0xff);
	}

	/* screen type */
	x = dtohs(config->screen_layout) & MASK_LAYOUTDIR;
	if (x != CONFIG_LAYOUTDIR_ANY) {
		switch (x) {
		case CONFIG_LAYOUTDIR_LTR << 6:
			append(buf, "ldltr");
			break;
		case CONFIG_LAYOUTDIR_RTL << 6:
			append(buf, "ldrtl");
			break;
		}
	}

	/* screen size */
	if (config->smallest_screen_width_dp)
		append(buf, "sw%ddp", dtohs(config->smallest_screen_width_dp));

	/* screen width */
	if (config->screen_width_dp)
		append(buf, "w%ddp", dtohs(config->screen_width_dp));

	/* screen height */
	if (config->screen_height_dp)
		append(buf, "h%ddp", dtohs(config->screen_height_dp));

	/* screen layout: screen size */
	x = dtohs(config->screen_layout) & MASK_SCREENSIZE;
	if (x != CONFIG_SCREENSIZE_ANY) {
		switch (x) {
		case CONFIG_SCREENSIZE_SMALL:
			append(buf, "small");
			break;
		case CONFIG_SCREENSIZE_NORMAL:
			append(buf, "normal");
			break;
		case CONFIG_SCREENSIZE_LARGE:
			append(buf, "large");
			break;
		case CONFIG_SCREENSIZE_XLARGE:
			append(buf, "xlarge");
			break;
		}
	}

	/* screen layout: screen long */
	x = dtohs(config->screen_layout) & MASK_SCREENLONG;
	if (x != CONFIG_SCREENLONG_ANY) {
		switch (x) {
		case CONFIG_SCREENLONG_NO:
			append(buf, "notlong");
			break;
		case CONFIG_SCREENLONG_YES:
			append(buf, "long");
			break;
		}
	}

	/* orientation */
	if (dtohs(config->orientation) != CONFIG_ORIENTATION_ANY) {
		switch (dtohs(config->orientation)) {
		case CONFIG_ORIENTATION_PORT:
			append(buf, "port");
			break;
		case CONFIG_ORIENTATION_LAND:
			append(buf, "land");
			break;
		case CONFIG_ORIENTATION_SQUARE:
			append(buf, "square");
			break;
		}
	}

	/* ui mode: type */
	x = dtohs(config->ui_mode) & MASK_UI_MODE_TYPE;
	if (x != CONFIG_UI_MODE_TYPE_ANY) {
		switch (x & MASK_UI_MODE_TYPE) {
		case CONFIG_UI_MODE_TYPE_DESK:
			append(buf, "desk");
			break;
		case CONFIG_UI_MODE_TYPE_CAR:
			append(buf, "car");
			break;
		case CONFIG_UI_MODE_TYPE_TELEVISION:
			append(buf, "television");
			break;
		case CONFIG_UI_MODE_TYPE_APPLIANCE:
			append(buf, "appliance");
			break;
		}
	}

	/* ui mode: night mode */
	x = dtohs(config->ui_mode) & MASK_UI_MODE_NIGHT;
	if (x != CONFIG_UI_MODE_NIGHT_ANY) {
		switch (x) {
		case CONFIG_UI_MODE_NIGHT_NO:
			append(buf, "notnight");
			break;
		case CONFIG_UI_MODE_NIGHT_YES:
			append(buf, "night");
			break;
		}
	}

	/* density */
	if (dtohs(config->density) != CONFIG_DENSITY_DEFAULT) {
		switch (dtohs(config->density)) {
		case CONFIG_DENSITY_LOW:
			append(buf, "ldpi");
			break;
		case CONFIG_DENSITY_MEDIUM:
			append(buf, "mdpi");
			break;
		case CONFIG_DENSITY_TV:
			append(buf, "tvdpi");
			break;
		case CONFIG_DENSITY_HIGH:
			append(buf, "hdpi");
			break;
		case CONFIG_DENSITY_XHIGH:
			append(buf, "xhdpi");
			break;
		case CONFIG_DENSITY_XXHIGH:
			append(buf, "xxhdpi");
			break;
		case CONFIG_DENSITY_XXXHIGH:
			append(buf, "xxxhdpi");
			break;
		case CONFIG_DENSITY_NONE:
			append(buf, "nodpi");
			break;
		case CONFIG_DENSITY_ANY:
			append(buf, "anydpi");
			break;
		}
	}

	/* touchscreen */
	if (dtohs(config->touchscreen) != CONFIG_TOUCHSCREEN_ANY) {
		switch (dtohs(config->touchscreen)) {
		case CONFIG_TOUCHSCREEN_NOTOUCH:
			append(buf, "notouch");
			break;
		case CONFIG_TOUCHSCREEN_FINGER:
			append(buf, "finger");
			break;
		case CONFIG_TOUCHSCREEN_STYLUS:
			append(buf, "stylus");
			break;
		}
	}

	/* input flags: keys hidden */
	x = dtohs(config->input_flags) & MASK_KEYSHIDDEN;
	if (x != CONFIG_KEYSHIDDEN_ANY) {
		switch (x) {
		case CONFIG_KEYSHIDDEN_NO:
			append(buf, "keysexposed");
			break;
		case CONFIG_KEYSHIDDEN_YES:
			append(buf, "keyshidden");
			break;
		case CONFIG_KEYSHIDDEN_SOFT:
			append(buf, "keyssoft");
			break;
		}
	}

	/* keyboard */
	if (dtohs(config->keyboard) != CONFIG_KEYBOARD_ANY) {
		switch (dtohs(config->keyboard)) {
		case CONFIG_KEYBOARD_NOKEYS:
			append(buf, "nokeys");
			break;
		case CONFIG_KEYBOARD_QWERTY:
			append(buf, "qwerty");
			break;
		case CONFIG_KEYBOARD_12KEY:
			append(buf, "12key");
			break;
		}
	}

	/* input flags: navigation hidden */
	x = dtohs(config->input_flags) & MASK_NAVHIDDEN;
	if (x != CONFIG_NAVHIDDEN_ANY) {
		switch (x) {
		case CONFIG_NAVHIDDEN_NO:
			append(buf, "navexposed");
			break;
		case CONFIG_NAVHIDDEN_YES:
			append(buf, "navhidden");
			break;
		}
	}

	/* navigation */
	if (dtohs(config->navigation) != CONFIG_NAVIGATION_ANY) {
		switch (dtohs(config->navigation)) {
		case CONFIG_NAVIGATION_NONAV:
			append(buf, "nonav");
			break;
		case CONFIG_NAVIGATION_DPAD:
			append(buf, "dpad");
			break;
		case CONFIG_NAVIGATION_TRACKBALL:
			append(buf, "trackball");
			break;
		case CONFIG_NAVIGATION_WHEEL:
			append(buf, "wheel");
			break;
		}
	}

	/* default config (all fields 0) */
	if (strlen(buf) == 0)
		strcpy(buf, "-");
}
