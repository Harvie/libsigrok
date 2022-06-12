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

#include <config.h>
#include "protocol.h"
#include <SDL2/SDL.h>

static const uint32_t drvopts[] = {
	//SR_CONF_DEMO_DEV,
	SR_CONF_OSCILLOSCOPE,
	SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
	SR_CONF_LIMIT_SAMPLES | SR_CONF_SET,
	SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
	SR_CONF_TRIGGER_SLOPE | SR_CONF_SET,
	SR_CONF_HORIZ_TRIGGERPOS | SR_CONF_SET,
	SR_CONF_CAPTURE_RATIO | SR_CONF_SET,
	SR_CONF_RLE | SR_CONF_SET,
};

/*
 * Channels are numbered 0 to 7.
 *
 * See also: http://www.linkinstruments.com/images/mso19_1113.gif
 */
static const char *channel_names[] = {
	/* Note: DSO needs to be first. */
	"DSO", "0", "1", "2", "3", "4", "5", "6", "7",
};

static const uint64_t samplerates[] = {
	SR_HZ(100),
	SR_MHZ(200),
	SR_HZ(100),
};

static const char *trigger_slopes[2] = {
	"r", "f",
};

static int init(struct sr_dev_driver *di, struct sr_context *sr_ctx) {
	SDL_Init(SDL_INIT_AUDIO);
	return std_init(di, sr_ctx);
}

static int cleanup(const struct sr_dev_driver *di) {
	SDL_Quit();
	return std_cleanup(di);
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	(void)options;

        struct dev_context *devc;
        struct sr_dev_inst *sdi;
        struct sr_channel *ch;
        struct sr_channel_group *cg, *acg;
/*
        struct sr_config *src;
        struct analog_gen *ag;
        GSList *l;
        int num_logic_channels, num_analog_channels, pattern, i;
        uint64_t limit_frames;
        char channel_name[16];

        num_logic_channels = DEFAULT_NUM_LOGIC_CHANNELS;
        num_analog_channels = DEFAULT_NUM_ANALOG_CHANNELS;
        limit_frames = DEFAULT_LIMIT_FRAMES;
*/


	//Create device instance
        sdi = g_malloc0(sizeof(struct sr_dev_inst));
        sdi->status = SR_ST_INACTIVE;
        sdi->model = g_strdup("SDL2 Soundcard");

	//Put driver specific data to driver instance
        devc = g_malloc0(sizeof(struct dev_context));
        devc->cur_samplerate = SR_KHZ(200);
        sdi->priv = devc;

        //Create analog channel group
        acg = g_malloc0(sizeof(struct sr_channel_group));
        acg->name = g_strdup("Analog");
        sdi->channel_groups = g_slist_append(sdi->channel_groups, acg);

	//Put new channel to group
        ch = sr_channel_new(sdi, 1, SR_CHANNEL_ANALOG, TRUE, "channel_name");
        acg->channels = g_slist_append(acg->channels, ch);

	//one more channel
        ch = sr_channel_new(sdi, 2, SR_CHANNEL_ANALOG, TRUE, "another_ch");
        acg->channels = g_slist_append(acg->channels, ch);


        return std_scan_complete(di, g_slist_append(NULL, sdi));







}

static int dev_open(struct sr_dev_inst *sdi)
{
	int ret;
	struct dev_context *devc;

	devc = sdi->priv;

	//if (serial_open(devc->serial, SERIAL_RDWR) != SR_OK)
	//	return SR_ERR;

	/* FIXME: discard serial buffer */
	//mso_check_trigger(devc->serial, &devc->trigger_state);
	//sr_dbg("Trigger state: 0x%x.", devc->trigger_state);

	//ret = mso_reset_adc(sdi);
	//if (ret != SR_OK)
	//	return ret;

	//mso_check_trigger(devc->serial, &devc->trigger_state);
	//sr_dbg("Trigger state: 0x%x.", devc->trigger_state);

	//    ret = mso_reset_fsm(sdi);
	//    if (ret != SR_OK)
	//            return ret;
	//    return SR_ERR;

	return SR_OK;
}

