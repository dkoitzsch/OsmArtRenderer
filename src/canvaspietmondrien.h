#ifndef CANVASPIETMONDRIEN_H
#define CANVASPIETMONDRIEN_H

#include "canvas.h"
#include "objectsrepository.h"

#include <QPainter>

namespace osm {

class CanvasPietMondrien : public Canvas
{
    Q_OBJECT
public:
    // objects_name: this is the set of objects to render. Nothing else will be rendered.
    explicit CanvasPietMondrien(int width, int height, osm::ObjectsRepository *repository, QWidget *parent = nullptr);
    virtual ~CanvasPietMondrien();

protected:
    virtual void paint(QPainter& painter) override;
};

}  // namespace osm

#endif // CANVASPIETMONDRIEN_H
