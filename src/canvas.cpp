#include "canvas.h"
#include "objects.h"
#include "utils.h"

#include <QDebug>
#include <QKeyEvent>
#include <QOffscreenSurface>
#include <QSurfaceFormat>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>

#include <QPolygonF>
#include <QVector>
#include <QPointF>

#include <QPainter>
#include <QRect>

namespace osm {

Canvas::Canvas(int width, int height, osm::ObjectsRepository* repository, QWidget *parent)
    : QOpenGLWidget(parent), objects_repository_(repository), map_data_(nullptr),
      repaint_(false), show_image_(false), erase_(false)
{
    auto format = this->format();
    format.setSamples(8);
    setFormat(format);
    setFixedSize(width, height);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);

    transformation_.scale = 1;
    transformation_.translate_x = transformation_.translate_y = 0;
}

Canvas::~Canvas()
{
}

void Canvas::initializeGL()
{
    initializeOpenGLFunctions();  // It is very important that initializeGLFunctions() is at this location in the code

    glClearColor(.2, .2, .2, 1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);  // requires glMaterial for the rendered objects to be viewed in color
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_MULTISAMPLE);
}

void Canvas::paintEvent(QPaintEvent * /*e*/)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (erase_) {
        QPainter painter(this);
        painter.fillRect(0, 0, width(), height(), Qt::black);
        painter.end();
        erase_ = false;
        return;
    }
    if (map_data_ == nullptr) {
        return;
    }
    if (!repaint_) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);


    painter.fillRect(0, 0, width(), height(), Qt::white);

    QRect r(0, 0, width(), height());
    painter.translate(r.center());
    painter.scale(transformation_.scale, transformation_.scale);
    painter.translate(-r.center());
    painter.translate(transformation_.translate_x, transformation_.translate_y);

    if (show_image_) {
        painter.drawImage(QPointF(0, 0), image_);
    } else {
        paint(painter);
    }

    painter.end();
    repaint_ = false;
}

void Canvas::paint(QPainter& painter)
{
    QVector<Objects*> paint_this_;
    for (auto it = objects_repository_->OrderedObjectsNames()->begin(); it != objects_repository_->OrderedObjectsNames()->end(); ++it) {
        //try {
        auto config = objects_repository_->ObjectsConfiguration(*it);
        if (config) {
            if (objects_repository_->ObjectsConfiguration(*it)->Enabled()) {
                Objects* obj = objects_repository_->Objects(*it);
                paint_this_.push_back(obj);
            }
        } else {
            qDebug() << "Could not find the objects configuration for" << *it;
        }
        //} catch(const ObjectsConfigurationNotFound& e) {
        //    qDebug() << "Could not find the objects configuration for" << e.Message();
        //}
    }

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
    painter.fillRect(0, 0, width(), height(), QColor(Qt::white));

    for (auto it = paint_this_.begin(); it != paint_this_.end(); ++it) {
        ObjectsConfiguration* config = objects_repository_->ObjectsConfiguration(QString::fromStdString((*it)->Name()));
        if ((*it)->ObjectsType() == ObjectsTypes::OCEAN && (*it)->Size() > 0 && config->Enabled()) {
            painter.fillRect(0, 0, width(), height(), config->FillColor());
        }
        for (auto it_ways = (*it)->Ways()->begin(); it_ways != (*it)->Ways()->end(); ++it_ways) {
            QVector<QPointF> points = CreatePoints(*it_ways, width(), height(), map_data_, (*it)->ObjectsType()==ObjectsTypes::ANY);
            QPolygonF polygon(points);
            Way* way = *it_ways;
            if (way->is_closed && config->Enabled()) {
                if ((*it)->ObjectsType() == ObjectsTypes::OCEAN) {
                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::white);
                    painter.drawPolygon(polygon, Qt::FillRule::WindingFill);

                    // DEBUG
                    /*painter.setBrush(QColor(255,192,203));
                    QPen p_tmp(QColor(255,192,203));
                    p_tmp.setWidth(4);
                    painter.setPen(p_tmp);
                    painter.drawPolyline(polygon);

                    for (auto it_tmp = way->nodes.begin(); it_tmp != way->nodes.end(); ++it_tmp) {
                        if (it_tmp < way->nodes.begin()+20) {
                            painter.setPen(Qt::green);
                            painter.setBrush(Qt::green);
                            painter.drawEllipse((*it_tmp)->x, height() - (*it_tmp)->y, 15, 15);
                        } else if (it_tmp > way->nodes.end()-20) {
                            painter.setPen(Qt::red);
                            painter.setBrush(Qt::red);
                            painter.drawEllipse((*it_tmp)->x, height() - (*it_tmp)->y, 15, 15);
                        } else {
                            painter.setPen(Qt::yellow);
                            painter.setBrush(Qt::yellow);
                            painter.drawEllipse((*it_tmp)->x, height() - (*it_tmp)->y, 5, 5);
                        }
                    }*/
                } else if (config->Filled()) {
                    painter.setBrush(config->FillColor());
                    //painter.setPen(config->FillColor());
                    painter.setPen(Qt::transparent);
                    painter.drawPolygon(polygon, Qt::FillRule::WindingFill);
                }
            }
            if (config->Outlined()) {
                painter.setBrush(Qt::transparent);
                //painter.setBrush(config->OutlineColor());
                QPen p(config->OutlineColor());
                p.setWidth(config->LineWidth());
                painter.setPen(p);
                painter.drawPolyline(polygon);
            }
        }
    }
}

