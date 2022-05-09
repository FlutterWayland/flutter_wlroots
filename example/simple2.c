#include "flutter_wlroots.h"

int main(int argc, const char *const argv[]) {
    struct fwr_instance_opts opts = {};
    opts.argc = argc;
    opts.argv = &argv[0];
    opts.assets_path = "flutter_assets";
    opts.icu_data_path = "icudtl.dat";

    struct fwr_instance *instance;
    if (fwr_instance_create(opts, &instance)) {
        fwr_instance_run(instance);
    }

    printf("main exiting\n");
    return 0;
}
