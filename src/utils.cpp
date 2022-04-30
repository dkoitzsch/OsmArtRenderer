#include "utils.h"

#include "types.h"

#include <iomanip>
#include <cmath>
#include <ctime>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <vector>

// x equals lon; y equals lat => aspect_ratio = x / y => x = aspect_ratio * y
namespace osm
{

namespace utils
{

#define earthRadiusKm 6371.0

// This function converts decimal degrees to radians
double Deg2Rad(double deg) {
  return (deg * M_PI / 180.0);
}

//  This function converts radians to decimal degrees
double Rad2Deg(double rad) {
  return (rad * 180.0 / M_PI);
}

float Distance(const float &origin_lat, const float &origin_lon,
              const float &destination_lat, const float &destination_lon)
{
    float radius = 6371.0;  // km

    float dlat = Deg2Rad(destination_lat) - Deg2Rad(origin_lat);
    float dlon = Deg2Rad(destination_lon) - Deg2Rad(origin_lon);
    float a = sin(dlat/2.0) * sin(dlat/2.0) + cos(Deg2Rad(origin_lat)) \
        * cos(Deg2Rad(destination_lat)) * sin(dlon/2.0) * sin(dlon/2.0);
    float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return radius * c;
}

float GetAspectRatio(const float& min_lat, const float& max_lat,
                       const float& min_lon, const float& max_lon)
{
    return Distance(min_lat, min_lon, min_lat, max_lon)
            / Distance(min_lat, min_lon, max_lat, min_lon);
}

void MapLatLonToXy(const float& lat, const float& lon,
                      const float& min_lat, const float& max_lat,
                      const float& min_lon, const float& max_lon,
                      const int& width, const int& height,
                      float& x, float& y)
{
    float d_lat = max_lat - min_lat;
    float d_lon = max_lon - min_lon;
    float r_lat = (lat - min_lat) / d_lat;
    float r_lon = (lon - min_lon) / d_lon;
    x = r_lon * (float) width;
    y = r_lat * (float) height;
}


std::string GetTimestampAsString()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();
    return str;
}

/*float Str2Float(const std::string& str)
{
    try {
        return std::stof(str);
    } catch (std::invalid_argument& err) {
        std::cout << err.exception::what() << "\n";
    } catch (std::out_of_range& err) {
        std::cout << err.exception::what() << "\n";
    }
    return 0.0;
}*/

std::vector<std::vector<osm::Point<float>>>
ConvertCoastline2Polygons(const std::vector<osm::Way*>* waysCoastline,
                          const float& min_lat, const float& max_lat,
                          const float& min_lon, const float& max_lon,
                          const int& width, const int& height)
{
    /*
     Go through the ways until there's an orientation change.
     Create a polygon for all the ways with the previous orientation.
     Then proceed, until there's an orientation change again.
     Again, create a polygon for the points.
     etc.
     */
    //std::vector<ofPath> polygons;
    std::vector<std::vector<osm::Point<float>>> polygons;
    //ofPath currentPolygon;
    std::vector<osm::Point<float>> currentPolygon;
    //bool emptyPolygon = true;
    int currentDirection = 0;  // -1 = downwards, 1 = upwards
    for (auto it = waysCoastline->begin(); it != waysCoastline->end(); ++it) {
        osm::Way* way = *it;
        // Get the orientation of the way (whether water is right or left of it).
        // If downwards then the water is on the left side of the line.
        // (Note: Water is always on the right in the walking direction of the coastline.)
        int direction = -1;  // From the top to the bottom of the map.
        if (way->nodes.at(0)->lat < way->nodes.at(1)->lat) {
            direction = 1;
        }
        if (currentDirection == 0) {
            currentDirection = direction;
        }
        // Keep adding the points across ways, until there is an orientation change.
        if (currentDirection != direction) {
            //currentPolygon.setFilled(true);
            polygons.push_back(currentPolygon);  // Pushs a copy.
            currentPolygon.clear();
            currentDirection = direction;
            //emptyPolygon = true;
        } else {
            //ofPath p;
            //auto node = way->nodes.begin();
            for (auto node = way->nodes.begin(); node != way->nodes.end(); ++node) {
                float x, y;
                MapLatLonToXy((*node)->lat, (*node)->lon,
                              min_lat, max_lat,
                              min_lon, max_lon,
                              width, height,
                              x, y);
                currentPolygon.push_back({.x=x, .y=y});
            }
            /*if (emptyPolygon) {
                currentPolygon.moveTo(x, y);
                emptyPolygon = false;
            } else {
                currentPolygon.lineTo(x, y);
            }
            ++node;
            for (; node != way->nodes.end(); ++node) {
                mapLatLonToXy((*node)->lat, (*node)->lon,
                              min_lat, max_lat,
                              min_lon, max_lon,
                              width, height,
                              x, y);
                //currentPolygon.lineTo(x, y);
            }
            */
        }
    }
    // TODO: Create vector of points instead of polygon
    polygons.push_back(currentPolygon);  // Push the last polygon, too.
    return polygons;
}

std::vector<osm::Point<float>> CreatePoints(osm::Way* way, int width, int height,
                                            float minLat, float maxLat,
                                            float minLon, float maxLon,
                                            float renderScale, osm::Point<int> renderOffset)
{
    std::vector<osm::Point<float>> points;
    for (auto it = way->nodes.begin(); it != way->nodes.end(); ++it) {
        float x;
        float y;
        float lat = (*it)->lat;
        float lon = (*it)->lon;

        MapLatLonToXy(
                    lat, lon,
                    minLat, maxLat,
                    minLon, maxLon,
                    width, height,
                    x, y);
        x *= renderScale;
        y *= renderScale;
        x += renderOffset.x;
        y += renderOffset.y;

        points.push_back(osm::Point<float>{.x = x, .y = height - y});
    }
    return points;
}

#define d2r 0.0174532925199433
double HaversineKm(double lat1, double long1, double lat2, double long2)
{
    double dlong = (long2 - long1) * d2r;
    double dlat = (lat2 - lat1) * d2r;
    double a = pow(sin(dlat/2.0), 2) + cos(lat1*d2r) * cos(lat2*d2r) * pow(sin(dlong/2.0), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    double d = 6367 * c;
    return d;
}

}  // namespace utils
}  // namespace osm
