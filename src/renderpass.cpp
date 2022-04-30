#include "renderpass.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QVector3D>

namespace effects {

struct VertexData
{
    QVector2D position;
    QVector2D texCoord;
};

//QOpenGLShaderProgram* RenderPass::program_ = nullptr;
QOpenGLContext* RenderPass::context_ = nullptr;
QOpenGLFramebufferObject* RenderPass::fbo_ = nullptr;
QOpenGLFramebufferObject* RenderPass::down_fbo_ = nullptr;
QOffscreenSurface* RenderPass::surface_ = nullptr;
QOpenGLBuffer* RenderPass::vertex_buf_ = nullptr;
QOpenGLBuffer* RenderPass::index_buf_ = nullptr;
bool RenderPass::initialized_ = false;

RenderPass::RenderPass() : program_(nullptr) //, context_(nullptr), fbo_(nullptr), surface_(nullptr), initialized_(false)
{
}

RenderPass::~RenderPass()
{
    // TODO: Destroy GL, FBO, context, etc.
}

/*
bool RenderPass::Initialize(int width, int height, const
                            QString& vertex_shader, const QString& fragment_shader,
                            const QList<QImage>& textures)
{
    initialized_ = false;

    width_ = width;
    height_ = height;
    vertex_shader_ = vertex_shader;
    fragment_shader_ = fragment_shader;

    if (!SetupGl(width, height,
        vertex_shader_.toLatin1().data(),
        fragment_shader_.toLatin1().data(),
                 context_, surface_, fbo_, program_)) {
        return false;
    }

    if (!SetupTextures(textures, gl_textures_)) {
        return false;
    }

    if (!FinalizeSetup(context_, program_, textures.length(), vertex_buf_, index_buf_)) {
        return false;
    }

    initialized_ = true;
    return true;
}
*/

void RenderPass::Destroy()
{
    for (int i = 0; i < gl_textures_.length(); ++i) {
        gl_textures_[i]->release();
        delete gl_textures_[i];
        gl_textures_[i] = nullptr;
    }

    fbo_->release();
    down_fbo_->release();
    vertex_buf_->release();
    index_buf_->release();
    vertex_buf_->destroy();
    index_buf_->destroy();
    //program_->release();
    delete fbo_;
    delete down_fbo_;
    delete vertex_buf_;
    delete index_buf_;
    delete surface_;
    delete context_;

    fbo_ = nullptr;
    down_fbo_ = nullptr;
    vertex_buf_ = nullptr;
    index_buf_ = nullptr;
    surface_ = nullptr;
    context_ = nullptr;

    if (program_) {
        program_->release();
        delete program_;
        program_ = nullptr;
    }

    initialized_ = false;
}

QImage RenderPass::Combine(QList<QImage> textures, const QColor& mask_color)
{
    if (textures.length() < 2) {
        throw std::logic_error("At least two textures need to be provided for combination.");
    }
    QImage combined = textures.at(0);
    for (int i = 1; i < textures.length(); ++i) {
        combined = MaskedOverlay(combined, textures.at(i),
                                  combined.width(), combined.height(),
                                  mask_color);
    }

    return combined;
}

QImage RenderPass::Blur(const QImage& texture, const QVector2D& direction, const BLURSIZE blur_size)
{
    int size = 0;
    if (blur_size == BLURSIZE_9) {
        size = 9;
    } else if (blur_size == BLURSIZE_13) {
        size = 13;
    }
    return PostProcess({texture},
                        texture.width(), texture.height(),
                        ":/shaders/default.vert",
                        ":/shaders/gaussian_blur.frag",
                        [texture, direction, size](QOpenGLShaderProgram* program) {
        program->setUniformValue("blur_size", size);
        program->setUniformValue("texture_width", static_cast<float>(texture.width()));
        program->setUniformValue("texture_height", static_cast<float>(texture.height()));
        program->setUniformValue("direction", direction);
    });
}

QImage RenderPass::Erode(const QImage& texture, int erosion_size)
{
    return PostProcess({texture},
                        texture.width(), texture.height(),
                        ":/shaders/default.vert",
                        ":/shaders/erosion.frag",
                        [texture, erosion_size](QOpenGLShaderProgram* program) {
        program->setUniformValue("texture_width", static_cast<float>(texture.width()));
        program->setUniformValue("texture_height", static_cast<float>(texture.height()));
        program->setUniformValue("erosion_size", erosion_size);
    });
}

QImage RenderPass::Dilate(const QImage& texture, int dilation_size)
{
    return PostProcess({texture},
                        texture.width(), texture.height(),
                        ":/shaders/default.vert",
                        ":/shaders/dilation.frag",
                        [texture, dilation_size](QOpenGLShaderProgram* program) {
        program->setUniformValue("texture_width", static_cast<float>(texture.width()));
        program->setUniformValue("texture_height", static_cast<float>(texture.height()));
        program->setUniformValue("dilation_size", dilation_size);
    });
}

QImage RenderPass::Blend(const QImage& texture_0, const QImage& texture_1,
                          const int& width, const int& height, const float& alpha)
{
    return PostProcess({texture_0, texture_1},
                        width, height,
                        ":/shaders/default.vert",
                        ":/shaders/blend.frag",
                        [alpha](QOpenGLShaderProgram* program) {
        program->setUniformValue("alpha", alpha);
    });
}

QImage RenderPass::MaskedOverlay(const QImage& texture_0, const QImage& texture_1,
                                   const int& width, const int& height, const QColor& mask_color)
{
    return PostProcess({texture_0, texture_1},
                        width, height,
                        ":/shaders/default.vert",
                        ":/shaders/masked_overlay.frag",
                        [mask_color](QOpenGLShaderProgram* program) {
        program->setUniformValue(
                    "mask_color",
                    QVector3D(mask_color.red(), mask_color.green(), mask_color.blue())
                );
    });
}

QImage RenderPass::PostProcess(const QList<QImage>& textures,
                    const int& width, const int& height,
                    const QString& vertex_shader,
                    const QString& fragment_shader,
                    std::function<void(QOpenGLShaderProgram*)> set_params)
{
    //QElapsedTimer timer;
    //timer.start();

    //QOpenGLShaderProgram* program = nullptr;
    QOpenGLContext* context = nullptr;
    QOpenGLFramebufferObject* fbo = nullptr;
    QOffscreenSurface* surface = nullptr;

    //if (program_) {
    //    program_->release();
    //}

    if (!SetupGl(width, height,
                 vertex_shader.toLatin1().data(),
                 fragment_shader.toLatin1().data(),
                 context, surface, fbo,
                 program_)) {
        return {};
    }
    //quint64 t = timer.elapsed();
    //qDebug() << "---------Setting up GL:" << t << "ms";

    QList<QOpenGLTexture*> gl_textures;
    if (!SetupTextures(textures, gl_textures)) {
        return {};
    }
    //qDebug() << "---------Setting up texture:" << (timer.elapsed() - t) << "ms";
    //t = timer.elapsed();

    //QOpenGLBuffer* vertex_buf;
    //QOpenGLBuffer* index_buf;
    if (!FinalizeSetup(context_, program_, textures.length(), vertex_buf_, index_buf_)) {
        return {};
    }
    //qDebug() << "---------Finalizing GL setup:" << (timer.elapsed() - t) << "ms";
    //t = timer.elapsed();

    if (set_params != nullptr && program_) {
        set_params(program_);
    }

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glDisable(GL_DEPTH_TEST);

    fbo_->bind();

    DrawFullscreenQuad(context_);
    //qDebug() << "---------Drawing fullscreen quad:" << (timer.elapsed() - t) << "ms";
    //t = timer.elapsed();

    QOpenGLFramebufferObject::blitFramebuffer(
        down_fbo_, fbo_,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    down_fbo_->bindDefault();

    //QImage result = fbo_->toImage(false);
    QImage result = down_fbo_->toImage(false);

    //qDebug() << "---------FBO -> image:" << (timer.elapsed() - t) << "ms";
    //t = timer.elapsed();

    program_->release();
    program_->removeAllShaders();
    delete program_;
    program_ = nullptr;

    /*
    for (int i = 0; i < gl_textures.length(); ++i) {
        gl_textures[i]->release();
        delete gl_textures[i];
        gl_textures[i] = nullptr;
    }

    fbo->release();
    vertex_buf->release();
    index_buf->release();
    vertex_buf->destroy();
    index_buf->destroy();
    program_->release();
    delete fbo;
    delete vertex_buf;
    delete index_buf;
    delete surface;
    delete context;
    //delete program; // Results in a SEGFAULT !?!?

    qDebug() << "---------Clean up:" << (timer.elapsed() - t) << "ms";
    t = timer.elapsed();
    */

    return result;
}

// The shaders are embedded in a resource file (i.e. ":/shaders/default.vert").
bool RenderPass::SetupGl(const int& width,
              const int& height,
              const QString& vertex_shader,
              const QString& fragment_shader,
              QOpenGLContext*& out_context,
              QOffscreenSurface*& surface,
              QOpenGLFramebufferObject*& out_fbo,
              QOpenGLShaderProgram*& out_program)
{
    if (!context_) {
        context_ = new QOpenGLContext;
        if(!context_->create())
        {
            qDebug() << "Can't create GL context.";
            return false;
        }
    }

    if (!surface_) {
        surface_ = new QOffscreenSurface;
        surface_->setFormat(context_->format());
        surface_->create();
        if(!surface_->isValid())
        {
            qDebug() << "Surface not valid.";
            return false;
        }

    }

    if(!context_->makeCurrent(surface_))
    {
        qDebug() << "Can't make context current.";
        return false;
    }
    // Before calling functions(), the context need to be current.
    context_->functions()->glViewport(0, 0, width, height);

    if (!fbo_) {
        //QOpenGLFramebufferObjectFormat format;
        //format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        QOpenGLFramebufferObjectFormat muliSampleFormat;
       muliSampleFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
       muliSampleFormat.setMipmap(true);
       muliSampleFormat.setSamples(4);
       muliSampleFormat.setTextureTarget(GL_TEXTURE_2D);
       muliSampleFormat.setInternalTextureFormat(GL_RGBA32F_ARB); //(GL_BGRA_EXT);
        //format.setSamples(8);
        fbo_ = new QOpenGLFramebufferObject(width, height, muliSampleFormat);

        QOpenGLFramebufferObjectFormat downSampledFormat;
        downSampledFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        downSampledFormat.setMipmap(true);
        downSampledFormat.setTextureTarget(GL_TEXTURE_2D);
        downSampledFormat.setInternalTextureFormat(GL_RGBA32F_ARB);
        down_fbo_ = new QOpenGLFramebufferObject(width, height, downSampledFormat);
    }

    if (!program_) {
        program_ = new QOpenGLShaderProgram(context_);
    } else {
        program_->removeAllShaders();
    }
    if (!program_->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_shader))
    {
        qDebug() << "Can't add vertex shader.";
        return false;
    }
    if (!program_->addShaderFromSourceFile(QOpenGLShader::Fragment, fragment_shader))
    {
        qDebug() << "Can't add fragment shader.";
        return false;
    }
    if (!program_->link())
    {
        qDebug() << "Can't link program.";
        return false;
    }
    if (!program_->bind())
    {
        qDebug() << "Can't bind program.";
        return false;
    }

    return true;

    /*
    out_context = new QOpenGLContext;
    if(!out_context->create())
    {
        qDebug() << "Can't create GL context.";
        return false;
    }

    surface = new QOffscreenSurface;
    surface->setFormat(out_context->format());
    surface->create();
    if(!surface->isValid())
    {
        qDebug() << "Surface not valid.";
        return false;
    }

    if(!out_context->makeCurrent(surface))
    {
        qDebug() << "Can't make context current.";
        return false;
    }

    out_fbo = new QOpenGLFramebufferObject(width, height);

    out_context->functions()->glViewport(0, 0, width, height);
    out_program = new QOpenGLShaderProgram(out_context);
    if (!out_program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_shader))
    {
        qDebug() << "Can't add vertex shader.";
        return false;
    }
    if (!out_program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragment_shader))
    {
        qDebug() << "Can't add fragment shader.";
        return false;
    }
    if (!out_program->link())
    {
        qDebug() << "Can't link program.";
        return false;
    }
    if (!out_program->bind())
    {
        qDebug() << "Can't bind program.";
        return false;
    }

    return true;
    */
}

bool RenderPass::SetupTextures(const QList<QImage>& texture_images, QList<QOpenGLTexture*>& out_gl_textures)
{
    auto it = texture_images.begin();
    int text_id = 0;
    while (it != texture_images.end()) {
        QOpenGLTexture* texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        texture->setData(*it);

        texture->bind(text_id++);
        if(!texture->isBound())
        {
            qDebug() << "Texture not bound.";
            return false;
        }

        out_gl_textures.append(texture);
        ++it;
    }

    return true;
}

bool RenderPass::FinalizeSetup(QOpenGLContext* /*context*/,
                    QOpenGLShaderProgram* program,
                    const int& texture_count,
                    QOpenGLBuffer*& out_vertex_buf,
                    QOpenGLBuffer*& out_index_buf)
{
    if (!vertex_buf_) {
        VertexData vertices[] =
        {
            {{ -1.0f, +1.0f }, { 0.0f, 1.0f }}, // top-left
            {{ +1.0f, +1.0f }, { 1.0f, 1.0f }}, // top-right
            {{ -1.0f, -1.0f }, { 0.0f, 0.0f }}, // bottom-left
            {{ +1.0f, -1.0f }, { 1.0f, 0.0f }}  // bottom-right
        };

        vertex_buf_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if(!vertex_buf_->create())
        {
            qDebug() << "Can't create vertex buffer.";
            return false;
        }
        if(!vertex_buf_->bind())
        {
            qDebug() << "Can't bind vertex buffer.";
            return false;
        }
        vertex_buf_->allocate(vertices, 4 * sizeof(VertexData));
    }

    if (!index_buf_) {
        GLuint indices[] =
        {
            0, 1, 2, 3
        };

        index_buf_ = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        if(!index_buf_->create())
        {
            qDebug() << "Can't create index buffer.";
            return false;
        }
        if(!index_buf_->bind())
        {
            qDebug() << "Can't bind index buffer.";
            return false;
        }
        index_buf_->allocate(indices, 4 * sizeof(GLuint));
    }

    int offset = 0;
    program_->enableAttributeArray("in_position");
    program_->setAttributeBuffer("in_position", GL_FLOAT, offset, 2, sizeof(VertexData));
    offset += sizeof(QVector2D);
    program_->enableAttributeArray("in_tex_coord");

    program_->setAttributeBuffer("in_tex_coord", GL_FLOAT, offset, 2, sizeof(VertexData));
    for (int i = 0; i < texture_count; ++i) {
        QString texture_name = "texture_" + QString::number(i);
        program_->setUniformValue(texture_name.toLatin1().data(), i);
    }

    return true;

    /*
    out_vertex_buf = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    out_index_buf = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);

    if(!out_vertex_buf->create())
    {
        qDebug() << "Can't create vertex buffer.";
        return false;
    }

    if(!out_index_buf->create())
    {
        qDebug() << "Can't create index buffer.";
        return false;
    }

    if(!out_vertex_buf->bind())
    {
        qDebug() << "Can't bind vertex buffer.";
        return false;
    }
    out_vertex_buf->allocate(vertices, 4 * sizeof(VertexData));

    if(!out_index_buf->bind())
    {
        qDebug() << "Can't bind index buffer.";
        return false;
    }
    out_index_buf->allocate(indices, 4 * sizeof(GLuint));

    int offset = 0;
    program->enableAttributeArray("in_position");
    program->setAttributeBuffer("in_position", GL_FLOAT, offset, 2, sizeof(VertexData));
    offset += sizeof(QVector2D);
    program->enableAttributeArray("in_tex_coord");
    program->setAttributeBuffer("in_tex_coord", GL_FLOAT, offset, 2, sizeof(VertexData));
    for (int i = 0; i < texture_count; ++i) {
        QString texture_name = "texture_" + QString::number(i);
        program->setUniformValue(texture_name.toLatin1().data(), i);
    }

    return true;
    */
}

void RenderPass::DrawFullscreenQuad(QOpenGLContext* context)
{
    context->functions()->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, Q_NULLPTR);
}

}  // namespace effects
