#ifndef WATERCOLORPASS_H
#define WATERCOLORPASS_H

#include "renderpass.h"

namespace effects {

class WatercolorPass : public RenderPass
{
public:
    WatercolorPass();
    ~WatercolorPass();

    // Sets the noise scale factor for the noise image used by the watercolor effect (default it 5).
    void NoiseScaleFactor(double factor);
    void ThresholdCombinerAlpha(double alpha);
    void ThresholdCombinerThreshold(double threshold);
    void FinalBlendAlpha(double alpha);
    // Sets the watercolor texture to be used for the effect.
    void EffectTexture(QImage* texture);
    // Sets the name of this effect - used for storing debugging images.
    void Name(const QString& name);
    // Adds a watercolor effect to the given image.
    virtual QImage Process(const QImage& input_image) override;

private:
    QImage ProcessStep(const QImage& input_image, const QImage& texture, const double& noise_scale_factor);
    double noise_scale_factor_;
    double threshold_combiner_alpha_;
    double threshold_combiner_threshold_;
    double final_blend_alpha_;
    BLURSIZE blur_size_;
    QImage* effect_texture_;
    QString name_;
    QImage* noise_image_;
};

}  // namespace effects

#endif // WATERCOLORPASS_H
