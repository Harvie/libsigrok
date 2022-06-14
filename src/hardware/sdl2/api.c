/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2022 Tomas Mudrunka <harviecz@gmail.com>
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

#include <config.h>
#include "protocol.h"
#include <SDL2/SDL.h>

static const uint32_t drvopts[] = {
	SR_CONF_OSCILLOSCOPE,
	SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
	SR_CONF_LIMIT_SAMPLES | SR_CONF_SET,
	SR_CONF_SAMPLERATE | SR_CONF_GET, // | SR_CONF_LIST | SR_CONF_SET,
	/*
	SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
	SR_CONF_TRIGGER_SLOPE | SR_CONF_SET,
	SR_CONF_HORIZ_TRIGGERPOS | SR_CONF_SET,
	SR_CONF_CAPTURE_RATIO | SR_CONF_SET,
	SR_CONF_RLE | SR_CONF_SET,
*/
};

static const char *channel_names[] = {
	//Channel names for 7.1 DS Audio:
	//Front-Left, Front-Right, Center, LowFreq, Surround-Left, Surround-Right, Hearing-Impaired, Visualy-Impaired, etc...
	"FL", "FR", "CE", "LF", "SL", "SR", "HI", "VI", "CL", "CR", "RSL", "RSR", "CH13", "CH14", "CH15", "CH16", "PLSSTOP", "SRSLY"
};

static int init(struct sr_dev_driver *di, struct sr_context *sr_ctx)
{
	SDL_Init(SDL_INIT_AUDIO);
	return std_init(di, sr_ctx);
}

static int cleanup(const struct sr_dev_driver *di)
{
	SDL_Quit();
	return std_cleanup(di);
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	(void)options;

	GSList		       *devices = NULL;
	struct dev_context	   *devc;
	struct sr_dev_inst	   *sdi;
	struct sr_channel	  *ch;
	struct sr_channel_group *acg;

	SDL_AudioDeviceID dev_count = SDL_GetNumAudioDevices(0);
	SDL_AudioDeviceID dev_i;
	SDL_AudioSpec	  dev_spec;

	for (dev_i = 0; dev_i < dev_count; ++dev_i) {
		//printf("Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));

		if (SDL_GetAudioDeviceSpec(dev_i, 1, &dev_spec)) continue;

		//Create driver specific data (priv) structure for driver instance
		devc = g_malloc0(sizeof(struct dev_context));
		memcpy(&devc->sdl_device_spec, &dev_spec, sizeof(SDL_AudioSpec));
		devc->sdl_device_index = dev_i;
		devc->cur_samplerate   = SR_HZ(devc->sdl_device_spec.freq);

		//Create device instance
		sdi	    = g_malloc0(sizeof(struct sr_dev_inst));
		sdi->status = SR_ST_INACTIVE;
		sdi->model  = g_strdup_printf("[#%d, %dch, %dHz] %s", dev_i, dev_spec.channels, dev_spec.freq, SDL_GetAudioDeviceName(dev_i, 0));
		sdi->priv = devc;			  //Reference to driver specific data
		devices	  = g_slist_append(devices, sdi); //Add device to list

		//Create analog channel group
		acg		    = g_malloc0(sizeof(struct sr_channel_group));
		acg->name	    = g_strdup("Analog");
		sdi->channel_groups = g_slist_append(sdi->channel_groups, acg);

		int ch_i;
		for (ch_i = 0; ch_i < dev_spec.channels; ch_i++) {
			//Put new channel to group
			ch	      = sr_channel_new(sdi, ch_i, SR_CHANNEL_ANALOG, TRUE, channel_names[ch_i]);
			acg->channels = g_slist_append(acg->channels, ch);
		}
	}

	return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	struct dev_context *devc;

	devc = sdi->priv;

	//Check if SDL device is still available
	SDL_AudioSpec dev_spec;
	if (SDL_GetAudioDeviceSpec(devc->sdl_device_index, 1, &dev_spec)) return SR_ERR;

	//TODO: flush buffer?

	return SR_OK;
}

