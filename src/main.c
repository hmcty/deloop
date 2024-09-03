/* PipeWire */
/* SPDX-FileCopyrightText: Copyright © 2022 Wim Taymans */
/* SPDX-License-Identifier: MIT */

/*
 [title]
 Audio capture using \ref pw_stream "pw_stream".
 [title]
 */

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>

#include <spa/param/audio/format-utils.h>

#include <spa/utils/ringbuffer.h>

#include <pipewire/pipewire.h>

#define DEFAULT_RATE      48000
#define DEFAULT_CHANNELS  2
#define DEFAULT_VOLUME    0.7
#define RECORD_SECONDS    60

typedef struct data {
  struct pw_main_loop *loop;

  struct pw_stream *istream;
  struct pw_stream *ostream;

  struct spa_audio_info format;
  unsigned move : 1;

  struct spa_ringbuffer music_rb;
} userdata_t;

static void process_istream(userdata_t *data) {
  struct pw_buffer *b;
  struct spa_buffer *buf;
  float *samples, max;
  uint32_t c, n, n_channels, n_samples, peak;

  if ((b = pw_stream_dequeue_buffer(data->istream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  buf = b->buffer;
  if ((samples = buf->datas[0].data) == NULL)
    return;

  n_channels = data->format.info.raw.channels;
  n_samples = buf->datas[0].chunk->size / sizeof(float);

  /* move cursor up */
  if (data->move)
    fprintf(stdout, "%c[%dA", 0x1b, n_channels + 1);
  fprintf(stdout, "captured %d samples\n", n_samples / n_channels);
  for (c = 0; c < data->format.info.raw.channels; c++) {
    max = 0.0f;
    for (n = c; n < n_samples; n += n_channels) {
      max = fmaxf(max, fabsf(samples[n]));
    }

    peak = (uint32_t)SPA_CLAMPF(max * 30, 0.f, 39.f);

    fprintf(stdout, "channel %d: |%*s%*s| peak:%f\n", c, peak + 1, "*",
            40 - peak, "", max);
  }
  data->move = true;
  fflush(stdout);

  pw_stream_queue_buffer(data->istream, b);
}

static void process_ostream(userdata_t *data) {

}

/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. consume stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void *userdata) {
  userdata_t *data = userdata;
  process_istream(data); 
  process_ostream(data);
}

/* Be notified when the stream param changes. We're only looking at the
 * format changes.
 */
static void on_stream_param_changed(void *_data, uint32_t id,
                                    const struct spa_pod *param) {
  struct data *data = _data;

  /* NULL means to clear the format */
  if (param == NULL || id != SPA_PARAM_Format)
    return;

  if (spa_format_parse(param, &data->format.media_type,
                       &data->format.media_subtype) < 0)
    return;

  /* only accept raw audio */
  if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
      data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
    return;

  /* call a helper function to parse the format for us. */
  spa_format_audio_raw_parse(param, &data->format.info.raw);

  fprintf(stdout, "capturing rate:%d channels:%d\n", data->format.info.raw.rate,
          data->format.info.raw.channels);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_process,
};

static void do_quit(void *userdata, int signal_number) {
  struct data *data = userdata;
  pw_main_loop_quit(data->loop);
}

int main(int argc, char *argv[]) {
  struct data data = {
      0,
  };
  uint8_t buffer[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

  spa_ringbuffer_init(&data.music_rb);
  spa_ringbuffer_set_avail(
    &data.music_rb,
    DEFAULT_RATE * DEFAULT_CHANNELS * RECORD_SECONDS * sizeof(float);

  pw_init(&argc, &argv);

  /* make a main loop. If you already have another main loop, you can add
   * the fd of this pipewire mainloop to it. */
  data.loop = pw_main_loop_new(NULL);

  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

  /* Create a simple stream, the simple stream manages the core and remote
   * objects for you if you don't need to deal with them.
   *
   * If you plan to autoconnect your stream, you need to provide at least
   * media, category and role properties.
   *
   * Pass your events and a user_data pointer as the last arguments. This
   * will inform you about the stream state. The most important event
   * you need to listen to is the process event where you need to produce
   * the data.
   */
  struct pw_properties *iprops = pw_properties_new(
    PW_KEY_MEDIA_TYPE, "Audio",
    PW_KEY_CONFIG_NAME, "client-rt.conf",
    PW_KEY_MEDIA_CATEGORY, "Capture",
    PW_KEY_MEDIA_ROLE, "Music",
    NULL);
  if (argc > 1)
    /* Set stream target if given on command line */
    pw_properties_set(iprops, PW_KEY_TARGET_OBJECT, argv[1]);

  /* uncomment if you want to capture from the sink monitor ports */
  /* pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true"); */

  data.istream = pw_stream_new_simple(
    pw_main_loop_get_loop(data.loop),
    "audio-capture",
    iprops, &stream_events, &data);

  /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
   * id means that this is a format enumeration (of 1 value).
   * We leave the channels and rate empty to accept the native graph
   * rate and channels. */
  const struct spa_pod *iparams[1];
  iparams[0] = spa_format_audio_raw_build(
      &b, SPA_PARAM_EnumFormat,
      &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32));

  /* Now connect this stream. We ask that our process function is
   * called in a realtime thread. */
  pw_stream_connect(
    data.istream,
    PW_DIRECTION_INPUT,
    PW_ID_ANY,
    PW_STREAM_FLAG_AUTOCONNECT
      | PW_STREAM_FLAG_MAP_BUFFERS
      | PW_STREAM_FLAG_RT_PROCESS,
    iparams, 1);

  // OUTPUT
  struct pw_properties *oprops = pw_properties_new(
    PW_KEY_MEDIA_TYPE, "Audio",
    PW_KEY_MEDIA_CATEGORY, "Playback",
    PW_KEY_MEDIA_ROLE, "Music",
    NULL);

  /* uncomment if you want to capture from the sink monitor ports */
  /* pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true"); */

  data.ostream = pw_stream_new_simple(
    pw_main_loop_get_loop(data.loop),
    "audio-capture",
    oprops, &stream_events, &data);

  const struct spa_pod *oparams[1];
  oparams[0] = spa_format_audio_raw_build(
      &b, SPA_PARAM_EnumFormat,
      &SPA_AUDIO_INFO_RAW_INIT(
        .format = SPA_AUDIO_FORMAT_F32,
        .channels = DEFAULT_CHANNELS,
        .rate = DEFAULT_RATE));

  pw_stream_connect(
    data.ostream,
    PW_DIRECTION_OUTPUT,
    PW_ID_ANY,
    PW_STREAM_FLAG_AUTOCONNECT
      | PW_STREAM_FLAG_MAP_BUFFERS
      | PW_STREAM_FLAG_RT_PROCESS,
    oparams, 1);

  /* and wait while we let things run */
  pw_main_loop_run(data.loop);

  pw_stream_destroy(data.istream);
  pw_main_loop_destroy(data.loop);
  pw_deinit();

  return 0;
}
