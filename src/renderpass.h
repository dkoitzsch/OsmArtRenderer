#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <QImage>
#include <QList>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QString>

#include <functional>

namespace effects {

class RenderPass
{
public:
    RenderPass();
    virtual ~RenderPass();

    //bool Initialize(int width, int height, const QString& vertex_shader, const QString& fragment_shader, const QList<QImage>& textures);
    void Destroy();

    virtual QImage Process(const QImage& input_image) = 0;
    // Textures must be order in a way that the texture which has to be on top is the first in the list.
    QImage Combine(QList<QImage> textures, const QColor& mask_color);

    enum BLURSIZE {
        BLURSIZE_9, BLURSIZE_13
    };

protected:
    QImage Blur(const QImage& texture, const QVector2D& direction, const BLURSIZE blur_size = BLURSIZE_9);
    QImage Erode(const QImage& texture, int erosion_size = 5);
    QImage Dilate(const QImage& texture, int dilation_size = 5);
    QImage Blend(const QImage& texture_0, const QImage& texture_1,
                 const int& width, const int& height, const float& alpha);
    // Overlays two textures in the way that the color from texture 1 is only taken,
    // if the color of texture 0 has the specified "mask_color".
    // In other words: mask_color says "please take the color of the other texture" ;-)
    QImage MaskedOverlay(const QImage& texture_0, const QImage& texture_1,
                          const int& width, const int& height, const QColor& mask_color);
    QImage PostProcess(const QList<QImage>& textures,
                        const int& width, const int& height,
                        const QString& vertex_shader,
                        const QString& fragment_shader,
                        std::function<void(QOpenGLShaderProgram*)> set_params = nullptr);

    QOpenGLShaderProgram* program_;
    static QOpenGLContext* context_;
    static QOpenGLFramebufferObject* fbo_;
    static QOpenGLFramebufferObject* down_fbo_;
    static QOffscreenSurface* surface_;
    QList<QOpenGLTexture*> gl_textures_;
    static QOpenGLBuffer* vertex_buf_;
    static QOpenGLBuffer* index_buf_;
    int width_;
    int height_;
    QString vertex_shader_;
    QString fragment_shader_;
    static bool initialized_;

private:
    bool SetupGl(const int& width,
                  const int& height,
                  const QString& vertex_shader,
                  const QString& fragment_shader,
                  QOpenGLContext*& out_context,
                  QOffscreenSurface*& surface,
                  QOpenGLFramebufferObject*& out_fbo,
                  QOpenGLShaderProgram*& out_program);
    bool SetupTextures(const QList<QImage>& texture_images, QList<QOpenGLTexture*>& out_gl_textures);
    bool FinalizeSetup(QOpenGLContext* context,
                        QOpenGLShaderProgram* program,
                        const int& texture_count,
                        QOpenGLBuffer*& out_vertex_buf,
                        QOpenGLBuffer*& out_index_buf);
    void DrawFullscreenQuad(QOpenGLContext* context);
};

}

#endif // RENDERPASS_H
