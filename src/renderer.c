#include "renderer.h"
#include "flutter_embedder.h"
#include "instance.h"
#include "shaders.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-util.h>

#define WLR_USE_UNSTABLE
#include <wlr/render/egl.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_output_layout.h>

#include "EGL/egl.h"

#define GL_BGRA_EXT 0x80E1

static void texture_destruction_callback(void *user_data) {}

static struct fwr_renderer_page_texture* page_get_texture(struct fwr_instance *instance, size_t width, size_t height, bool make_fbo) {
  struct fwr_renderer *renderer = &instance->fwr_renderer;
  struct gl_fns *fns = &renderer->fns;
  struct fwr_renderer_page *page = &renderer->pages[renderer->current_page];

  bool found = false;
  struct fwr_renderer_page_texture *page_texture;
  wl_list_for_each(page_texture, &page->unused_textures, link) {
    if (page_texture->width == width && page_texture->height == height) {
      found = true;
      break;
    }
  }

  if (found) {
    wl_list_remove(&page_texture->link);
  } else {
    GLuint err;
    GLuint fbo = 0;

    if (make_fbo) {
      fns->glGenFramebuffers(1, &fbo);
      wlr_log(WLR_INFO, "fbo: %d", fbo);

      fns ->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    GLuint tex = 0;
    fns->glGenTextures(1, &tex);
    wlr_log(WLR_INFO, "tex: %d", tex);

    fns->glBindTexture(GL_TEXTURE_2D, tex);
    fns->glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
    fns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    fns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (make_fbo) {
      fns->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

      GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
      fns->glDrawBuffers(1, drawBuffers);

      fns->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    fns->glBindTexture(GL_TEXTURE_2D, 0);

    err = fns->glGetError();
    if (err != 0) {
      wlr_log(WLR_ERROR, "GL ERROR %d", err);
    }

    page_texture = calloc(1, sizeof(struct fwr_renderer_page_texture));
    page_texture->page = page;
    page_texture->texture = tex;
    page_texture->fbo = fbo;
    page_texture->width = width;
    page_texture->height = height;
  }

  wl_list_insert(&page->textures, &page_texture->link);

  return page_texture;
}

static bool create_backing_store(const FlutterBackingStoreConfig *config, FlutterBackingStore *backing_store_out, void *user_data) {
  //wlr_log(WLR_INFO, "Create backing store");

  struct fwr_instance *instance = user_data;

  struct fwr_renderer_page_texture *page_texture = page_get_texture(instance, config->size.width, config->size.height, false);

  backing_store_out->struct_size = sizeof(FlutterBackingStore);
  backing_store_out->user_data = page_texture;
  backing_store_out->type = kFlutterBackingStoreTypeOpenGL;
  backing_store_out->did_update = false;

  //backing_store_out->open_gl.type = kFlutterOpenGLTargetTypeFramebuffer;
  //backing_store_out->open_gl.framebuffer.target = GL_TEXTURE_2D;
  //backing_store_out->open_gl.framebuffer.name = page_texture->fbo;
  //backing_store_out->open_gl.framebuffer.user_data = page_texture;
  //backing_store_out->open_gl.framebuffer.destruction_callback = texture_destruction_callback;

  backing_store_out->open_gl.type = kFlutterOpenGLTargetTypeTexture;
  backing_store_out->open_gl.texture.target = GL_TEXTURE_2D;
  backing_store_out->open_gl.texture.name = page_texture->texture;
  backing_store_out->open_gl.texture.format = 0x93A1; //GL_BGRA_EXT; //0x8058; //GL_RGBA;
  backing_store_out->open_gl.texture.user_data = page_texture;
  backing_store_out->open_gl.texture.destruction_callback = texture_destruction_callback;
  backing_store_out->open_gl.texture.width = page_texture->width;
  backing_store_out->open_gl.texture.height = page_texture->height;

  return true;
}

static bool collect_backing_store(const FlutterBackingStore *backing_store, void *user_data) {
  //wlr_log(WLR_INFO, "Collect backing store");
  return true;
}

static bool present_layers(const FlutterLayer** f_layers, size_t layers_count, void *user_data) {
  //wlr_log(WLR_INFO, "Present layers");

  struct fwr_instance *instance = user_data;
  struct fwr_renderer *renderer = &instance->fwr_renderer;

  pthread_mutex_lock(&renderer->render_mutex);

  // Deallocate current scene
  for (int i = 0; i < renderer->current_scene.layers_count; i++) {
    struct fwr_renderer_scene_layer *layer = &renderer->current_scene.layers[i];
    if (layer->type == sceneLayerPlatform) {
      free(layer->platform.mutations);
    }
  }
  free(renderer->current_scene.layers);

  // Copy layers array into a scene description we own so it can be accessed from the main thread.
  struct fwr_renderer_scene_layer *layers = calloc(layers_count, sizeof(struct fwr_renderer_scene_layer));

  for (int i = 0; i < layers_count; i++) {
    const FlutterLayer *f_layer = f_layers[i];
    struct fwr_renderer_scene_layer *layer = &layers[i];

    if (f_layer->type == kFlutterLayerContentTypeBackingStore) {
      layer->type = sceneLayerTexture;
      layer->texture.texture = f_layer->backing_store->user_data;
    } else if (f_layer->type == kFlutterLayerContentTypePlatformView) {
      const FlutterPlatformView *platform_view = f_layer->platform_view;

      layer->type = sceneLayerPlatform;
      layer->offset = f_layer->offset;
      layer->size = f_layer->size;
      layer->platform.platform_view_id = platform_view->identifier;
      layer->platform.mutations_count = platform_view->mutations_count;
      
      FlutterPlatformViewMutation *mutations = calloc(platform_view->mutations_count, sizeof(FlutterPlatformViewMutation));
      for (int k = 0; k < platform_view->mutations_count; k++) {
        const FlutterPlatformViewMutation *f_mutation = platform_view->mutations[k];
        mutations[k] = *f_mutation;
      }
      layer->platform.mutations = mutations;
    }
  }

  renderer->current_scene.layers_count = layers_count;
  renderer->current_scene.layers = layers;

  uint8_t last_page_idx;
  if (renderer->current_page == 0) {
    last_page_idx = 1;
  } else {
    last_page_idx = 0;
  }

  // Textures which are unused from current page can be deallocated.
  struct fwr_renderer_page *page = &renderer->pages[renderer->current_page];
  struct fwr_renderer_page_texture *page_texture;
  struct fwr_renderer_page_texture *tmp;
  wl_list_for_each_safe(page_texture, tmp, &page->unused_textures, link) {
    renderer->fns.glDeleteTextures(1, &page_texture->texture);
    wl_list_remove(&page_texture->link);
    free(page_texture);
  }

  // If present, delete last sync object.
  if (renderer->current_scene.sync != 0) {
    eglDestroySync(instance->egl->display, renderer->current_scene.sync);
  }

  // Create a sync object for new backpage.
  renderer->current_scene.sync = eglCreateSync(instance->egl->display, EGL_SYNC_FENCE, NULL);

  // All textures in former backpage are now unused and available for reuse.
  struct fwr_renderer_page *last_page = &renderer->pages[last_page_idx];
  wl_list_for_each_safe(page_texture, tmp, &last_page->textures, link) {
    wl_list_remove(&page_texture->link);
    wl_list_insert(&last_page->unused_textures, &page_texture->link);
  }

  // Flip the page
  if (renderer->current_page == 0) {
    renderer->current_page = 1;
  } else {
    renderer->current_page = 0;
  }

  pthread_mutex_unlock(&renderer->render_mutex);
  return true;
}

void fwr_renderer_init(struct fwr_instance *instance, gl_resolve_fn resolver) {
  struct fwr_renderer *renderer = &instance->fwr_renderer;
  struct gl_fns *fns = &renderer->fns;

  fns->glGenFramebuffers = (void (*)(GLsizei, GLuint*)) resolver("glGenFramebuffers");
  fns->glBindFramebuffer = (void (*)(GLenum, GLuint)) resolver("glBindFramebuffer");
  fns->glGenTextures = (void (*)(GLsizei, GLuint*)) resolver("glGenTextures");
  fns->glBindTexture = (void (*)(GLenum, GLuint)) resolver("glBindTexture");
  fns->glTexImage2D = (void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) resolver("glTexImage2D");
  fns->glTexParameteri = (void (*)(GLenum, GLenum, GLint)) resolver("glTexParameteri");
  fns->glFramebufferTexture = (void (*)(GLenum, GLenum, GLuint, GLint)) resolver("glFramebufferTexture");
  fns->glDrawBuffers = (void (*)(GLsizei, const GLenum*)) resolver("glDrawBuffers");
  fns->glCreateShader = (GLuint (*)(GLenum)) resolver("glCreateShader");
  fns->glShaderSource = (void (*)(GLuint, GLsizei, const GLchar**, const GLint*)) resolver("glShaderSource");
  fns->glCompileShader = (void (*)(GLuint)) resolver("glCompileShader");
  fns->glGetShaderiv = (void (*)(GLuint, GLenum, GLint*)) resolver("glGetShaderiv");
  fns->glDeleteShader = (void (*)(GLuint)) resolver("glDeleteShader");
  fns->glCreateProgram = (GLuint (*)()) resolver("glCreateProgram");
  fns->glAttachShader = (void (*)(GLuint, GLuint)) resolver("glAttachShader");
  fns->glLinkProgram = (void (*)(GLuint)) resolver("glLinkProgram");
  fns->glDetachShader = (void (*)(GLuint, GLuint)) resolver("glDetachShader");;
  fns->glGetProgramiv = (void (*)(GLuint, GLenum, GLint*)) resolver("glGetProgramiv");
  fns->glDeleteProgram = (void (*)(GLuint)) resolver("glDeleteProgram");
  fns->glGetUniformLocation = (GLint (*)(GLuint, const GLchar*)) resolver("glGetUniformLocation");
  fns->glGetAttribLocation = (GLint (*)(GLuint, const GLchar*)) resolver("glGetAttribLocation");
  fns->glActiveTexture = (void (*)(GLenum)) resolver("glActiveTexture");
  fns->glUseProgram = (void (*)(GLuint)) resolver("glUseProgram");
  fns->glUniformMatrix3fv = (void (*)(GLint, GLsizei, GLboolean, const GLfloat*)) resolver("glUniformMatrix3fv");
  fns->glUniform1i = (void (*)(GLint, GLint)) resolver("glUniform1i");
  fns->glUniform1f = (void (*)(GLint, GLfloat)) resolver("glUniform1f");
  fns->glVertexAttribPointer = (void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) resolver("glVertexAttribPointer");
  fns->glEnableVertexAttribArray = (void (*)(GLuint)) resolver("glEnableVertexAttribArray");
  fns->glDrawArrays = (void (*)(GLenum, GLint, GLsizei)) resolver("glDrawArrays");
  fns->glDisableVertexAttribArray = (void (*)(GLuint)) resolver("glDisableVertexAttribArray");
  fns->glEnable = (void (*)(GLenum)) resolver("glEnable");
  fns->glDisable = (void (*)(GLenum)) resolver("glDisable");
  fns->glGetTextureImage = (void (*)(GLuint, GLint, GLenum, GLenum, GLsizei, void*)) resolver("glGetTextureImage");
  fns->glCheckFramebufferStatus = (GLenum (*)(GLenum)) resolver("glCheckFramebufferStatus");
  fns->glGetError = (GLenum (*)()) resolver("glGetError");
  fns->glFramebufferTexture2D = (void (*)(GLenum, GLenum, GLenum, GLuint, GLint)) resolver("glFramebufferTexture2D");
  fns->glGenBuffers = (void (*)(GLsizei, GLuint*)) resolver("glGenBuffers");
  fns->glBindBuffer = (void (*)(GLenum, GLuint)) resolver("glBindBuffer");
  fns->glBufferData = (void (*)(GLenum, GLsizeiptr, const void*, GLenum)) resolver("glBufferData");
  fns->glClearColor = (void (*)(GLfloat, GLfloat, GLfloat, GLfloat)) resolver("glClearColor");
  fns->glClear = (void (*)(GLbitfield)) resolver("glClear");
  fns->glDeleteFramebuffers = (void (*)(GLsizei, GLuint*)) resolver("glDeleteFramebuffers");
  fns->glDeleteTextures = (void (*)(GLsizei, GLuint*)) resolver("glDeleteTextures");
  fns->glBindSampler = (void (*)(GLuint, GLuint)) resolver("glBindSampler");
  fns->glBlendFuncSeparate = (void (*)(GLenum, GLenum, GLenum, GLenum)) resolver("glBlendFuncSeparate");

  fns->glGetIntegerv = (void (*)(GLenum, GLint*)) resolver("glGetIntegerv");
  fns->glGetBooleanv = (void (*)(GLenum, GLboolean*)) resolver("glGetBooleanv");

  //fns->glPushClientAttrib = (void (*)(GLbitfield)) resolver("glPushClientAttrib");
  //fns->glPopClientAttrib = (void (*)(GLbitfield)) resolver("glPopClientAttrib");

  const char *egl_exts = eglQueryString(instance->egl->display, EGL_EXTENSIONS);
  // NOTE(hansihe): Using substring here is not 100% correct but good enough.
  // If a extension is called "EGL_IMG_context_priority_whatever" or something,
  // this will return a false positive.
  bool ext_context_priority = strstr(egl_exts, "EGL_IMG_context_priority") != NULL;

  size_t atti = 0;
  EGLint flutter_context_attribs[5];
  flutter_context_attribs[atti++] = EGL_CONTEXT_CLIENT_VERSION;
  flutter_context_attribs[atti++] = 2;
  if (ext_context_priority) {
    flutter_context_attribs[atti++] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
    flutter_context_attribs[atti++] = EGL_CONTEXT_PRIORITY_HIGH_IMG;
  }
  flutter_context_attribs[atti++] = EGL_NONE;
  
  renderer->flutter_egl_context = eglCreateContext(
    instance->egl->display, EGL_NO_CONFIG_KHR, instance->egl->context, flutter_context_attribs);
  if (renderer->flutter_egl_context == NULL) {
    wlr_log(WLR_ERROR, "Could not create flutter EGL context!");
  }

  renderer->flutter_resource_egl_context = eglCreateContext(
    instance->egl->display, EGL_NO_CONFIG_KHR, renderer->flutter_egl_context, flutter_context_attribs);
  if (renderer->flutter_resource_egl_context == NULL) {
    wlr_log(WLR_ERROR, "Could not create flutter resource EGL context!");
  }

  renderer->current_page = 0;
  renderer->current_scene.layers_count = 0;

  // TODO: check and print context priority?

  if (pthread_mutex_init(&renderer->render_mutex, NULL) != 0) {
    wlr_log(WLR_ERROR, "Could not init render mutex");
  }

  eglMakeCurrent(instance->egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, instance->fwr_renderer.flutter_egl_context);

  //wlr_egl_make_current(instance->egl);
  renderer->quad_rgbx_shader = make_quad_rgbx_shader(instance);

  const GLfloat x1 = 0.0;
  const GLfloat x2 = 1.0;
  const GLfloat y1 = 0.0;
  const GLfloat y2 = 1.0;
  const GLfloat texcoords[] = {
    x2, y2,
    x1, y2,
    x2, y1,
    x1, y1
  };

  fns->glGenBuffers(1, &renderer->tex_coord_buffer);
  fns->glBindBuffer(GL_ARRAY_BUFFER, renderer->tex_coord_buffer);
  fns->glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

  fns->glGenBuffers(1, &renderer->quad_vert_buffer);
  fns->glBindBuffer(GL_ARRAY_BUFFER, renderer->quad_vert_buffer);
  fns->glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

  instance->fl_compositor.struct_size = sizeof(FlutterCompositor);
  instance->fl_compositor.user_data = instance;
  instance->fl_compositor.avoid_backing_store_cache = true;
  instance->fl_compositor.create_backing_store_callback = create_backing_store;
  instance->fl_compositor.collect_backing_store_callback = collect_backing_store;
  instance->fl_compositor.present_layers_callback = present_layers;

  for (int i = 0; i < FWR_RENDERER_NUM_PAGES; i++) {
    struct fwr_renderer_page *page = &renderer->pages[i];
    page->instance = instance;
    wl_list_init(&page->textures);
    wl_list_init(&page->unused_textures);
  }

  eglMakeCurrent(instance->egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

static void render_scene_layer_platform(struct fwr_instance *instance, struct fwr_renderer_scene_layer *layer, struct timespec *now) {
  //wlr_log(WLR_INFO, "pos: %f %f size: %f %f", layer->offset.x, layer->offset.y, layer->size.width, layer->size.height);

  uint32_t view_handle = layer->platform.platform_view_id;
  struct fwr_view *view;
  if (!handle_map_get(instance->views, view_handle, (void**) &view)) {
    wlr_log(WLR_ERROR, "Got invalid view handle!");
  }

  struct wlr_texture *texture = wlr_surface_get_texture(view->surface->surface);
  if (texture == NULL) return;
  //wlr_log(WLR_INFO, "tex: %ld", (uint64_t) texture);

  float matrix[9];
  enum wl_output_transform transform = wlr_output_transform_invert(view->surface->surface->current.transform);

  double ox = 0, oy = 0;
  // This shouldn't be needed because we render to the opengl viewport directly without
  // the layout transformations.
  // wlr_output_layout_output_coords(
  //     view->instance->output_layout, instance->output->wlr_output, &ox, &oy);

  // Initial render box.
  // Located at the origin of the viewport.
  // Sized to the box size of the platform view.
  // TODO hidpi?
  struct wlr_box box = {
    .x = ox * instance->output->wlr_output->scale,
    .y = oy * instance->output->wlr_output->scale,
    .width = layer->size.width * instance->output->wlr_output->scale,
    .height = layer->size.height * instance->output->wlr_output->scale,
  };

  wlr_matrix_project_box(matrix, &box, transform, 0,
    instance->output->wlr_output->transform_matrix);

  double opacity = 1.0;

  //wlr_log(WLR_INFO, "num mutators: %ld", layer->platform.mutations_count);
  for (int m = 0; m < layer->platform.mutations_count; m++) {
    FlutterPlatformViewMutation *mutation = &layer->platform.mutations[m];
    switch (mutation->type) {
      case kFlutterPlatformViewMutationTypeOpacity: {
        opacity = mutation->opacity;
        break;
      }
      case kFlutterPlatformViewMutationTypeClipRect: {
        // TODO
        break;
      }
      case kFlutterPlatformViewMutationTypeClipRoundedRect: {
        // TODO
        break;
      }
      case kFlutterPlatformViewMutationTypeTransformation: {
        FlutterTransformation *ft = &mutation->transformation;
        float transformation[9] = {
          ft->scaleX, ft->skewX, ft->transX,
          ft->skewY, ft->scaleY, ft->transY,
          ft->pers0, ft->pers1, ft->pers2
        };
        wlr_matrix_multiply(matrix, transformation, matrix);
        break;
      }
    }
  }

  wlr_render_texture_with_matrix(instance->renderer, texture, matrix, opacity);

  wlr_presentation_surface_sampled_on_output(instance->presentation, view->surface->surface, instance->output->wlr_output);
  wlr_surface_send_frame_done(view->surface->surface, now);
}

static void render_scene_layer_texture(struct fwr_instance *instance, struct fwr_renderer_scene_layer *layer) {
  struct fwr_renderer *renderer = &instance->fwr_renderer;
  struct gl_fns *fns = &renderer->fns;

  // TODO offset to output coordinates?
  // Should not be neccesary since we render the flutter layers directly to the opengl viewport
  // as well.

  fns->glUseProgram(renderer->quad_rgbx_shader.prog);

  fns->glEnableVertexAttribArray(renderer->quad_rgbx_shader.pos_attrib);
  fns->glBindBuffer(GL_ARRAY_BUFFER, renderer->quad_vert_buffer);
  fns->glVertexAttribPointer(renderer->quad_rgbx_shader.pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);

  fns->glEnableVertexAttribArray(renderer->quad_rgbx_shader.tex_attrib);
  fns->glBindBuffer(GL_ARRAY_BUFFER, renderer->tex_coord_buffer);
  fns->glVertexAttribPointer(renderer->quad_rgbx_shader.tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);

  fns->glActiveTexture(GL_TEXTURE0);

  fns->glBindTexture(GL_TEXTURE_2D, layer->texture.texture->texture);
  fns->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  fns->glDisableVertexAttribArray(renderer->quad_rgbx_shader.pos_attrib);
  fns->glDisableVertexAttribArray(renderer->quad_rgbx_shader.tex_attrib);

  fns->glBindTexture(GL_TEXTURE_2D, 0);
  fns->glBindBuffer(GL_ARRAY_BUFFER, 0);

  fns->glUseProgram(0);
}

void fwr_renderer_render_scene(struct fwr_instance *instance) {
  struct fwr_renderer *renderer = &instance->fwr_renderer;

  struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

  pthread_mutex_lock(&renderer->render_mutex);

  if (renderer->current_scene.sync != 0) {
      eglWaitSync(instance->egl->display, renderer->current_scene.sync, 0);
      eglDestroySync(instance->egl->display, renderer->current_scene.sync);
      renderer->current_scene.sync = 0;
  }

  for (int i = 0; i < renderer->current_scene.layers_count; i++) {
    struct fwr_renderer_scene_layer *layer = &renderer->current_scene.layers[i];
    switch (layer->type) {
    case sceneLayerPlatform:
      render_scene_layer_platform(instance, layer, &now);
      break;
    case sceneLayerTexture:
      render_scene_layer_texture(instance, layer);
      break;
    }
  }

  pthread_mutex_unlock(&renderer->render_mutex);
}