static int config_get(unsigned int key, GVariant **data, const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;

	(void)cg;

	if (!sdi) return SR_ERR_ARG;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_LIMIT_SAMPLES: *data = g_variant_new_uint64(devc->limit_samples); break;
	case SR_CONF_SAMPLERATE: *data = g_variant_new_uint64(devc->cur_samplerate); break;
	default: return SR_ERR_NA;
	}

	return SR_OK;
}

static int config_set(unsigned int key, GVariant *data, const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;
	uint64_t	    num_samples;
	//const char *slope;
	//int trigger_pos;
	double pos;

	(void)cg;

	return SR_OK;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_SAMPLERATE:
		// FIXME
		//return mso_configure_rate(sdi, g_variant_get_uint64(data));
		return SR_ERR_NA;
	case SR_CONF_LIMIT_SAMPLES:
		num_samples = g_variant_get_uint64(data);
		sr_err("Received config samples: %lu", num_samples);
		devc->limit_samples = num_samples;
		break;
	case SR_CONF_CAPTURE_RATIO: break;
	case SR_CONF_TRIGGER_SLOPE:
		//if ((idx = std_str_idx(data, ARRAY_AND_SIZE(trigger_slopes))) < 0)
		//	return SR_ERR_ARG;
		//devc->trigger_slope = idx;
		break;
	case SR_CONF_HORIZ_TRIGGERPOS:
		pos = g_variant_get_double(data);
		if (pos < 0 || pos > 255) {
			sr_err("Trigger position (%f) should be between 0 and 255.", pos);
			return SR_ERR_ARG;
		}
		//trigger_pos = (int)pos;
		//devc->trigger_holdoff[0] = trigger_pos & 0xff;
		break;
	case SR_CONF_RLE: break;
	default: return SR_ERR_NA;
	}

	return SR_OK;
}

