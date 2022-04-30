#ifndef OBJECTSCONFIGURATION_H
#define OBJECTSCONFIGURATION_H

#include <QCheckBox>
#include <QColor>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

namespace osm {

class ObjectsConfiguration : public QObject
{
    Q_OBJECT
public:
    explicit ObjectsConfiguration() = default;
    explicit ObjectsConfiguration(bool enabled, int linewidth, QColor outline_color, bool outlined, QColor fill_color, bool filled);
    virtual ~ObjectsConfiguration();

    void Enabled(bool enabled) { enabled_ = enabled; }
    bool Enabled() { return enabled_; }
    void LineWidth(int w) { line_width_ = w; }
    int LineWidth() { return line_width_; }
    void OutlineColor(QColor col) { outline_color_ = col; }
    QColor OutlineColor() { return outline_color_; }
    void Outlined(bool outlined) { outlined_ = outlined; }
    bool Outlined() { return outlined_; }
    void FillColor(QColor col) { fill_color_ = col; }
    QColor FillColor() { return fill_color_; }
    void Filled(bool filled) { filled_ = filled; }
    bool Filled() { return filled_; }

    QWidget* Widget() { return widget_; }

signals:
    void Updated(ObjectsConfiguration* config);

private:
    QColor SelectColor(QLineEdit* target);

    bool enabled_;
    int line_width_;
    QColor outline_color_;
    bool outlined_;
    QColor fill_color_;
    bool filled_;

    QWidget* widget_;
    QSpinBox* ui_line_width_;
    QLineEdit* ui_fill_color_;
    QLineEdit* ui_outline_color_;
    QCheckBox* ui_enabled_;
    QCheckBox* ui_outlined_;
    QCheckBox* ui_filled_;
};

}  // namespace osm

#endif // OBJECTSCONFIGURATION_H
