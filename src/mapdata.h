#ifndef OSMMAPDATA_H
#define OSMMAPDATA_H

#include "types.h"

#include <map>
#include <string>
#include <vector>

namespace osm {

class Parser;

class MapData
{
public:
    MapData();
    ~MapData();
    const std::vector<Way*> Ways();
    int NodesCount();
    int WaysCount();
    float MinLat();
    float MaxLat();
    float MinLon();
    float MaxLon();

private:
    float min_lon_;
    float max_lon_;
    float min_lat_;
    float max_lat_;
    std::vector<Way*> ways_;
    std::map<std::string, Node*> nodes_;
    std::vector<Node*> nodes_for_deletion_;
    std::vector<Tag*> tags_for_deletion_;
    Point<float> min_xy_;
    Point<float> max_xy_;

    friend class Parser;
};

} // namespace osm

#endif // OSMMAPDATA_H
