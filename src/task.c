#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>

#define WLR_USE_UNSTABLE
#include <wlr/util/log.h>
#include <wlr/render/egl.h>

#include "instance.h"

pid_t get_tid() {
  #ifdef SYS_gettid
  return syscall(SYS_gettid);
  #else
  #error "SYS_gettid unavailable on this system"
  #endif
}

struct fwr_render_task {
  struct fwr_render_task *next;
  struct fwr_instance *instance;
  FlutterTask task;
  uint64_t target_time;
};

// Must be called with a lock on instance->render_task_list_mutex
void engine_platform_task_queue_schedule(struct fwr_instance *instance) {
  struct fwr_render_task *next = instance->queued_platform_tasks;
  uint64_t first_task_ns = UINT64_MAX;
  while (next != NULL) {
    if (first_task_ns > next->target_time) {
      first_task_ns = next->target_time;
    }
    next = next->next;
  }

  if (first_task_ns != UINT16_MAX) {
    uint64_t current_time_nanos = instance->fl_proc_table.GetCurrentTime();
    if (first_task_ns <= current_time_nanos) {
      // Run right away
      eventfd_write(instance->platform_notify_fd, 1);
      // Disable timer
      wl_event_source_timer_update(instance->platform_timer_event_source, 0);
    } else {
      int64_t diff_ns = first_task_ns - current_time_nanos;
      int64_t diff_ms = diff_ns / 1000000;

      // Enable timer
      wl_event_source_timer_update(instance->platform_timer_event_source, diff_ms + 1);
    }
  }
}

void engine_platform_task_queue_process(void *user_data) {
  struct fwr_instance *instance = user_data;

  uint64_t current_time_nanos = instance->fl_proc_table.GetCurrentTime();

  pthread_mutex_lock(&instance->platform_task_list_mutex);

  struct fwr_render_task *head = instance->queued_platform_tasks;
  instance->queued_platform_tasks = NULL;

  struct fwr_render_task *new_head = NULL;
  struct fwr_render_task *last_requeued = NULL;

  while (head != NULL) {
    struct fwr_render_task *curr = head;
    if (current_time_nanos > head->target_time) {
      //wlr_log(WLR_DEBUG, "running %ld", head->task.task);

      if (instance->fl_proc_table.RunTask(instance->engine, &curr->task) != kSuccess) {
          wlr_log(WLR_ERROR, "Failed to run render task?!");
      }

      head = curr->next;
      free(curr);
    } else {
      wlr_log(WLR_DEBUG, "requeueing %ld", head->task.task);
      // Keep in queue for later
      if (new_head == NULL) {
        last_requeued = curr;
      }
      head = curr->next;
      curr->next = new_head;
      new_head = curr;
    }
  }

  // If we have any requeues, we need to add them back into the queue.
  if (last_requeued != NULL) {
    last_requeued->next = instance->queued_platform_tasks;
    instance->queued_platform_tasks = new_head;
  }

  engine_platform_task_queue_schedule(instance);

  pthread_mutex_unlock(&instance->platform_task_list_mutex);
}

// Callback from the event loop, on task timer expiry.
static int engine_platform_task_queue_timer(void *user_data) {
  engine_platform_task_queue_process(user_data);
  return 0;
}

// Callback from event loop, on notifyfd.
static int engine_renderer_task_queue_notifyfd(int fd, uint32_t mask, void *user_data) {
  struct fwr_instance *instance = user_data;

  // Reset eventfd
  eventfd_t value;
  eventfd_read(instance->platform_notify_fd, &value);

  // Process tasks.
  engine_platform_task_queue_process(user_data);

  return 0;
}

static bool engine_cb_platform_runs_on_current_thread(void *user_data) {
  struct fwr_instance *instance = user_data;
  return get_tid() == instance->platform_tid;
}
static void engine_cb_platform_post_task(FlutterTask task, uint64_t target_time, void *user_data) {
  struct fwr_instance *instance = user_data;

  pthread_mutex_lock(&instance->platform_task_list_mutex);

  struct fwr_render_task *curr = calloc(1, sizeof(struct fwr_render_task));
  curr->task = task;
  curr->instance = instance;
  curr->next = instance->queued_platform_tasks;
  curr->target_time = target_time;

  instance->queued_platform_tasks = curr;

  engine_platform_task_queue_schedule(instance);

  pthread_mutex_unlock(&instance->platform_task_list_mutex);
}

void fwr_tasks_init(struct fwr_instance *instance) {
  instance->platform_tid = get_tid();

  instance->platform_notify_fd = eventfd(0, 0);
  instance->platform_notify_event_source = wl_event_loop_add_fd(
    instance->wl_event_loop, instance->platform_notify_fd, WL_EVENT_READABLE, engine_renderer_task_queue_notifyfd, instance);

  instance->platform_timer_event_source = wl_event_loop_add_timer(
    instance->wl_event_loop, engine_platform_task_queue_timer, instance);

  instance->platform_task_runner.struct_size = sizeof(FlutterTaskRunnerDescription);
  instance->platform_task_runner.identifier = 0;
  instance->platform_task_runner.user_data = instance;
  instance->platform_task_runner.runs_task_on_current_thread_callback = engine_cb_platform_runs_on_current_thread;
  instance->platform_task_runner.post_task_callback = engine_cb_platform_post_task;

  instance->custom_task_runners.struct_size = sizeof(FlutterCustomTaskRunners);
  instance->custom_task_runners.platform_task_runner = &instance->platform_task_runner;
}