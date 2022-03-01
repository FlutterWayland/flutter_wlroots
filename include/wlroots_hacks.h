
// Normally private symbols we need access to.
// This needs to be done because access to the `wlr_egl` instance is normally not available
// from the renderer.
// We instead create the egl instance + renderer manually.
struct wlr_egl *wlr_egl_create_with_drm_fd(int drm_fd);
void wlr_egl_destroy(struct wlr_egl *egl);
