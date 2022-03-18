#include <stdint.h>
#include <stdlib.h>

#include <wayland-util.h>
#include <instance.h>

#define WLR_USE_UNSTABLE
#include <wlr/util/log.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/egl.h>

#include "renderer.h"

static void output_frame(struct wl_listener *listener, void *data) {
  struct fwr_output *output = wl_container_of(listener, output, frame);
  struct fwr_instance *instance = output->instance;
  struct wlr_output *wlr_output = output->wlr_output;
  //wlr_log(WLR_INFO, "output frame %d", instance->out_tex);

  int64_t vsync_baton = output->instance->vsync_baton;
  output->instance->vsync_baton = 0;
  if (vsync_baton != 0) {
    uint64_t current_time = output->instance->fl_proc_table.GetCurrentTime();
    output->instance->fl_proc_table.OnVsync(
      output->instance->engine, 
      vsync_baton,
      current_time,
      current_time + 16600000
    );
    wlr_log(WLR_DEBUG, "Returning baton");
  }

  //wlr_log(WLR_INFO, "render");

  wlr_output_attach_render(wlr_output, NULL);

  struct wlr_renderer *renderer = instance->renderer;
  wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

  float color[4] = {0.0, 0.1, 0.0, 1.0};
  wlr_renderer_clear(renderer, color);

  //fwr_renderer_render_flutter_buffer(instance);
  fwr_renderer_render_scene(instance);

	wlr_output_render_software_cursors(wlr_output, NULL);

  wlr_renderer_end(renderer);

  wlr_output_commit(wlr_output);
}

static void output_mode(struct wl_listener *listener, void *data) {
  struct fwr_output *output = wl_container_of(listener, output, mode);
  struct wlr_output *wlr_output = data;

  wlr_log(WLR_INFO, "Set output mode");

  if (output->instance->engine != NULL) {
    FlutterWindowMetricsEvent window_metrics = {};
    window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
    window_metrics.width = wlr_output->width;
    window_metrics.height = wlr_output->height;
    window_metrics.pixel_ratio = 1.0;
    FlutterEngineSendWindowMetricsEvent(output->instance->engine, &window_metrics);
  }
}

void fwr_server_new_output(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, new_output);
  struct wlr_output *wlr_output = data;

  if (instance->output != NULL) {
    // Only ever have a single output.
    // This will be the first output we get, all others are ignored.
    return;
  }

  wlr_output_init_render(wlr_output, instance->allocator, instance->renderer);

	struct fwr_output *output =
	  calloc(1, sizeof(struct fwr_output));
	output->wlr_output = wlr_output;
	output->instance = instance;
	///* Sets up a listener for the frame notify event. */
	output->frame.notify = output_frame;
	wl_signal_add(&wlr_output->events.frame, &output->frame);

  output->mode.notify = output_mode;
  wl_signal_add(&wlr_output->events.mode, &output->mode);
  
  instance->output = output;

  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    wlr_output_set_mode(wlr_output, mode);
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output)) {
      return;
    }

    wlr_log(WLR_INFO, "Setting mode when creating new output!");

    if (output->instance->engine != NULL) {
      FlutterWindowMetricsEvent window_metrics = {};
      window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
      window_metrics.width = mode->width;
      window_metrics.height = mode->height;
      window_metrics.pixel_ratio = 1.0;
      FlutterEngineSendWindowMetricsEvent(instance->engine, &window_metrics);
    }
  }

  wlr_output_layout_add_auto(instance->output_layout, wlr_output);
}