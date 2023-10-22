#include "watercolorpass.h"
#include "qnoise.h"

#include <QDebug>
#include <QElapsedTimer>
//#include <QVector2D>
//#include <memory>
//#include <stdexcept>

namespace effects {

WatercolorPass::WatercolorPass() : noise_scale_factor_(0.5), threshold_combiner_alpha_(0.5), threshold_combiner_threshold_(0.7),
    final_blend_alpha_(0.8), effect_texture_(nullptr), noise_image_(nullptr)
{
}

WatercolorPass::~WatercolorPass()
{
    if (noise_image_ != nullptr) {
        delete noise_image_;
    }
}

void WatercolorPass::NoiseScaleFactor(double factor)
{
    noise_scale_factor_ = factor;
}

void WatercolorPass::ThresholdCombinerAlpha(double alpha)
{
    threshold_combiner_alpha_ = alpha;
}

void WatercolorPass::ThresholdCombinerThreshold(double threshold)
{
    threshold_combiner_threshold_ = threshold;
}

void WatercolorPass::FinalBlendAlpha(double alpha)
{
    final_blend_alpha_ = alpha;
}

void WatercolorPass::EffectTexture(QImage* texture)
{
    effect_texture_ = texture;
}

void WatercolorPass::Name(const QString& name)
{
    name_ = name;
}

QImage WatercolorPass::Process(const QImage& input_image)
{
    if (effect_texture_ == nullptr) {
        throw std::logic_error("Effect texture is not set (nullptr).");
    }
    return ProcessStep(input_image, *effect_texture_, noise_scale_factor_);
}

/*
 * TODO: Make sure that the order of the texture per step is correct.
 * 1) Blur
 * 2) Create noise
 * 3) Combine 1) and 2) with threshold_combiner.frag
 * 4) Combine 3) and texture with color_combiner.frag
 * 5) Invert 3) with invert.frag
 * 6) Blur 5) with blur.frag and size = 13
 * 7) Mask 5) and 6) with mask.frag (to just get the blurred outlines)
 * 8) Blend 4) and 7) with alpha = 0.8f to get the final image
 * */
QImage WatercolorPass::ProcessStep(const QImage& input_image, const QImage& texture, const double& noise_scale_factor)
{
    int width = input_image.width();
    int height = input_image.height();

    if (noise_image_ == nullptr) {
        noise_image_ = new QImage(QNoise::create_noise_image(width, height, noise_scale_factor));
#ifdef QT_DEBUG
        noise_image_->save("noise.png", "PNG", 100);
        QElapsedTimer timer;
        timer.start();
#endif
    }

    // 1) Blur
    QImage processed_img = Blur(input_image, QVector2D(1.0, 0.0), BLURSIZE_13);
    processed_img = Blur(processed_img, QVector2D(0.0, 1.0), BLURSIZE_13);
#ifdef QT_DEBUG
    processed_img.save(name_ + "_1_blur.png", "PNG", 100);
    qint64 t = timer.elapsed();
    qDebug() << "------Blur (2x) pass..." << t << "\u0394ms\n";
#endif
/*
    // 2) Create noise
    QImage noise_img = QNoise::create_noise_image(width, height, noise_scale_factor);
#ifdef QT_DEBUG
    noise_img.save(name_ + "_2_noise.png", "PNG", 100);
#endif
    qDebug() << "------Noise pass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
*/
    // 3) Combine 1) and 2) with threshold_combiner.frag
    QImage noised_processed_img = PostProcess({processed_img, *noise_image_},
                                               width, height,
                                               ":/shaders/default.vert", ":/shaders/threshold_combiner.frag",
                                              [this](QOpenGLShaderProgram* program) {
            program->setUniformValue("alpha", (GLfloat) this->threshold_combiner_alpha_);
            program->setUniformValue("threshold", (GLfloat) this->threshold_combiner_threshold_);
    });
#ifdef QT_DEBUG
    noised_processed_img.save(name_ + "_3_noised_processed.png", "PNG", 100);
    qDebug() << "------Threshold Combiner pass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
#endif

    // 4) Combine 3) and texture with color_combiner.frag
    QImage textured_processed_img = PostProcess({noised_processed_img, texture},
                                                 width, height,
                                                 ":/shaders/default.vert", ":/shaders/color_combiner.frag");
#ifdef QT_DEBUG
    textured_processed_img.save(name_ + "_4_textured_processed.png", "PNG", 100);
    qDebug() << "------Color Combiner pass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
#endif

    // 5) Invert 3) with invert.frag
    QImage inverted_noised_processed_img = PostProcess({noised_processed_img},
                                                        width, height,
                                                        ":/shaders/default.vert", ":/shaders/invert.frag");
#ifdef QT_DEBUG
    inverted_noised_processed_img.save(name_ + "_5_inverted_noised_processed.png", "PNG", 100);
    qDebug() << "------Invertpass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
#endif

    // 6) Blur 5) with blur.frag and size = 13
    QImage blurred_inverted_noised_processed_img = Blur(inverted_noised_processed_img, QVector2D(1.0, 0.0), BLURSIZE_13);
    blurred_inverted_noised_processed_img = Blur(blurred_inverted_noised_processed_img, QVector2D(0.0, 1.0), BLURSIZE_13);
#ifdef QT_DEBUG
    blurred_inverted_noised_processed_img.save(name_ + "_6_blurred_inverted_noised_processed.png", "PNG", 100);
    qDebug() << "------Blur (2x) pass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
#endif

    // 7) Mask 5) and 6) with mask.frag (to just get the blurred outlines)
    QImage masked = PostProcess({blurred_inverted_noised_processed_img, inverted_noised_processed_img},
                                 width, height,
                                 ":/shaders/default.vert", ":/shaders/mask.frag");
#ifdef QT_DEBUG
    masked.save(name_ + "_7_masked.png", "PNG", 100);
    qDebug() << "------Mask pass..." << (timer.elapsed() - t) << "\u0394ms\n";
    t = timer.elapsed();
#endif

    // 8) Blend 4) and 7) with alpha = 0.8f to get the final image
    //QImage final = Blend(textured_processed_img, masked, width, height, final_blend_alpha_);
    QImage final = Blend(masked, textured_processed_img, width, height, final_blend_alpha_);
#ifdef QT_DEBUG
    final.save(name_ + "_8_final.png", "PNG", 100);
    qDebug() << "------Blend pass..." << (timer.elapsed() - t) << "\u0394ms\n";
#endif
    return final;
}

}  // namespace effects
