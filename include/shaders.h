#pragma once

#include <GLES2/gl2.h>

struct fwr_instance;

struct quad_rgbx_shader {
    GLuint prog;

    GLint proj;
    GLint tex;
    GLint alpha;
    GLint pos_attrib;
    GLint tex_attrib;
};

struct quad_rgbx_shader make_quad_rgbx_shader(struct fwr_instance *instance);

extern const GLfloat quad_verts[8];