void Canvas::Update()
{
    ShowImage(RenderToImage());
}

void Canvas::MapData(osm::MapData *map_data)
{
    map_data_ = map_data;
}

QImage Canvas::RenderToImage()
{
    return RenderToImage(width(), height());
}

QImage Canvas::RenderToImage(int width, int height, bool offscreen)
{
    QOpenGLContext* ctx;
    QOffscreenSurface* surface;
    if (offscreen) {
        ctx = new QOpenGLContext(this);
        QSurfaceFormat format;
        ctx->setFormat(format);
        ctx->create();
        surface = new QOffscreenSurface;
        surface->setFormat(ctx->format());
        surface->create();
        ctx->makeCurrent(surface);
    } else {
        makeCurrent();
    }

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(8);
    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(width, height, format);
    fbo->bind();

    QOpenGLPaintDevice fboPaintDev(width, height);

    QPainter painter(&fboPaintDev);

    paint(painter);

    painter.end();

    paintGL();

    QImage image = fbo->toImage();

    fbo->release();
    delete fbo;

    if (offscreen) {
        ctx->doneCurrent();
        delete ctx;
        delete surface;
    } else {
        doneCurrent();
    }

    return image;
}

void Canvas::ShowImage(QImage image)
{
    image_ = image;
    repaint_ = true;
    show_image_ = true;
    repaint();
}

void Canvas::ResetTransformation()
{
    transformation_.scale = 1;
    transformation_.translate_x = 0;
    transformation_.translate_y = 0;
}

void Canvas::keyReleaseEvent(QKeyEvent* ke)
{
    if(ke->key() == Qt::Key_Plus) {
        transformation_.scale += 0.2;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Minus) {
        transformation_.scale -= 0.2;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Left) {
        transformation_.translate_x += 10;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Right) {
        transformation_.translate_x -= 10;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Up) {
        transformation_.translate_y += 10;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Down) {
        transformation_.translate_y -= 10;
        ke->accept();
        Update();
        return;
    } else if(ke->key() == Qt::Key_Backspace) {
        ResetTransformation();
        ke->accept();
        Update();
        return;
    } else {
        // Allow for the default handling of the key
        QOpenGLWidget::keyReleaseEvent(ke); // aka the base class implementation
    }
}

QVector<QPointF> Canvas::CreatePoints(Way* way, int width, int height, osm::MapData* map_data, bool map_coords)
{
    QVector<QPointF> points;
    auto it_nodes = way->nodes.begin();
    while (it_nodes != way->nodes.end()) {
        float x;
        float y;

        if (map_coords) {
            float lat = (*it_nodes)->lat;
            float lon = (*it_nodes)->lon;
            float min_lat = map_data->MinLat();
            float max_lat = map_data->MaxLat();
            float min_lon = map_data->MinLon();
            float max_lon = map_data->MaxLon();

            osm::utils::MapLatLonToXy(
                        lat, lon,
                        min_lat, max_lat,
                        min_lon, max_lon,
                        width, height,
                        x, y);
        } else {
            x = (*it_nodes)->x;
            y = (*it_nodes)->y;
        }

        points.append(QPointF(x, height - y));
        ++it_nodes;
    }
    return points;
}

}  // namespace osm