static int config_get(int key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;

	(void)cg;

	if (!sdi)
		return SR_ERR_ARG;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_SAMPLERATE:
		*data = g_variant_new_uint64(devc->cur_rate);
		break;
	default:
		return SR_ERR_NA;
	}

	return SR_OK;
}

static int config_set(int key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;
	uint64_t num_samples;
	const char *slope;
	int trigger_pos;
	double pos;

	(void)cg;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_SAMPLERATE:
		// FIXME
		return mso_configure_rate(sdi, g_variant_get_uint64(data));
	case SR_CONF_LIMIT_SAMPLES:
		num_samples = g_variant_get_uint64(data);
		if (num_samples != 1024) {
			sr_err("Only 1024 samples are supported.");
			return SR_ERR_ARG;
		}
		devc->limit_samples = num_samples;
		break;
	case SR_CONF_CAPTURE_RATIO:
		break;
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
		trigger_pos = (int)pos;
		devc->trigger_holdoff[0] = trigger_pos & 0xff;
		break;
	case SR_CONF_RLE:
		break;
	default:
		return SR_ERR_NA;
	}

	return SR_OK;
}

static int config_list(int key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	switch (key) {
	case SR_CONF_DEVICE_OPTIONS:
		return STD_CONFIG_LIST(key, data, sdi, cg, NO_OPTS, drvopts, devopts);
	case SR_CONF_SAMPLERATE:
		*data = std_gvar_samplerates_steps(ARRAY_AND_SIZE(samplerates));
		break;
	case SR_CONF_TRIGGER_MATCH:
		*data = g_variant_new_string(TRIGGER_TYPE);
		break;
	default:
		return SR_ERR_NA;
	}

	return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	int ret = SR_ERR;

	devc = sdi->priv;

	if (mso_configure_channels(sdi) != SR_OK) {
		sr_err("Failed to configure channels.");
		return SR_ERR;
	}

	/* FIXME: No need to do full reconfigure every time */
//      ret = mso_reset_fsm(sdi);
//      if (ret != SR_OK)
//              return ret;

	/* FIXME: ACDC Mode */
	devc->ctlbase1 &= 0x7f;
//      devc->ctlbase1 |= devc->acdcmode;

	ret = mso_configure_rate(sdi, devc->cur_rate);
	if (ret != SR_OK)
		return ret;

	/* set dac offset */
	ret = mso_dac_out(sdi, devc->dac_offset);
	if (ret != SR_OK)
		return ret;

	ret = mso_configure_threshold_level(sdi);
	if (ret != SR_OK)
		return ret;

	//ret = mso_configure_trigger(sdi);
	//if (ret != SR_OK)
	//	return ret;

	/* END of config hardware part */
	ret = mso_arm(sdi);
	if (ret != SR_OK)
		return ret;

	/* Start acquisition on the device. */
	//mso_check_trigger(devc->serial, &devc->trigger_state);
	//ret = mso_check_trigger(devc->serial, NULL);
	//if (ret != SR_OK)
	//	return ret;

	/* Reset trigger state. */
	//devc->trigger_state = 0x00;

	std_session_send_df_header(sdi);

	/* Our first channel is analog, the other 8 are of type 'logic'. */
	/* TODO. */

	serial_source_add(sdi->session, devc->serial, G_IO_IN, -1,
			mso_receive_data, sdi);

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	stop_acquisition(sdi);

	return SR_OK;
}

static struct sr_dev_driver sdl2_driver_info = {
	.name = "sdl2",
	.longname = "SoundCard Audio Capture using SDL2",
	.api_version = 1,
	.init = init,
	.cleanup = cleanup,

	//scan
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,

	//config
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,

	//open
	.dev_open = dev_open,
	.dev_close = std_serial_dev_close,

	//acq
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,

	//inst
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(sdl2_driver_info);
