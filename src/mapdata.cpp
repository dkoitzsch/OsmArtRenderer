#include "mapdata.h"

#include <QDebug>

namespace osm {

MapData::MapData()
{

}

MapData::~MapData()
{
    for (size_t i = 0; i < ways_.size(); ++i) {
        delete ways_[i];
        ways_[i] = nullptr;
    }
    // No need to delete closed_ways_ as they're contained in ways_.
    qDeleteAll(nodes_for_deletion_);
    qDeleteAll(tags_for_deletion_);
}

const std::vector<Way*> MapData::Ways()
{
    return ways_;
}

int MapData::NodesCount()
{
    return nodes_.size();
}

int MapData::WaysCount()
{
    return ways_.size();
}

float MapData::MinLat()
{
    return min_lat_;
}

float MapData::MaxLat()
{
    return max_lat_;
}

float MapData::MinLon()
{
    return min_lon_;
}

float MapData::MaxLon()
{
    return max_lon_;
}

}  // namespace osm
