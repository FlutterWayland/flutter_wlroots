typedef int GLenum;
typedef int GLfloat;
typedef int GLfixed;
typedef int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef int GLsizeiptr;
typedef int GLint64;
typedef int GLint64EXT;
typedef int GLuint64;
typedef int GLuint64EXT;
typedef int GLintptr;
typedef int GLchar;
typedef int GLcharARB;
typedef int GLhandleARB;
typedef int GLbyte;
typedef int GLdouble;
typedef int GLshort;
typedef int GLbitfield;
typedef int GLintptrARB;
typedef int GLclampf;
typedef int GLsync;
typedef int GLhalfNV;
typedef int GLubyte;
typedef int GLushort;
typedef int GLDEBUGPROC;
typedef int GLDEBUGPROCAMD;
typedef int GLDEBUGPROCARB;
typedef int GLDEBUGPROCKHR;
typedef int GLVULKANPROCNV;
typedef int GLclampd;
typedef int GLeglImageOES;
typedef int GLboolean;
typedef int GLclampx;
typedef int GLvdpauSurfaceNV;
typedef int GLeglClientBufferEXT;
typedef int GLsizeiptrARB;

struct _cl_context {};
struct _cl_event {};

#define NULL 0

void* eglGetProcAddr(const char* name);

#define __glintercept_log(...)
