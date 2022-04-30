#include "watercoloreffect.h"

#include "objects.h"
#include "objectsconfiguration.h"
#include "objectsrepository.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFormLayout>
#include <QObject>

namespace effects {

WatercolorEffectConfiguration::WatercolorEffectConfiguration(QString ojects_name, double noise_scale_factor,
                                                             double final_blend_alpha, double threshold_combiner_alpha,
                                                             double threshold_combiner_threshold, QImage effect_texture)
    : noise_scale_factor_(noise_scale_factor), threshold_combiner_alpha_(threshold_combiner_alpha),
      threshold_combiner_threshold_(threshold_combiner_threshold), final_blend_alpha_(final_blend_alpha),
      effect_texture_(effect_texture), ojects_name_(ojects_name) {
    widget_ = new QWidget();
    QFormLayout* layout = new QFormLayout(widget_);
    ui_noise_scale_factor_ = new QDoubleSpinBox;
    ui_noise_scale_factor_->setDecimals(2);
    ui_noise_scale_factor_->setMaximum(300);
    ui_noise_scale_factor_->setValue(noise_scale_factor);
    ui_final_blend_alpha_ = new QDoubleSpinBox;
    ui_final_blend_alpha_->setDecimals(2);
    ui_final_blend_alpha_->setValue(final_blend_alpha);
    ui_threshold_combiner_alpha_ = new QDoubleSpinBox;
    ui_threshold_combiner_alpha_->setDecimals(2);
    ui_threshold_combiner_alpha_->setValue(threshold_combiner_alpha);
    ui_threshold_combiner_threshold_ = new QDoubleSpinBox;
    ui_threshold_combiner_threshold_->setDecimals(2);
    ui_threshold_combiner_threshold_->setValue(threshold_combiner_threshold);
    layout->addRow("Noise Scale", ui_noise_scale_factor_);
    layout->addRow("Final Blend Alpha", ui_final_blend_alpha_);
    layout->addRow("Threshold Combiner Alpha", ui_threshold_combiner_alpha_);
    layout->addRow("Threshold Combiner Threshold", ui_threshold_combiner_threshold_);
    widget_->setLayout(layout);

    QObject::connect<void(QDoubleSpinBox::*)(double)>(ui_noise_scale_factor_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        this->noise_scale_factor_ = value;
        emit Updated(this);
    });
    QObject::connect<void(QDoubleSpinBox::*)(double)>(ui_final_blend_alpha_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        this->final_blend_alpha_ = value;
        emit Updated(this);
    });
    QObject::connect<void(QDoubleSpinBox::*)(double)>(ui_threshold_combiner_alpha_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        this->threshold_combiner_alpha_ = value;
        emit Updated(this);
    });
    QObject::connect<void(QDoubleSpinBox::*)(double)>(ui_threshold_combiner_threshold_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        this->threshold_combiner_threshold_ = value;
        emit Updated(this);
    });
}

WatercolorEffectConfiguration::~WatercolorEffectConfiguration()
{
    //delete widget_;
    //delete ui_noise_scale_factor_;
}

WatercolorEffect::WatercolorEffect(QHash<QString, WatercolorEffectConfiguration*> config) : config_(config)
{
}

WatercolorEffect::~WatercolorEffect()
{
    /*for (auto it = config_.begin(); it != config_.end(); ++it) {
        delete it.value();
    }
    config_.clear();*/
}

QImage WatercolorEffect::Apply(osm::Canvas* canvas, bool offscreen)
{
    QImage source_image;
    QHash<QString, QImage> processed_images;
    QList<QImage> combination_order;

    // Store the previous "enabled" state of each objects.
    QHash<QString, bool> enabled_objects;
    for (auto it = canvas->ObjectsRepository()->OrderedObjectsNames()->begin(); it != canvas->ObjectsRepository()->OrderedObjectsNames()->end(); ++it) {
        auto config = canvas->ObjectsRepository()->ObjectsConfiguration(*it);
        if (config != nullptr) {
            enabled_objects[*it] = config->Enabled();
            config->Enabled(false);
        }
    }

    //QElapsedTimer timer;
    WatercolorPass pass;
    for (auto it = canvas->ObjectsRepository()->OrderedObjectsNames()->rbegin(); it != canvas->ObjectsRepository()->OrderedObjectsNames()->rend(); ++it) {
        qDebug() << "Processing objects" << *it;
        //timer.start();
        auto canvas_config = canvas->ObjectsRepository()->ObjectsConfiguration(*it);
        auto watercolor_config = config_[*it];
        bool previously_enabled = enabled_objects[*it];
        if (canvas_config && watercolor_config && previously_enabled) {
            canvas_config->Enabled(true);
            QColor fill_col = canvas_config->FillColor();
            QColor outline_col = canvas_config->OutlineColor();
            canvas_config->FillColor(Qt::black);
            canvas_config->OutlineColor(Qt::black);
            QImage src = canvas->RenderToImage(canvas->width(), canvas->height(), offscreen);
            //qDebug() << "---Rendering canvas to image..." << timer.elapsed() << "ms\n";
            QImage* texture = new QImage(watercolor_config->EffectsTexture());
            //qDebug() << "---Loading texture..." << timer.elapsed() << "ms\n";
            // TODO: Instead of passing single values, pass the configuration object instead!
            pass.EffectTexture(texture);
            pass.NoiseScaleFactor(watercolor_config->NoiseScaleFactor());
            pass.FinalBlendAlpha(watercolor_config->FinalBlendAlpha());
            pass.ThresholdCombinerAlpha(watercolor_config->ThresholdCombinerAlpha());
            pass.ThresholdCombinerThreshold(watercolor_config->ThresholdCombinerThreshold());
            pass.Name(watercolor_config->ObjectsName());
            QImage processed = pass.Process(/*input_image=*/src);
            //qDebug() << "---Post processing..." << timer.elapsed() << "ms\n";
            processed_images[*it] = processed;
            combination_order.push_back(processed);
            canvas_config->Enabled(false);
            canvas_config->FillColor(fill_col);
            canvas_config->OutlineColor(outline_col);
            delete texture;
        }
        //qDebug() << "Done -> took " << timer.elapsed() << "ms\n";
    }

    //qDebug() << "Combine results.";
    //timer.start();
    // Recommended sequence: buildings, highways, waterways, greenland, developed land.
    QImage combined = pass.Combine(combination_order, QColor(1, 1, 1));

    pass.Destroy();

#ifdef QT_DEBUG
    combined.save("combined.jpg", "JPG", 100);
#endif

    //qDebug() << "Done -> took " << timer.elapsed() << "ms\n";

    // Enable all previously enabled objects.
    for (auto it = canvas->ObjectsRepository()->OrderedObjectsNames()->begin(); it != canvas->ObjectsRepository()->OrderedObjectsNames()->end(); ++it) {
        auto config = canvas->ObjectsRepository()->ObjectsConfiguration(*it);
        if (config) {
            config->Enabled(enabled_objects[*it]);
        }
    }

    return combined;
}

}  // namespace effects
