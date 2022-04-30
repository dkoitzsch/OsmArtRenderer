#include "objectsconfiguration.h"

#include <QColorDialog>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QString>

namespace osm {

ObjectsConfiguration::ObjectsConfiguration(bool enabled, int linewidth, QColor outline_color, bool outlined, QColor fill_color, bool filled)
    : enabled_(enabled), line_width_(linewidth), outline_color_(outline_color), outlined_(outlined), fill_color_(fill_color), filled_(filled)
{
    widget_ = new QWidget();

    QFormLayout* layout = new QFormLayout;
    ui_enabled_ = new QCheckBox;
    layout->addRow(new QLabel("Enabled:"), ui_enabled_);
    ui_line_width_ = new QSpinBox;
    layout->addRow(new QLabel("Line Width:"), ui_line_width_);
    ui_outlined_ = new QCheckBox;
    layout->addRow(new QLabel("Outlined"), ui_outlined_);
    QHBoxLayout* outline_col_layout = new QHBoxLayout;
    ui_outline_color_ = new QLineEdit;
    ui_outline_color_->setEnabled(false);
    outline_col_layout->addWidget(ui_outline_color_);
    QPushButton* select_outline_col = new QPushButton("Select...");
    outline_col_layout->addWidget(select_outline_col);
    layout->addRow(new QLabel("Outline Color"), outline_col_layout);
    ui_filled_ = new QCheckBox;
    layout->addRow(new QLabel("Filled"), ui_filled_);
    QHBoxLayout* fill_col_layout = new QHBoxLayout;
    ui_fill_color_ = new QLineEdit;
    ui_fill_color_->setEnabled(false);
    fill_col_layout->addWidget(ui_fill_color_);
    QPushButton* select_fill_col = new QPushButton("Select...");
    fill_col_layout->addWidget(select_fill_col);
    layout->addRow(new QLabel("Fill Color"), fill_col_layout);

    ui_enabled_->setChecked(enabled_);
    ui_line_width_->setValue(line_width_);
    ui_outlined_->setChecked(outlined_);
    ui_outline_color_->setText(QString::number(outline_color_.red()) + ","
                               + QString::number(outline_color_.green()) + "," + QString::number(outline_color_.blue()));
    ui_filled_->setChecked(filled_);
    ui_fill_color_->setText(QString::number(fill_color_.red()) + ","
                            + QString::number(fill_color_.green()) + "," + QString::number(fill_color_.blue()));

    widget_->setLayout(layout);

    QObject::connect(select_fill_col, &QPushButton::clicked, this, [this] {
        QColor col = SelectColor(ui_fill_color_);
        if (col != Qt::transparent) {
            fill_color_ = col;
            QString col_str = QString::number(col.red()) + ","
                    + QString::number(col.green()) + "," + QString::number(col.blue());
            ui_fill_color_->setText(col_str);
            ui_fill_color_->setStyleSheet("QLineEdit {background-color: rgb(" + col_str + ");}");
            emit Updated(this);
        }
    });
    QObject::connect(select_outline_col, &QPushButton::clicked, this, [this] {
        QColor col = SelectColor(ui_outline_color_);
        if (col != Qt::transparent) {
            outline_color_ = col;
            QString col_str = QString::number(col.red()) + ","
                    + QString::number(col.green()) + "," + QString::number(col.blue());
            ui_outline_color_->setText(col_str);
            ui_outline_color_->setStyleSheet("QLineEdit {background-color: rgb(" + col_str + ");}");
            emit Updated(this);
        }
    });
    QObject::connect<void(QSpinBox::*)(int)>(ui_line_width_, &QSpinBox::valueChanged, this, [this](int value) {
        line_width_ = value;
        emit Updated(this);
    });
    QObject::connect(ui_outlined_, &QCheckBox::clicked, this, [this](bool value) {
        outlined_ = value;
        emit Updated(this);
    });
    QObject::connect(ui_filled_, &QCheckBox::clicked, this, [this](bool value) {
        filled_ = value;
        emit Updated(this);
    });
    QObject::connect(ui_enabled_, &QCheckBox::clicked, this, [this](bool value) {
        enabled_ = value;
        emit Updated(this);
    });
}

ObjectsConfiguration::~ObjectsConfiguration()
{
    //delete widget_;
}


QColor ObjectsConfiguration::SelectColor(QLineEdit* target)
{
    QColorDialog dlg(widget_);
    //dlg.setWindowFlags(Qt::Window);

    /*QPoint dialog_center = mapToGlobal(dlg.rect().center());
    QPoint parent_window_center = this->mapToGlobal(rect().center());

    // Get current screen size
    QRect rec = QGuiApplication::screenAt(this->pos())->geometry();

    // Using minimum size of window
    QSize size = this->minimumSize();

    // Set top left point
    QPoint topLeft = QPoint((rec.width() / 2) - (size.width() / 2), (rec.height() / 2) - (size.height() / 2));

    // set window position
    dlg.setGeometry(QRect(topLeft, size));*/

    //dlg.move(parent_window_center - dialog_center);
    auto col = target->text().trimmed().split(",");
    dlg.setCurrentColor(QColor(col[0].toInt(), col[1].toInt(), col[2].toInt()));
    dlg.exec();
    QColor color = dlg.selectedColor();

    if(color.isValid()) {
        qDebug() << "Color Choosen : " << color.name();
        target->setText(QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue()));
        return color;
    }
    return Qt::transparent;
}

}  // namespace osm
