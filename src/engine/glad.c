#include "glad.h"

#ifndef __EMSCRIPTEN__

#include <SDL.h>
#include <stdio.h>

PFNGLCLEARCOLORPROC              glClearColor;
PFNGLCLEARPROC                   glClear;
PFNGLENABLEPROC                  glEnable;
PFNGLDISABLEPROC                 glDisable;
PFNGLBLENDFUNCPROC               glBlendFunc;
PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
PFNGLVIEWPORTPROC                glViewport;
PFNGLGENBUFFERSPROC              glGenBuffers;
PFNGLBINDBUFFERPROC              glBindBuffer;
PFNGLBUFFERDATAPROC              glBufferData;
PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC         glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
PFNGLCREATESHADERPROC            glCreateShader;
PFNGLSHADERSOURCEPROC            glShaderSource;
PFNGLCOMPILESHADERPROC           glCompileShader;
PFNGLGETSHADERIVPROC             glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
PFNGLDELETESHADERPROC            glDeleteShader;
PFNGLCREATEPROGRAMPROC           glCreateProgram;
PFNGLATTACHSHADERPROC            glAttachShader;
PFNGLLINKPROGRAMPROC             glLinkProgram;
PFNGLGETPROGRAMIVPROC            glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC              glUseProgram;
PFNGLDELETEPROGRAMPROC           glDeleteProgram;
PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
PFNGLUNIFORM1IPROC               glUniform1i;
PFNGLUNIFORM1FPROC               glUniform1f;
PFNGLUNIFORM2FPROC               glUniform2f;
PFNGLUNIFORM3FPROC               glUniform3f;
PFNGLUNIFORM4FPROC               glUniform4f;
PFNGLDRAWELEMENTSPROC            glDrawElements;
PFNGLDRAWARRAYSPROC              glDrawArrays;
PFNGLGENTEXTURESPROC             glGenTextures;
PFNGLBINDTEXTUREPROC             glBindTexture;
PFNGLTEXIMAGE2DPROC              glTexImage2D;
PFNGLTEXPARAMETERIPROC           glTexParameteri;
PFNGLGENERATEMIPMAPPROC          glGenerateMipmap;
PFNGLDELETETEXTURESPROC          glDeleteTextures;
PFNGLACTIVETEXTUREPROC           glActiveTexture;
PFNGLGETERRORPROC                glGetError;
PFNGLREADPIXELSPROC              glReadPixels;
PFNGLDEPTHMASKPROC               glDepthMask;
PFNGLDEPTHFUNCPROC               glDepthFunc;
GLFINISHPROC                     glFinish;
PFNGLUNIFORM4FVPROC              glUniform4fv;
PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus;
PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers;
PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage;
PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers;

#define LOAD(name) \
    name = (typeof(name))SDL_GL_GetProcAddress(#name); \
    if (!name) { fprintf(stderr, "glad: failed to load %s\n", #name); ok = 0; }

int glad_load_gl(void) {
    int ok = 1;
    LOAD(glClearColor)
    LOAD(glClear)
    LOAD(glEnable)
    LOAD(glDisable)
    LOAD(glBlendFunc)
    LOAD(glBlendFuncSeparate)
    LOAD(glViewport)
    LOAD(glGenBuffers)
    LOAD(glBindBuffer)
    LOAD(glBufferData)
    LOAD(glDeleteBuffers)
    LOAD(glGenVertexArrays)
    LOAD(glBindVertexArray)
    LOAD(glDeleteVertexArrays)
    LOAD(glEnableVertexAttribArray)
    LOAD(glVertexAttribPointer)
    LOAD(glCreateShader)
    LOAD(glShaderSource)
    LOAD(glCompileShader)
    LOAD(glGetShaderiv)
    LOAD(glGetShaderInfoLog)
    LOAD(glDeleteShader)
    LOAD(glCreateProgram)
    LOAD(glAttachShader)
    LOAD(glLinkProgram)
    LOAD(glGetProgramiv)
    LOAD(glGetProgramInfoLog)
    LOAD(glUseProgram)
    LOAD(glDeleteProgram)
    LOAD(glGetUniformLocation)
    LOAD(glUniformMatrix4fv)
    LOAD(glUniform1i)
    LOAD(glUniform1f)
    LOAD(glUniform2f)
    LOAD(glUniform3f)
    LOAD(glUniform4f)
    LOAD(glDrawElements)
    LOAD(glDrawArrays)
    LOAD(glGenTextures)
    LOAD(glBindTexture)
    LOAD(glTexImage2D)
    LOAD(glTexParameteri)
    LOAD(glGenerateMipmap)
    LOAD(glDeleteTextures)
    LOAD(glActiveTexture)
    LOAD(glGetError)
    LOAD(glReadPixels)
    LOAD(glDepthMask)
    LOAD(glDepthFunc)
    LOAD(glFinish)
    LOAD(glUniform4fv)
    LOAD(glGenFramebuffers)
    LOAD(glBindFramebuffer)
    LOAD(glFramebufferTexture2D)
    LOAD(glFramebufferRenderbuffer)
    LOAD(glCheckFramebufferStatus)
    LOAD(glDeleteFramebuffers)
    LOAD(glGenRenderbuffers)
    LOAD(glBindRenderbuffer)
    LOAD(glRenderbufferStorage)
    LOAD(glDeleteRenderbuffers)
    return ok;
}

#endif /* !__EMSCRIPTEN__ */
