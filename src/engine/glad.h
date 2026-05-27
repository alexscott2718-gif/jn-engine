#ifndef GLAD_H
#define GLAD_H

/* Minimal GL 3.3 Core loader using SDL_GL_GetProcAddress.
   Only declares the GL functions actually used by this engine.
   On Emscripten the symbols come directly from <GLES3/gl3.h>. */

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
static inline int glad_load_gl(void) { return 1; }
#else

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef char           GLchar;
typedef ptrdiff_t      GLsizeiptr;
typedef ptrdiff_t      GLintptr;

#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_TRIANGLES                      0x0004
#define GL_LINES                          0x0001
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_TEXTURE_2D                     0x0DE1
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_LINEAR                         0x2601
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_TEXTURE0                       0x84C0
#define GL_CULL_FACE                      0x0B44

/* Function pointer typedefs */
typedef void (*PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (*PFNGLCLEARPROC)(GLbitfield);
typedef void (*PFNGLENABLEPROC)(GLenum);
typedef void (*PFNGLDISABLEPROC)(GLenum);
typedef void (*PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void (*PFNGLBLENDFUNCSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);
typedef void (*PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (*PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (*PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (*PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (*PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (*PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const void*);
typedef void (*PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void (*PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef void (*PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (*PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void (*PFNGLGENERATEMIPMAPPROC)(GLenum);
typedef void (*PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum);
typedef GLenum (*PFNGLGETERRORPROC)(void);
typedef void (*PFNGLREADPIXELSPROC)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
typedef void (*PFNGLDEPTHMASKPROC)(GLboolean);

/* Global function pointer declarations */
extern PFNGLCLEARCOLORPROC              glClearColor;
extern PFNGLCLEARPROC                   glClear;
extern PFNGLENABLEPROC                  glEnable;
extern PFNGLDISABLEPROC                 glDisable;
extern PFNGLBLENDFUNCPROC               glBlendFunc;
extern PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
extern PFNGLVIEWPORTPROC                glViewport;
extern PFNGLGENBUFFERSPROC              glGenBuffers;
extern PFNGLBINDBUFFERPROC              glBindBuffer;
extern PFNGLBUFFERDATAPROC              glBufferData;
extern PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
extern PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC         glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
extern PFNGLCREATESHADERPROC            glCreateShader;
extern PFNGLSHADERSOURCEPROC            glShaderSource;
extern PFNGLCOMPILESHADERPROC           glCompileShader;
extern PFNGLGETSHADERIVPROC             glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC            glDeleteShader;
extern PFNGLCREATEPROGRAMPROC           glCreateProgram;
extern PFNGLATTACHSHADERPROC            glAttachShader;
extern PFNGLLINKPROGRAMPROC             glLinkProgram;
extern PFNGLGETPROGRAMIVPROC            glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC              glUseProgram;
extern PFNGLDELETEPROGRAMPROC           glDeleteProgram;
extern PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
extern PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
extern PFNGLUNIFORM1IPROC               glUniform1i;
extern PFNGLUNIFORM1FPROC               glUniform1f;
extern PFNGLUNIFORM2FPROC               glUniform2f;
extern PFNGLUNIFORM3FPROC               glUniform3f;
extern PFNGLUNIFORM4FPROC               glUniform4f;
extern PFNGLDRAWELEMENTSPROC            glDrawElements;
extern PFNGLDRAWARRAYSPROC              glDrawArrays;
extern PFNGLGENTEXTURESPROC             glGenTextures;
extern PFNGLBINDTEXTUREPROC             glBindTexture;
extern PFNGLTEXIMAGE2DPROC              glTexImage2D;
extern PFNGLTEXPARAMETERIPROC           glTexParameteri;
extern PFNGLGENERATEMIPMAPPROC          glGenerateMipmap;
extern PFNGLDELETETEXTURESPROC          glDeleteTextures;
extern PFNGLACTIVETEXTUREPROC           glActiveTexture;
extern PFNGLGETERRORPROC                glGetError;
extern PFNGLREADPIXELSPROC              glReadPixels;
extern PFNGLDEPTHMASKPROC               glDepthMask;
typedef void (*GLFINISHPROC)(void);
extern GLFINISHPROC                     glFinish;

/* Call after SDL_GL_CreateContext */
int glad_load_gl(void);

#ifdef __cplusplus
}
#endif

#endif /* __EMSCRIPTEN__ */
#endif
