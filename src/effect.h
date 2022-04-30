#ifndef EFFECT_H
#define EFFECT_H

#include <QImage>
#include <canvas.h>

namespace effects {

class Effect
{
public:
    virtual QImage Apply(osm::Canvas* canvas, bool offscreen) = 0;
    virtual ~Effect() {};
};

}  // namespace effects

#endif // EFFECT_H
