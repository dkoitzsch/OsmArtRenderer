#ifndef OCEANLANDMASSFACTORY_H
#define OCEANLANDMASSFACTORY_H

#include "objects.h"
#include "types.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*
 PSEUDO CODE ALGORITHM FOR COASTLINE POLYGONS:

 1) Calculate the border intersection points BIPs (DONE)

 2) Put in queue q clockwise beginning at the top border
     If first element is E, then remove from front and add to back
 queue q;
 for all BIP on top: add BIP to q
 for all BIP on right: add BIP to q
 for all BIP on bottom: add BIP to q
 for all BIP on left: add BIP to q
 if q.front is E: remove q.front and add to q.back

 3) Then run the algorithm:
 stack<path> s;
 while (!q.isempty())
    BIP = q.front
    check if BIP is A of tail of s.top
        no: s.push(new path); s.top.append(BIP)
        yes: s.top.append(BIP)
    check if BIP is E of head of s.top
        no: s.top.append(BIP)
        yes: s.top.append(BIP), close s.top, pop s.top

 path is just a vector of BIPs

 close-function:
 polygon p;
 for i=0 to i=path.size
    if i==0: p.add(path[i])
    else if i>0 && path[i]==E:
        if path[i-1] and path[i] different borders, then p.add(corner) && p.add(path[i])
        else: p.add(path[i]);
    else if i>0 && path[i]==A: p.add(all points of way between path[i] and path[i-1]
    else if i==path.size: p.add(all points of way between path[0] and path[i]

*/

namespace osm
{

enum BorderType { kTop = 0, kRight = 1, kBottom = 2, kLeft = 3, kBorderTypeDefault = -1 };
enum PointType { kStart = 0, kEnd = 1, kPointTypeDefault = -1 };
struct BorderIntersectionPoint
{
    std::shared_ptr<Point<double>> p;
    //Point<double> p;
    PointType point_type;
    Way* w;
    BorderType border_type;
    Line<double> border;  // top, right, bottom, left.

    BorderIntersectionPoint()
    {
        point_type = PointType::kPointTypeDefault;
        border_type = BorderType::kBorderTypeDefault;
        p = nullptr;
        w = nullptr;
    }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "Point: type=" << (point_type == PointType::kStart ? "start" : "end");
        if (p != nullptr) {
            ss << " x=" << p->x << ",y=" << p->y;
        }
        if (w != nullptr) {
            ss << " way=" << w->id << " <" << w << ">";
        }
        return ss.str();
    }

    // TODO: Check for way and x and y, too. Two different ways could go through the same coordinates at the same border.
    bool operator<(const BorderIntersectionPoint& p)  const {
        // If you would need to store the points for multiple border types
        // in one container, then you should use the following comparison:
        // return (border_type < p.border_type) || ((border_type == p.border_type) && (this->p < p.p)); //(point_type < p.point_type));
        // If you are only interested in the geographical order, then just compare the points.

        return (*this->p.get() < *p.p.get());
    }

    // TODO: Check for way and x and y, too. Two different ways could go through the same coordinates at the same border.
    bool operator==(const BorderIntersectionPoint& p)  const {
        // If you would need to store the points for multiple border types
        // in one container, then you should use the following comparison:
        // return p.border_type == border_type && p.p == this->p;//p.point_type == point_type;
        // If you are only interested in the geographical order, then just compare the points.

        return *p.p.get() == *this->p.get();
    }
};

struct CoastlinePath
{
    std::vector<osm::Point<double>> points;
    BorderIntersectionPoint start;
    BorderIntersectionPoint end;

    std::string ToString()
    {
        std::stringstream ss;
        ss << "Coastline Path: start=(" << start.to_string() << ")";
        ss << " end=(" << end.to_string();
        ss << " points=[(" << points[0].x << "," << points[0].y << "),...,(" << points[points.size()-1].x << "," << points[points.size()-1].y << ")]";
        return ss.str();
    }
};
}

namespace std {
template <>
struct hash<osm::BorderIntersectionPoint>
{
    size_t operator() (const osm::BorderIntersectionPoint &bip) const
    {
        return ((uint64_t)bip.p->x)<<32 | (uint64_t)bip.p->y;
    }
};
}

namespace osm
{

// Computes the landmass polygons from the coastline ways.
// The polygon coordinates are already mapped from lat/lon to the screen space, and
// therefore don't need to be mapped again when drawing.
// Remember to subclass the OsmObjCollection and its RenderPoints method.
class OceanLandmassFactory
{
public:
    explicit OceanLandmassFactory() = default;
    OceanLandmassFactory(osm::Objects* coastline_objects,
                         int window_width, int window_height,
                         float render_scale, osm::Point<int> render_offset,
                         const BoundingBox& bbox);
    void Build();

    std::set<Way*>& WorkSetWays() { return work_set_; }
    std::set<std::vector<osm::Point<double>>>& CoastlinePolygon() { return coastline_polygons_; }

private:
    // Get the way of which the startpoint is not equal to any other endpoint ("single startpoint").
    void FirstPass();
    // Now try to connect lines with not the same start- and endpoint IDs, but very close locations.
    void SecondPass();
    // This pass calculates the intersections of the connected ways with the bounding box.
    void ThirdPass();
    // Creates polygons from the generated coastlines and the found intersection points with the borders.
    void FourthPass();

    std::vector<osm::Point<double>> CloseCoastlinePath(std::vector<BorderIntersectionPoint>& path);
    std::vector<osm::Point<double>> GetCornerPoints(BorderIntersectionPoint& start, BorderIntersectionPoint& end);
    osm::Point<double> GetNextCorner(osm::Point<double> p);

    void GetClosestPoint(const Way* const way);
    // Returns the distance in KM.
    double Distance(double a_lat, double a_lon, double b_lat, double b_lon);
    //bool Intersection(const Line<int>& a, const Line<int>& b, Point<double>& c_out);
    std::shared_ptr<Point<double>> Intersection(const Line<double>& a, const Line<double>& b);
    void SetIntersectionPointType(BorderIntersectionPoint& border_ip, const Point<double>& node_im1, const Point<double>& node_i);

private:
    int render_width_;
    int render_height_;
    float render_scale_;
    Point<int> render_offset_;
    //std::set<Way*> ways_; // Working copy (TODO: not needed...)
    BoundingBox bbox_;

    /* rings */
    /* coastlines */
    /* unconnected_coastlines */

    std::set<std::vector<osm::Point<double>>> coastline_polygons_;

    // First pass variables.
    std::set<Way*> work_set_;
    osm::Objects* coastline_objects_;

    // Third pass variables.
    std::unordered_map<BorderType, std::set<BorderIntersectionPoint>, std::hash<int>> border_type_to_intersection_point_;
    //std::unordered_set<Point<double>, PointHashFunction<double>> intersection_points_;  // key = x, value = y; Used to avoid duplicates
    std::unordered_map<Way*, std::vector<BorderIntersectionPoint>> way_to_border_intersection_point_;
    std::unordered_map<BorderIntersectionPoint, Way*> border_intersection_point_to_way_;
    //std::unordered_map<BorderIntersectionPoint, CoastlinePath> bip_to_coastlinepath_;
    std::unordered_map<BorderIntersectionPoint, std::shared_ptr<CoastlinePath>> bip_to_coastlinepath_;
};

}  // namespace osm

#endif // OCEANLANDMASSFACTORY_H
