#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#include <math.h>
#include <string>
#include <vector>

namespace osm
{

namespace utils
{

const double kEpsilon = 0.000001;

float GetAspectRatio(const float& min_lat, const float& max_lat,
                       const float& min_lon, const float& max_lon);
void MapLatLonToXy(const float& lat, const float& lon,
                   const float& min_lat, const float& max_lat,
                   const float& min_lon, const float& max_lon,
                   const int& width, const int& height,
                   float& x, float& y);

std::vector<std::vector<osm::Point<float>>>
ConvertCoastline2Polygons(const std::vector<osm::Way*>* waysCoastline,
                          const float& min_lat, const float& max_lat,
                          const float& min_lon, const float& max_lon,
                          const int& width, const int& height);

std::vector<osm::Point<float>> CreatePoints(osm::Way* way, int width, int height,
                                            float minLat, float maxLat,
                                            float minLon, float maxLon,
                                            float renderScale, osm::Point<int> renderOffset);

double HaversineKm(double lat1, double long1, double lat2, double long2);

inline double Mat2x2Det(double a, double b, double c, double d)
{
    return a*d - b*c;
}

inline bool IsOnLine(const osm::Line<double>& line, const osm::Point<double>& p)
{
    double crossprod = (p.y - line.start.y) * (line.end.x - line.start.x) - (p.x - line.start.x) * (line.end.y - line.start.y);
    if (abs(crossprod) > kEpsilon) {
        return false;
    }

    double dotprod = (p.x - line.start.x) * (line.end.x - line.start.x) + (p.y - line.start.y) * (line.end.y - line.start.y);
    if (dotprod < 0) {
        return false;
    }

    double squared_len = (line.end.x - line.start.x) * (line.end.x - line.start.x) + (line.end.y - line.start.y) * (line.end.y - line.start.y);
    if (dotprod > squared_len) {
        return false;
    }

    return true;
}

std::string GetTimestampAsString();


}  // namespace utils
}  // namespace osm

#endif // UTILS_H
