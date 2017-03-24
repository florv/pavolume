/* Copyright 2016 Florian Vessaz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pulse/pulseaudio.h>

#define NAME "pavolume"

pa_mainloop *mainloop = NULL;

enum volume_action {VOLUME_NO_ACTION, SET_VOLUME, DECREASE_VOLUME, INCREASE_VOLUME};
enum volume_action volume_action = VOLUME_NO_ACTION;

union volume_value {
	float f;
	pa_volume_t volume;
};
union volume_value value;
int value_is_volume = 0;

enum mute_action {MUTE_NO_ACTION, MUTE, UNMUTE, TOGGLE_MUTE};
enum mute_action mute_action = MUTE_NO_ACTION;

void die(const char *msg) {
	fprintf(stderr, "%s: %s.\n", NAME, msg);
	exit(1);
}

void die2(const char *msg, const char *msg2) {
	fprintf(stderr, "%s: %s (%s).\n", NAME, msg, msg2);
	exit(1);
}

void on_success_quit(pa_context *context, int success, void *userdata) {
	pa_mainloop_quit(mainloop, 0);
}

void on_sink_info(pa_context *context, const pa_sink_info *info, int eol, void *userdata) {
	if (info != NULL) {
		char buf[1024];
		pa_cvolume_snprint(buf, 1024, &info->volume);
		printf("%s\n", buf);

		if (mute_action == TOGGLE_MUTE) {
			mute_action = info->mute ? UNMUTE : MUTE;
		}

		if (mute_action == MUTE) {
			pa_context_success_cb_t callback =
				(volume_action == VOLUME_NO_ACTION) ? on_success_quit : NULL;
			pa_context_set_sink_mute_by_index(context, info->index, 1,
					callback, NULL);
		}

		if (volume_action != VOLUME_NO_ACTION) {
			pa_volume_t volume_step = value_is_volume ?  value.volume : (PA_VOLUME_NORM - PA_VOLUME_MUTED) * value.f;
			pa_cvolume volume = info->volume;
			switch (volume_action) {
				case DECREASE_VOLUME:
					pa_cvolume_dec(&volume, volume_step);
					break;
				case INCREASE_VOLUME:
					pa_cvolume_inc(&volume, volume_step);
					break;
				case SET_VOLUME:
					pa_cvolume_set(&volume, volume.channels, volume_step);
				default:
					break;
			}
			pa_cvolume_snprint(buf, 1024, &volume);
			printf("%s\n", buf);
			pa_context_success_cb_t callback =
				(mute_action == UNMUTE) ? NULL : on_success_quit;
			pa_context_set_sink_volume_by_index(context, info->index, &volume,
					callback, NULL);
		} else {
			if (mute_action == MUTE_NO_ACTION) {
				pa_mainloop_quit(mainloop, 0);
			}
		}

		if (mute_action == UNMUTE) {
			pa_context_set_sink_mute_by_index(context, info->index, 0,
					on_success_quit, NULL);
		}

	}
}

void on_server_info(pa_context *context, const pa_server_info *info, void *userdata) {
	pa_context_get_sink_info_by_name(context, info->default_sink_name, on_sink_info, NULL);
}

void on_context_state_change(pa_context *context, void *userdata) {
	pa_context_state_t state = pa_context_get_state(context);
	if (state == PA_CONTEXT_READY) {
		pa_context_get_server_info(context, on_server_info, NULL);
	} else if (state == PA_CONTEXT_FAILED) {
		die2("Failed to connect to pulse", pa_strerror(pa_context_errno(context)));
	}
}

void parse_volume(const char * s) {
	char * endptr;
	value.f = strtof(s, &endptr);
	if (endptr == s) {
		die("Couldn't parse numeric value in argument.");
	}
	if (*endptr == '%') {
		value.f /= 100.0;
	} else if (!strcmp(endptr, "dB")) {
		value.volume = pa_sw_volume_from_dB(value.f);
		value_is_volume = 1;
	}
}

int main (int argc, char **argv) {
	for (int i = 1; i < argc; ++i) {
		if (!strcasecmp(argv[i], "mute")) {
			mute_action = MUTE;
		} else if (!strcasecmp(argv[i], "unmute")) {
			mute_action = UNMUTE;
		} else if (!strcasecmp(argv[i], "toggle")) {
			mute_action = TOGGLE_MUTE;
		} else if (argv[i][0] == '+') {
			volume_action = INCREASE_VOLUME;
			parse_volume(argv[i] + 1);
		} else if (argv[i][0] == '-') {
			volume_action = DECREASE_VOLUME;
			parse_volume(argv[i] + 1);
		} else if (argv[i][0] == '=') {
			volume_action = SET_VOLUME;
			parse_volume(argv[i] + 1);
		} else {
			volume_action = SET_VOLUME;
			parse_volume(argv[i]);
		}
	}

	mainloop = pa_mainloop_new();
	if (mainloop == NULL) {
		die("pa_mainloop_new() failed");
	}
	pa_mainloop_api *pa_mainloop_api = pa_mainloop_get_api(mainloop);
	pa_context *pa_context = pa_context_new(pa_mainloop_api, NAME);
	if (pa_context == NULL) {
		die("pa_context_new() failed");
	}
	pa_context_set_state_callback(pa_context, on_context_state_change, NULL);
	pa_context_connect(pa_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

	int ret = 1;
	pa_mainloop_run(mainloop, &ret);
	return ret;
}
