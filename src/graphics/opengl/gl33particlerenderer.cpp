#include "graphics/opengl/gl33particlerenderer.h"

#include "graphics/opengl/gl33device.h"
#include "graphics/opengl/glutil.h"

#include "graphics/core/vertex.h"

#include "common/logger.h"

#include <GL/glew.h>

#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace Gfx;

CGL33ParticleRenderer::CGL33ParticleRenderer(CGL33Device* device)
    : m_device(device)
{
    GetLogger()->Info("Creating CGL33ParticleRenderer\n");

    std::string preamble = LoadSource("shaders/gl33/preamble.glsl");
    std::string shadowSource = LoadSource("shaders/gl33/shadow.glsl");
    std::string vsSource = LoadSource("shaders/gl33/particle_vs.glsl");
    std::string fsSource = LoadSource("shaders/gl33/particle_fs.glsl");

    GLint vsShader = CreateShader(GL_VERTEX_SHADER, { preamble, shadowSource, vsSource });
    if (vsShader == 0)
    {
        GetLogger()->Error("Cound not create vertex shader from file 'particle_vs.glsl'\n");
        return;
    }

    GLint fsShader = CreateShader(GL_FRAGMENT_SHADER, { preamble, shadowSource, fsSource });
    if (fsShader == 0)
    {
        GetLogger()->Error("Cound not create fragment shader from file 'particle_fs.glsl'\n");
        return;
    }

    m_program = LinkProgram({ vsShader, fsShader });

    glDeleteShader(vsShader);
    glDeleteShader(fsShader);

    glUseProgram(m_program);

    // Setup uniforms
    glm::mat4 identity(1.0f);

    m_projectionMatrix = glGetUniformLocation(m_program, "uni_ProjectionMatrix");
    m_viewMatrix = glGetUniformLocation(m_program, "uni_ViewMatrix");
    m_modelMatrix = glGetUniformLocation(m_program, "uni_ModelMatrix");
    m_fogRange = glGetUniformLocation(m_program, "uni_FogRange");
    m_fogColor = glGetUniformLocation(m_program, "uni_FogColor");
    m_color = glGetUniformLocation(m_program, "uni_Color");
    m_alphaScissor = glGetUniformLocation(m_program, "uni_AlphaScissor");

    // Set texture units
    auto texture = glGetUniformLocation(m_program, "uni_Texture");
    glUniform1i(texture, 10);

    glUniform1f(m_alphaScissor, 0.5f);

    // White texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);

    glUseProgram(0);

    // Generic buffer
    glGenBuffers(1, &m_bufferVBO);
    glBindBuffer(GL_COPY_WRITE_BUFFER, m_bufferVBO);
    glBufferData(GL_COPY_WRITE_BUFFER, m_bufferCapacity, nullptr, GL_STREAM_DRAW);

    glGenVertexArrays(1, &m_bufferVAO);
    glBindVertexArray(m_bufferVAO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    GetLogger()->Info("CGL33ParticleRenderer created successfully\n");
}

CGL33ParticleRenderer::~CGL33ParticleRenderer()
{
    glDeleteProgram(m_program);
    glDeleteTextures(1, &m_whiteTexture);
}

void CGL33ParticleRenderer::Begin()
{
    glUseProgram(m_program);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);

    m_texture = 0;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUniform4f(m_color, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(m_bufferVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferVBO);
}

void CGL33ParticleRenderer::End()
{
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_texture = 0;

    glDepthMask(GL_TRUE);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CGL33ParticleRenderer::SetProjectionMatrix(const glm::mat4& matrix)
{
    glUniformMatrix4fv(m_projectionMatrix, 1, GL_FALSE, value_ptr(matrix));
}

void CGL33ParticleRenderer::SetViewMatrix(const glm::mat4& matrix)
{
    glm::mat4 scale(1.0f);
    scale[2][2] = -1.0f;

    auto viewMatrix = scale * matrix;

    glUniformMatrix4fv(m_viewMatrix, 1, GL_FALSE, value_ptr(viewMatrix));
}

void CGL33ParticleRenderer::SetModelMatrix(const glm::mat4& matrix)
{
    glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, value_ptr(matrix));
}

void CGL33ParticleRenderer::SetColor(const glm::vec4& color)
{
    glUniform4f(m_color, color.r, color.g, color.b, color.a);
}

void CGL33ParticleRenderer::SetTexture(const Texture& texture)
{
    if (m_texture == texture.id) return;

    m_texture = texture.id;

    glActiveTexture(GL_TEXTURE10);

    if (texture.id == 0)
        glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    else
        glBindTexture(GL_TEXTURE_2D, texture.id);
}


void CGL33ParticleRenderer::SetTransparency(TransparencyMode mode)
{
    switch (mode)
    {
    case TransparencyMode::NONE:
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        break;
    case TransparencyMode::ALPHA:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        glDepthMask(GL_TRUE);
        break;
    case TransparencyMode::BLACK:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        glBlendEquation(GL_FUNC_ADD);
        glDepthMask(GL_FALSE);
        break;
    case TransparencyMode::WHITE:
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        glBlendEquation(GL_FUNC_ADD);
        glDepthMask(GL_FALSE);
        break;
    }
}

void CGL33ParticleRenderer::DrawParticle(PrimitiveType type, int count, const Vertex3D* vertices)
{
    size_t size = count * sizeof(Vertex3D);

    if (m_bufferCapacity < size)
        m_bufferCapacity = size;

    // Send new vertices to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferVBO);
    glBufferData(GL_ARRAY_BUFFER, m_bufferCapacity, nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
        reinterpret_cast<void*>(offsetof(Vertex3D, position)));

    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex3D),
        reinterpret_cast<void*>(offsetof(Vertex3D, color)));

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
        reinterpret_cast<void*>(offsetof(Vertex3D, uv)));

    glDrawArrays(TranslateGfxPrimitive(type), 0, count);
}
