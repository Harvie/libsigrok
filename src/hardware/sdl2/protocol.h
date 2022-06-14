/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2011 Daniel Ribeiro <drwyrm@gmail.com>
 * Copyright (C) 2012 Renato Caldas <rmsc@fe.up.pt>
 * Copyright (C) 2013 Lior Elazary <lelazary@yahoo.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSIGROK_HARDWARE_SDL2_PROTOCOL_H
#define LIBSIGROK_HARDWARE_SDL2_PROTOCOL_H

#include <stdint.h>
#include <string.h>
#include <glib.h>
//#include <libudev.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"
#include <SDL2/SDL.h>

#define LOG_PREFIX "sdl2-audio-interface"

struct dev_context {
	SDL_AudioDeviceID sdl_device_index;
	SDL_AudioSpec	  sdl_device_spec;

	uint64_t cur_samplerate;
	uint64_t limit_samples;
	uint64_t limit_samples_remaining;
	uint64_t capture_ratio;
};

//SR_PRIV void stop_acquisition(const struct sr_dev_inst *sdi);

#endif
