#pragma once

#include "shaders.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <bits/pthreadtypes.h>
#include <stdbool.h>
#include <pthread.h>

#include <flutter_embedder.h>
#include <wayland-util.h>

struct fwr_instance;

struct gl_fns {
  void (*glGenFramebuffers)(GLsizei, GLuint*);
  void (*glBindFramebuffer)(GLenum, GLuint);
  void (*glGenTextures)(GLsizei, GLuint*);
  void (*glBindTexture)(GLenum, GLuint);
  void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
  void (*glTexParameteri)(GLenum, GLenum, GLint);
  void (*glFramebufferTexture)(GLenum, GLenum, GLuint, GLint);
  void (*glDrawBuffers)(GLsizei, const GLenum*);
  GLuint (*glCreateShader)(GLenum);
  void (*glShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
  void (*glCompileShader)(GLuint);
  void (*glGetShaderiv)(GLuint, GLenum, GLint*);
  void (*glDeleteShader)(GLuint);
  GLuint (*glCreateProgram)();
  void (*glAttachShader)(GLuint, GLuint);
  void (*glLinkProgram)(GLuint);
  void (*glDetachShader)(GLuint, GLuint);
  void (*glGetProgramiv)(GLuint, GLenum, GLint*);
  void (*glDeleteProgram)(GLuint);
  GLint (*glGetUniformLocation)(GLuint, const GLchar*);
  GLint (*glGetAttribLocation)(GLuint, const GLchar*);
  void (*glActiveTexture)(GLenum);
  void (*glUseProgram)(GLuint);
  void (*glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
  void (*glUniform1i)(GLint, GLint);
  void (*glUniform1f)(GLint, GLfloat);
  void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
  void (*glEnableVertexAttribArray)(GLuint);
  void (*glDrawArrays)(GLenum, GLint, GLsizei);
  void (*glDisableVertexAttribArray)(GLuint);
  void (*glEnable)(GLenum);
  void (*glDisable)(GLenum);
  void (*glGetTextureImage)(GLuint, GLint, GLenum, GLenum, GLsizei, void*);
  GLenum (*glCheckFramebufferStatus)(GLenum);
  GLenum (*glGetError)();
  void (*glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
  void (*glGenBuffers)(GLsizei, GLuint*);
  void (*glBindBuffer)(GLenum, GLuint);
  void (*glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
  void (*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
  void (*glClear)(GLbitfield);
  void (*glDeleteFramebuffers)(GLsizei, GLuint*);
  void (*glDeleteTextures)(GLsizei, GLuint*);
  void (*glBindSampler)(GLuint, GLuint);
  void (*glPushClientAttrib)(GLbitfield);
  void (*glPopClientAttrib)(GLbitfield);
  void (*glBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);

  void (*glGetBooleanv)(GLenum, GLboolean*);
  void (*glGetIntegerv)(GLenum, GLint*);
};

struct fwr_renderer_fbo {
  GLuint fbo;
  GLuint tex;
  EGLSync sync;
};

struct fwr_renderer_page_texture {
  struct wl_list link;
  struct fwr_renderer_page *page;
  GLuint texture;
  GLuint fbo;
  size_t width;
  size_t height;
};

struct fwr_renderer_page {
  struct fwr_instance *instance;
  struct wl_list unused_textures;
  struct wl_list textures;
};

enum fwr_renderer_scene_layer_type {
  sceneLayerTexture,
  sceneLayerPlatform,
};

struct fwr_renderer_scene_layer_texture {
  struct fwr_renderer_page_texture *texture;
};

struct fwr_renderer_scene_layer_platform {
  int64_t platform_view_id;
  size_t mutations_count;
  FlutterPlatformViewMutation *mutations;
};

struct fwr_renderer_scene_layer {
  FlutterPoint offset;
  FlutterSize size;

  enum fwr_renderer_scene_layer_type type;
  union {
    struct fwr_renderer_scene_layer_texture texture;
    struct fwr_renderer_scene_layer_platform platform;
  };
};

struct fwr_renderer_scene {
  size_t layers_count;
  struct fwr_renderer_scene_layer *layers;
  EGLSync sync;
};

#define FWR_RENDERER_NUM_FBOS 2
#define FWR_RENDERER_NUM_PAGES 2

struct fwr_renderer {
  struct gl_fns fns;

  GLuint tex_coord_buffer;
  GLuint quad_vert_buffer;

  struct quad_rgbx_shader quad_rgbx_shader;

  bool fbo_inited;
  uint8_t current_fbo;
  struct fwr_renderer_fbo fbos[FWR_RENDERER_NUM_FBOS];

  uint8_t current_page;
  struct fwr_renderer_page pages[FWR_RENDERER_NUM_PAGES];
  struct fwr_renderer_scene current_scene;

  int flutter_tex_width;
  int flutter_tex_height;

  EGLContext flutter_egl_context;
  EGLContext flutter_resource_egl_context;

  pthread_mutex_t render_mutex;
};

typedef void (*gl_resolved_fn)();
typedef gl_resolved_fn (*gl_resolve_fn)(const char *name);

void fwr_renderer_init(struct fwr_instance *instance, gl_resolve_fn resolver);
//void fwr_renderer_ensure_fbo(struct fwr_instance *instance, int width, int height);
//void fwr_renderer_render_flutter_buffer(struct fwr_instance *instance);
//GLuint fwr_renderer_get_active_fbo(struct fwr_instance *instance);
//void fwr_renderer_flip_fbo(struct fwr_instance *instance);

void fwr_renderer_render_scene(struct fwr_instance *instance);