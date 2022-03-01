#include "shaders.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <wlr/util/log.h>

const GLfloat quad_verts[8] = {
	1, -1, // top right
	-1, -1, // top left
	1, 1, // bottom right
	-1, 1, // bottom left
};

static const GLchar quad_vertex_src[] =
"attribute vec2 pos;\n"
"attribute vec2 texcoord;\n"
"varying vec2 v_texcoord;\n"
"\n"
"void main() {\n"
"	gl_Position = vec4(pos, 1.0, 1.0);\n"
"	v_texcoord = texcoord;\n"
"}\n";

static const GLchar tex_fragment_src_rgbx[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"\n"
"void main() {\n"
"  gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0);\n"
"}\n";

GLuint compile_shader(struct fwr_instance *instance, GLuint type, const GLchar *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLchar *data = calloc(1, sizeof(GLchar) * 10000);
        GLsizei len = 0;
        glGetShaderInfoLog(shader, 10000, &len, data);

        printf("Shader compile error (%d): %.*s\n\n", len, len, data);
        printf("%s\n\n", src);

        glDeleteShader(shader);
        shader = 0;
    }

    return shader;
}

GLuint link_program(struct fwr_instance *instance, const GLchar *vert_src, const GLchar *frag_src) {
    GLuint vert = compile_shader(instance, GL_VERTEX_SHADER, vert_src);
    if (!vert) {
        printf("Failed to compile vertex shader!\n");
        return 0;
    }

    GLuint frag = compile_shader(instance, GL_FRAGMENT_SHADER, frag_src);
    if (!frag) {
        printf("Failed to compile fragment shader!\n");
        glDeleteShader(vert);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        glDeleteProgram(prog);
        printf("Failed to link shader!\n");
        return 0;
    }

    return prog;
}

struct quad_rgbx_shader make_quad_rgbx_shader(struct fwr_instance *instance) {
    GLuint prog = link_program(instance, quad_vertex_src, tex_fragment_src_rgbx);

    struct quad_rgbx_shader shader = {};
    shader.prog = prog;
    shader.proj = glGetUniformLocation(prog, "proj");
    shader.tex = glGetUniformLocation(prog, "tex");
    shader.alpha = glGetUniformLocation(prog, "alpha");
    shader.pos_attrib = glGetAttribLocation(prog, "pos");
    shader.tex_attrib = glGetAttribLocation(prog, "texcoord");

    return shader;
}