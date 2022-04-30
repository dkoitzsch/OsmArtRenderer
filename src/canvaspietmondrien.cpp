#include "canvaspietmondrien.h"
#include "constants.h"

namespace osm {

CanvasPietMondrien::CanvasPietMondrien(int width, int height, osm::ObjectsRepository *repository, QWidget *parent)
    : Canvas(width, height, repository, parent)
{

}

CanvasPietMondrien::~CanvasPietMondrien()
{

}

void CanvasPietMondrien::paint(QPainter& painter)
{
    if (objects_repository_->Objects(kBuildingsName) == nullptr) {
        throw ObjectsNotFound("Could not find the objects with name " + QString(kBuildingsName));
    }
    if (objects_repository_->Objects(kHighwaysName) == nullptr) {
        throw ObjectsNotFound("Could not find the objects with name " + QString(kHighwaysName));
    }
    if (objects_repository_->Objects(kHighwaysExtName) == nullptr) {
        throw ObjectsNotFound("Could not find the objects with name " + QString(kHighwaysName));
    }

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
    painter.fillRect(0, 0, width(), height(), QColor(Qt::white));

    QColor col;

    //ObjectsConfiguration* config = objects_repository_->ObjectsConfiguration(kBuildingsName);
    for (auto it_ways = objects_repository_->Objects(kBuildingsName)->Ways()->begin();
         it_ways != objects_repository_->Objects(kBuildingsName)->Ways()->end(); ++it_ways) {
        QVector<QPointF> points = CreatePoints(*it_ways, width(), height(), map_data_, true);

        int r = random() % 3;
        if (r == 0) {
            col.setRgb(255, 0, 0);
        } else if (r == 1) {
            col.setRgb(255, 255, 0);
        } else if (r == 2) {
            col.setRgb(0, 0, 255);
        }

        QPolygonF polygon(points);
        Way* way = *it_ways;
        // Ignore whether it's enabled or not.
        if (way->is_closed) {
            painter.setBrush(col);
            painter.setPen(col);
            painter.drawPolygon(polygon, Qt::FillRule::WindingFill);
        }
    }

    for (auto it_ways = objects_repository_->Objects(kHighwaysName)->Ways()->begin();
         it_ways != objects_repository_->Objects(kHighwaysName)->Ways()->end(); ++it_ways) {
        QVector<QPointF> points = CreatePoints(*it_ways, width(), height(), map_data_, true);
        QPolygonF polygon(points);
        // Ignore whether it's enabled or not.
        painter.setBrush(QColor(50, 50, 50));
        QPen p(QColor(50, 50, 50));
        p.setWidth(2);
        painter.setPen(p);
        painter.drawPolyline(polygon);
    }

    for (auto it_ways = objects_repository_->Objects(kHighwaysExtName)->Ways()->begin();
         it_ways != objects_repository_->Objects(kHighwaysExtName)->Ways()->end(); ++it_ways) {
        QVector<QPointF> points = CreatePoints(*it_ways, width(), height(), map_data_, true);
        QPolygonF polygon(points);
        // Ignore whether it's enabled or not.
        painter.setBrush(Qt::black);
        QPen p(Qt::black);
        p.setWidth(5);
        painter.setPen(p);
        painter.drawPolyline(polygon);
    }
}

}  // namespace osm
