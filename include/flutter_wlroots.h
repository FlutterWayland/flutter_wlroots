#include "stdbool.h"

struct fwr_instance;

struct fwr_instance_opts {
  const char *assets_path;
  const char *icu_data_path;
  int argc;
  const char *const *argv;
};

bool fwr_instance_create(struct fwr_instance_opts opts, struct fwr_instance **instance_out);

bool fwr_instance_run(struct fwr_instance *instance);