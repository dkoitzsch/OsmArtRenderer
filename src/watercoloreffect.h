#ifndef WATERCOLOREFFECT_H
#define WATERCOLOREFFECT_H

#include "canvas.h"
#include "effect.h"
#include "watercolorpass.h"

#include <QHash>
#include <QImage>
#include <QObject>
#include <QDoubleSpinBox>
#include <QWidget>

namespace effects {

class WatercolorEffectConfiguration : public QObject
{
    Q_OBJECT
public:
    explicit WatercolorEffectConfiguration(QString ojects_name, double noise_scale_factor,
                                           double final_blend_alpha, double threshold_combiner_alpha,
                                           double threshold_combiner_threshold, QImage effect_texture);
    virtual ~WatercolorEffectConfiguration();

    double NoiseScaleFactor() { return noise_scale_factor_; }
    void NoiseScaleFactor(double value) { noise_scale_factor_ = value; }
    double ThresholdCombinerAlpha() { return threshold_combiner_alpha_; }
    void ThresholdCombinerAlpha(double alpha) { threshold_combiner_alpha_ = alpha; }
    double ThresholdCombinerThreshold() { return threshold_combiner_threshold_; }
    void ThresholdCombinerThreshold(double threshold) { threshold_combiner_threshold_ = threshold; }
    double FinalBlendAlpha() { return final_blend_alpha_; }
    void FinalBlendAlpha(double alpha) { final_blend_alpha_ = alpha; }
    QImage EffectsTexture() { return effect_texture_; }
    void EffectsTexture(QImage texture) { effect_texture_ = texture; }
    QString ObjectsName() { return ojects_name_; }
    QWidget* Widget() { return widget_; }

signals:
    void Updated(WatercolorEffectConfiguration* config);

private:
    double noise_scale_factor_;
    double threshold_combiner_alpha_;
    double threshold_combiner_threshold_;
    double final_blend_alpha_;
    QImage effect_texture_;
    QString ojects_name_;
    QWidget* widget_;
    QDoubleSpinBox* ui_noise_scale_factor_;
    QDoubleSpinBox* ui_threshold_combiner_threshold_;
    QDoubleSpinBox* ui_threshold_combiner_alpha_;
    QDoubleSpinBox* ui_final_blend_alpha_;
};

class WatercolorEffect : public Effect
{
public:
    explicit WatercolorEffect(QHash<QString, WatercolorEffectConfiguration*> config);
    virtual ~WatercolorEffect();

    virtual QImage Apply(osm::Canvas* canvas, bool offscreen);

private:
    QHash<QString, WatercolorEffectConfiguration*> config_;
};

}  // namespace effects

#endif // WATERCOLOREFFECT_H
