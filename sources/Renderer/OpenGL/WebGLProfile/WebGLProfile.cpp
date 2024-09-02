/*
 * WebGLProfile.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "../GLProfile.h"
#include "../GLTypes.h"
#include "../Ext/GLExtensions.h"
#include "../../../Core/Assertion.h"
#include <LLGL/RenderSystemFlags.h>
#include <LLGL/Container/DynamicArray.h>
#include <cstring>


namespace LLGL
{

namespace GLProfile
{


int GetRendererID()
{
    return RendererID::WebGL;
}

const char* GetModuleName()
{
    return "WebGL";
}

const char* GetRendererName()
{
    return "WebGL";
}

const char* GetAPIName()
{
    return "WebGL";
}

const char* GetShadingLanguageName()
{
    return "ESSL";
}

GLint GetMaxViewports()
{
    return 1;
}

void DepthRange(GLclamp_t nearVal, GLclamp_t farVal)
{
    glDepthRangef(nearVal, farVal);
}

void ClearDepth(GLclamp_t depth)
{
    glClearDepthf(depth);
}

void GetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void* data)
{
    LLGL_ASSERT_PTR(data);
    glGetBufferSubData(target, offset, size, data); // supported in WebGL 2 but not in GLES 3
}

static GLbitfield ToGLESMapBufferRangeAccess(GLenum access)
{
    switch (access)
    {
        case GL_READ_ONLY:  return GL_MAP_READ_BIT;
        case GL_WRITE_ONLY: return GL_MAP_WRITE_BIT;
        case GL_READ_WRITE: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        default:            return 0;
    }
}

// This emulates the glMapBuffer API from GLES3 for WebGL
static struct MapBufferContext
{
    GLuint              buffer  = 0;
    GLintptr            offset  = 0;
    GLsizeiptr          size    = 0;
    GLbitfield          access  = 0;
    DynamicByteArray    data;
}
g_mapBufferContext;

void* MapBuffer(GLenum target, GLenum access)
{
    /* Translate GL access type to GLES bitfield, determine buffer length, and map entire buffer range */
    GLbitfield flags = ToGLESMapBufferRangeAccess(access);
    GLint length = 0;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &length);
    return MapBufferRange(target, 0, length, flags);
}

void* MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    LLGL_ASSERT(g_mapBufferContext.access == 0, "cannot map multiple GL buffers interleaved in WebGL backend");

    if (access == 0 || !(length > 0))
        return nullptr;

    /* Get currently bound buffer at the specified target */
    GLint bufferBinding = GLTypes::BufferTargetToBindingPname(target);
    GLint buffer = 0;
    glGetIntegerv(bufferBinding, &buffer);

    /* Allocate intermdiate buffer and store in global context */
    g_mapBufferContext.buffer   = static_cast<GLuint>(buffer);
    g_mapBufferContext.offset   = offset;
    g_mapBufferContext.size     = length;
    g_mapBufferContext.access   = access;
    g_mapBufferContext.data.resize(static_cast<std::size_t>(length), UninitializeTag{});

    /* Manually map GL buffer into CPU memory space to emulate glMapBuffer API for WebGL */
    if ((access & GL_MAP_READ_BIT) != 0)
        glGetBufferSubData(target, offset, length, g_mapBufferContext.data.get()); // supported in WebGL 2 but not in GLES 3
    
    return g_mapBufferContext.data.get();
}

void UnmapBuffer(GLenum target)
{
    if (g_mapBufferContext.access != 0)
    {
        /* Write data back to buffer */
        if ((g_mapBufferContext.access & GL_MAP_WRITE_BIT) != 0)
            glBufferSubData(target, g_mapBufferContext.offset, g_mapBufferContext.size, g_mapBufferContext.data.get());
        g_mapBufferContext.access = 0;
    }
}

void DrawBuffer(GLenum buf)
{
    glDrawBuffers(1, &buf);
}

void FramebufferTexture1D(GLenum /*target*/, GLenum /*attachment*/, GLenum /*textarget*/, GLuint /*texture*/, GLint /*level*/)
{
    // dummy
}

void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void FramebufferTexture3D(GLenum target, GLenum attachment, GLenum /*textarget*/, GLuint texture, GLint level, GLint layer)
{
    glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    glFramebufferTextureLayer(target, attachment, texture, level, layer);
}


} // /namespace GLProfile

} // /namespace LLGL



// ================================================================================