static int config_list(unsigned int key, GVariant **data, const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	//struct dev_context *devc;
	//devc = sdi->priv;

	if (cg) return SR_ERR_NA; //Cannot handle this right now

	switch (key) {
	case SR_CONF_DEVICE_OPTIONS: return STD_CONFIG_LIST(key, data, sdi, cg, NO_OPTS, drvopts, devopts);
	case SR_CONF_SAMPLERATE:
		// *data = std_gvar_samplerates_steps(ARRAY_AND_SIZE(samplerates));
		// *data = std_gvar_samplerates_steps(&devc->cur_samplerate, 1); //not working
		return SR_ERR_NA;
		break;
	case SR_CONF_TRIGGER_MATCH:
		// *data = g_variant_new_string(TRIGGER_TYPE);
		break;
	default: return SR_ERR_NA;
	}

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi);
int sdl_data_callback(int fd, int revents, void *cb_data);
int sdl_data_callback(int fd, int revents, void *cb_data)
{
	(void)fd;
	(void)revents;

	struct sr_dev_inst	   *sdi;
	struct dev_context	   *devc;
	struct sr_datafeed_packet packet;
	struct sr_datafeed_analog packet_analog;

	struct sr_analog_encoding encoding;
	struct sr_analog_meaning  meaning;
	struct sr_analog_spec	  spec;

	struct sr_rational r_scale, r_offset;

	sdi  = cb_data;
	devc = sdi->priv;

	sr_err("Samples: %lu", devc->limit_samples_remaining);

	if (devc->limit_samples_remaining <= 0 || devc->limit_samples_remaining > 65535) { //Already sent everything
		dev_acquisition_stop(sdi); //TODO: is this really needed???
		return SR_OK;
	}

	sr_analog_init(&packet_analog, &encoding, &meaning, &spec, 0);

	struct sr_channel_group *lastcg = g_slist_nth_data(sdi->channel_groups, 0);
	//struct sr_channel *srch = g_slist_nth_data(lastcg->channels, 0);
	//sr_err("NASEL JSEM %s\n", srch->name);

	SDL_AudioFormat sf = AUDIO_S8;
	sf = devc->sdl_open_spec.format;

	//encoding
	encoding.unitsize	   = SDL_AUDIO_BITSIZE(sf) / 8; //???
	encoding.is_signed	   = SDL_AUDIO_ISSIGNED(sf);
	encoding.is_float	   = SDL_AUDIO_ISFLOAT(sf);
	encoding.is_bigendian	   = SDL_AUDIO_ISBIGENDIAN(sf);
	encoding.digits		   = 2;
	encoding.is_digits_decimal = 1;
	r_scale.p  = 1;
	r_scale.q  = 1<<SDL_AUDIO_BITSIZE(sf); //TODO: calculate properly for all edge cases!
	r_offset.p = 0;
	r_offset.q = 1;
	encoding.scale		   = r_scale;
	encoding.offset		   = r_offset;
	spec.spec_digits	   = 2;

	//meaning
	meaning.mq	 = SR_MQ_VOLTAGE;
	meaning.unit	 = SR_UNIT_VOLT;
	meaning.mqflags	 = 0;
	meaning.channels = lastcg->channels;

	//data
	uint8_t data[1024]	  = {60, 127, 0, 127, -60, -60, 0, 0};
	packet_analog.data	  = data;
	packet_analog.num_samples = 4;
	packet_analog.encoding	  = &encoding;
	packet_analog.meaning	  = &meaning;
	packet_analog.spec	  = &spec;

	uint32_t recv_bytes = 0;
	while(!recv_bytes) {
		recv_bytes = SDL_DequeueAudio(devc->sdl_open_index, data, 1024);
		SDL_Delay(10);
	}
	sr_err("SDL2 received %d bytes!", recv_bytes);

	packet_analog.num_samples =
		recv_bytes / ((SDL_AUDIO_BITSIZE(sf)/8)*devc->sdl_open_spec.channels);

	//packet
	packet.type    = SR_DF_ANALOG;
	packet.payload = &packet_analog;

	sr_session_send(sdi, &packet);
	devc->limit_samples_remaining -= packet_analog.num_samples;

	return G_SOURCE_CONTINUE;
	//return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	devc = sdi->priv;

	devc->limit_samples_remaining = devc->limit_samples;
	devc->limit_samples_remaining = 100; //FIXME: pulseview should set this!
	sr_err("Limiting samples to %lu", devc->limit_samples_remaining);

	//Initialize SDL2 recording
	devc->sdl_device_spec.callback=NULL;
	devc->sdl_device_spec.format = AUDIO_S8;
	devc->sdl_device_spec.samples = 1024/64;

	devc->sdl_open_index = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(devc->sdl_device_index, 1), 1, &devc->sdl_device_spec, &devc->sdl_open_spec, 0);
	sr_err("Open SDL2 device for capture: %d", devc->sdl_open_index);
	if(!devc->sdl_open_index) {
		sr_err("Could not open SDL2 device for capture!");
		return SR_ERR;
	}
	SDL_PauseAudioDevice(devc->sdl_open_index, 0);

	sr_session_source_add(sdi->session, -1, 0, 100, sdl_data_callback, (struct sr_dev_inst *)sdi);

	std_session_send_df_header(sdi);

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	devc = sdi->priv;

	std_session_send_df_end(sdi);

	SDL_CloseAudioDevice(devc->sdl_open_index);

	return SR_OK;
}

static struct sr_dev_driver sdl2_driver_info = {
	.name	     = "sdl2",
	.longname    = "SoundCard Audio Capture using SDL2",
	.api_version = 1,
	.init	     = init,
	.cleanup     = cleanup,

	//scan
	.scan	   = scan,
	.dev_list  = std_dev_list,
	.dev_clear = std_dev_clear,

	//config
	.config_get  = config_get,
	.config_set  = config_set,
	.config_list = config_list,

	//open
	.dev_open  = dev_open,
	.dev_close = std_dummy_dev_close,

	//acq
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop  = dev_acquisition_stop,

	//inst
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(sdl2_driver_info);
