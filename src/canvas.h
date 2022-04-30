#ifndef CANVAS_H
#define CANVAS_H

#include "objectsrepository.h"
#include "mapdata.h"

#include <map>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QWidget>
#include <set>
#include <string>

namespace osm {

class Canvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    static const int kMaxHeight = 512;

    explicit Canvas(int width, int height, osm::ObjectsRepository *repository, QWidget *parent = nullptr);
    virtual ~Canvas();

    virtual void initializeGL() override;

    void ObjectsRepository(ObjectsRepository* repository) { objects_repository_ = repository; }
    osm::ObjectsRepository* ObjectsRepository() { return objects_repository_; }
    void MapData(osm::MapData *map_data_);

    QImage RenderToImage();
    QImage RenderToImage(int width, int height, bool offscreen = false);
    void ShowImage(QImage image);

    void ResetTransformation();

public slots:
    void keyReleaseEvent(QKeyEvent*) override;

protected:
    void paintEvent(QPaintEvent *e) override;
    virtual void paint(QPainter& painter);

    void Update();

    QVector<QPointF> CreatePoints(Way* way, int width, int height, osm::MapData* map_data, bool map_coords = true);

    osm::ObjectsRepository* objects_repository_;
    osm::MapData* map_data_;
    osm::Transformation transformation_;

    bool repaint_;
    bool show_image_;
    QImage image_;
    bool erase_;
};

}  // namespace osm

#endif // CANVAS_H
