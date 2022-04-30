#include "objects.h"
#include "oceanlandmassfactory.h"
#include "utils.h"

#include <QDebug>
#include <QString>

#include <iostream>
#include <iterator>     // std::back_inserter
#include <algorithm>    // std::copy
#include <queue>
#include <stack>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace osm {

OceanLandmassFactory::OceanLandmassFactory(Objects* coastline_objects,
                                           int window_width, int window_height,
                                           float render_scale, Point<int> render_offset,
                                           const BoundingBox& bbox) :
render_width_(window_width), render_height_(window_height),
render_scale_(render_scale), render_offset_(render_offset),
bbox_(bbox), coastline_objects_(coastline_objects)
{

}

void OceanLandmassFactory::Build()
{
    if (coastline_objects_->Ways()->empty()) {
        return;
    }

    work_set_.clear();
    coastline_polygons_.clear();
    border_type_to_intersection_point_.clear();
    for (auto it = way_to_border_intersection_point_.begin(); it != way_to_border_intersection_point_.end(); ++it) {
        it->second.clear();
    }
    way_to_border_intersection_point_.clear();
    border_intersection_point_to_way_.clear();
    bip_to_coastlinepath_.clear();

    FirstPass();
    //SecondPass();
    ThirdPass();
    FourthPass();
}

void OceanLandmassFactory::FirstPass()
{
    std::set<std::string> startpoints;
    std::set<std::string> endpoints;
    std::unordered_map<std::string, Way*> endpoints_to_ways;
    std::unordered_map<std::string, Way*> startpoints_to_ways;

    // Create local datastructures with start- and endpoints only,
    // and their mapping to their respective ways.
    for (auto it = coastline_objects_->Ways()->begin(); it != coastline_objects_->Ways()->end(); ++it) {
        Way* w = *it;
        Node* ep = w->nodes.at(w->nodes.size() - 1);
        Node* sp = w->nodes.at(0);
        endpoints.insert(ep->id);
        endpoints_to_ways[ep->id] = w;
        startpoints.insert(sp->id);
        startpoints_to_ways[sp->id] = w;
        work_set_.insert(w);
    }

    /*
     * There are two ways where the endpoint of the first way
     * has the same id as the startpoint of the second way
     *
     *       ep
     * o-----o
     *       sp_it
     *       o---------o
     *
     * In the end there will be one combined way
     * (the sp_it point will not be added - therefore +1 when copying,
     * since this point is equal to ep).
     * o---------------o
     *
     * You remove the two original ways, and insert the new way instead.
     * You also want to make sure that you remove the ways and their ids
     * from the local data structures.
     * You then add the newly created way to the mapping of points to ways.
     *
     * In step n-1 there will be one startpoint and one endpoint:
     * o-----o
     *       o-----------.....(very long)...----o
     * o-----------------...................----o
     * And in this case ep and sp belong to the same way, and you just
     * remove the endpoint from the local datastructure, and the sp
     * remains -> leave the while loop in step n
     * */
    while (!endpoints.empty()) {
        std::string ep = *endpoints.begin();

        auto sp_it = startpoints.find(ep);
        // If a startpoint could be found with the same id, and the ways for each point is not the same...
        if (sp_it != startpoints.end() && (startpoints_to_ways[*sp_it]->id != endpoints_to_ways[ep]->id)) {
            std::string sp = *sp_it;
            Way* way = new Way;
            Way* sp_w = startpoints_to_ways[sp];
            Way* ep_w = endpoints_to_ways[ep];
            way->id = ep_w->id + " + " + sp_w->id;
            std::copy(ep_w->nodes.begin(), ep_w->nodes.end(), std::back_inserter(way->nodes));
            std::copy(sp_w->nodes.begin() + 1, sp_w->nodes.end(), std::back_inserter(way->nodes));
            work_set_.erase(work_set_.find(ep_w));
            work_set_.erase(work_set_.find(sp_w));
            work_set_.insert(way);

            // Remove the sp and ep which are connected now - they're in the middle of the new way now.
            if (startpoints.find(sp) != startpoints.end()) startpoints.erase(startpoints.find(sp));
            if (endpoints.find(ep) != endpoints.end()) endpoints.erase(endpoints.find(ep));
            if (endpoints_to_ways.find(ep) != endpoints_to_ways.end()) endpoints_to_ways.erase(endpoints_to_ways.find(ep));
            if (startpoints_to_ways.find(sp) != startpoints_to_ways.end()) startpoints_to_ways.erase(startpoints_to_ways.find(sp));

            // And update the sp and ep to point to the new way
            endpoints_to_ways[way->nodes.at(way->nodes.size() - 1)->id] = way;
            startpoints_to_ways[way->nodes.at(0)->id] = way;

        }

        // ep could already be deleted above after the new connected way was created.
        // This case here happens if the above conditions are false: either there is no startpoint
        // for this endpoint, or their way ids are equal (they belong to the same way).
        if (endpoints.find(ep) != endpoints.end()) {
            endpoints.erase(endpoints.find(ep));
        }
    }
}

void OceanLandmassFactory::SecondPass()
{
    /*std::cout << "Check for SP -> EP mapping.\n";
    // Startpoint of w1 to endpoint of w2 mapping.
    std::string_view("_IGNORE");
    for (auto it1 = coastlines_ways_->ways()->begin(); it1 != coastlines_ways_->ways()->end(); ++it1) {
        Way* w1 = *it1;
        if (ends_with(w1->id, "_IGNORE")) {
            continue;
        }
        if (attached_to_another_way_.find(w1) != attached_to_another_way_.end()) {
            // Ok, this way w1 was already attached to another way.
            continue;
        }
        Node* n1 = w1->nodes.at(0);
        // Now iterate over all ways again and check if there's a way with an apropriate endpoint.
        for (auto it2 = coastlines_ways_->ways()->begin(); it2 != coastlines_ways_->ways()->end(); ++it2) {
            Way* w2 = *it2;
            if (ends_with(w2->id, "_IGNORE")) {
                continue;
            }
            if (w1 == w2) {
                // Obviously, if it is the same, continue to search.
                continue;
            }
            // First check if this way w1 was already attached to the way w2.
            if ((set_of_ways_attached_to_another_way_.find(w2) != set_of_ways_attached_to_another_way_.end())
                && (set_of_ways_attached_to_another_way_[w2].find(w1) != set_of_ways_attached_to_another_way_[w2].end())) {
                continue;
            }
            // Otherwise calculate the distance between the points.
            Node* n2 = w2->nodes.at(w2->nodes.size()-1);
            if (n1->id != n2->id) {
                double d = distance(n1->lat, n1->lon, n2->lat, n2->lon);
                if (processed_way_.find(w1) != processed_way_.end()) {
                    std::cout << "w1 was already processed\n";
                }
                if (processed_way_.find(w2) != processed_way_.end()) {
                    std::cout << "w2 was already processed\n";
                }
                std::cout << "Distance between SP w1 ID:" << w1->id << " and EP w2 ID:" << w2->id << " = " << d << "\n\n";
            }
        }
    }

    std::cout << "Check for EP -> SP mapping.\n";
    // Endpoint of w1 to startpoint of w2 mapping.
    for (auto it1 = coastlines_ways_->ways()->begin(); it1 != coastlines_ways_->ways()->end(); ++it1) {
        Way* w1 = *it1;
        if (ends_with(w1->id, "_IGNORE")) {
            continue;
        }
        Node* n1 = w1->nodes.at(w1->nodes.size()-1);
        for (auto it2 = coastlines_ways_->ways()->begin(); it2 != coastlines_ways_->ways()->end(); ++it2) {
            Way* w2 = *it2;
            if (ends_with(w2->id, "_IGNORE")) {
                continue;
            }
            if (w1 == w2) {
                continue;
            }
            if (attached_to_another_way_.find(w2) != attached_to_another_way_.end()) {
                // Ok, this way w2 was already attached to another way.
                continue;
            }
            Node* n2 = w2->nodes.at(0);
            if (n1->id != n2->id) {
                double d = distance(n1->lat, n1->lon, n2->lat, n2->lon);
                if (processed_way_.find(w1) != processed_way_.end()) {
                    std::cout << "w1 was already processed\n";
                }
                if (processed_way_.find(w2) != processed_way_.end()) {
                    std::cout << "w2 was already processed\n";
                }
                std::cout << "Distance between EP w1 ID:" << w1->id << " and SP w2 ID:" << w2->id << " = " << d << "\n\n";
            }
        }
    }
     */
}

// Set the intersection point to either start or end depending on the vector between the node points and the border type.
// node_im1 and node_i are the two node points between which the intersection was found.
void OceanLandmassFactory::SetIntersectionPointType(BorderIntersectionPoint& border_ip, const Point<double>& node_im1, const Point<double>& node_i)
{
    // Just check if the points are inside or outside the bbox.
    if ((node_im1.x > 0 && node_im1.x < render_width_ && node_im1.y > 0 && node_im1.y < render_height_)
        && (node_i.x <= 0 || node_i.x >= render_width_ || node_i.y <= 0 || node_i.y >= render_height_)){
        // i-1 inside, i outside -> line leaves the bbox => endpoint
        border_ip.point_type = PointType::kEnd;
    } else if ((node_i.x >= 0 && node_i.x <= render_width_ && node_i.y >= 0 && node_i.y <= render_height_)
               && (node_im1.x < 0 || node_im1.x > render_width_ || node_im1.y < 0 || node_im1.y > render_height_)){
        // i-1 outside, i inside -> line enters the bbox => startpoint
        border_ip.point_type = PointType::kStart;
    }
}

void OceanLandmassFactory::ThirdPass()
{
    //std::unordered_map<BorderType, std::set<BorderIntersectionPoint>, std::hash<int>> border_type_to_intersection_point;
    std::unordered_set<Point<double>, PointHashFunction<double>> intersection_points;  // key = x, value = y; Used to avoid duplicates
    //std::unordered_map<Way*, std::vector<BorderIntersectionPoint>> way_to_border_intersection_point;

    std::shared_ptr<CoastlinePath> current_coastlinepath = nullptr;

    // This algorithm will always find the startpoint first, then the endpoint.
    // This is because it always starts with the first node and iterates through to the end node.

    border_type_to_intersection_point_[BorderType::kTop] = {};
    border_type_to_intersection_point_[BorderType::kRight] = {};
    border_type_to_intersection_point_[BorderType::kBottom] = {};
    border_type_to_intersection_point_[BorderType::kLeft] = {};

    Line<double> top = {.start={.x=0,.y=0},.end={.x=static_cast<double>(render_width_),.y=0}};
    Line<double> right = {.start={.x=static_cast<double>(render_width_),.y=0},.end={.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)}};
    Line<double> bottom = {.start={.x=0,.y=static_cast<double>(render_height_)},.end={.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)}};
    Line<double> left = {.start={.x=0,.y=0},.end={.x=0,.y=static_cast<double>(render_height_)}};

    //Point<double> intersection_pt = {.x=0,.y=0};
    //bool intersects = false;

    // TODO: Allow alternating start and endpoint. I.e. a long coastline that
    // intersects with borders multiple times.

    // NOTE: If the nodes are quite close to the border, it can happen, that two consecutive points
    // are determined as two end or two start points. You can ignore these points since the
    // area described by them is too small anyways.
    // Added note after reading this: really ????

    for (auto it = work_set_.begin(); it != work_set_.end(); ++it) {
        Way* w = *it;
        way_to_border_intersection_point_[w] = {};
        //std::cout << "Computing intersections for way " << w->id << " <" << w << ">\n";
        // TODO: Add the intersections in a vector or queue. Then it should be star-end-start-end-... for the same way.
        Point<double> a, b;
        float x, y;
        for (size_t node_i = 1; node_i < w->nodes.size(); ++node_i) {
            osm::utils::MapLatLonToXy(w->nodes.at(node_i - 1)->lat,
                                 w->nodes.at(node_i - 1)->lon,
                                 bbox_.min_lat, bbox_.max_lat,
                                 bbox_.min_lon, bbox_.max_lon,
                                 render_width_, render_height_,
                                 x, y);
            a.x = x;
            a.y = render_height_ - y;
            osm::utils::MapLatLonToXy(w->nodes.at(node_i)->lat,
                                 w->nodes.at(node_i)->lon,
                                 bbox_.min_lat, bbox_.max_lat,
                                 bbox_.min_lon, bbox_.max_lon,
                                 render_width_, render_height_,
                                 x, y);
            b.x = x;
            b.y = render_height_ - y;

            // Find an intersection between node i-1 and i and the top border.
            std::shared_ptr<Point<double>> top_pt = Intersection({.start=a,.end=b}, top);
            if (top_pt) {
                // We found the intersection...
                if (intersection_points.find(*top_pt) == intersection_points.end()) {
                    // ...and it has not yet been discovered / added before.
                    BorderIntersectionPoint border_ip;
                    border_ip.p = top_pt;
                    border_ip.w = w;
                    border_ip.border_type = BorderType::kTop;
                    border_ip.border = top;
                    SetIntersectionPointType(border_ip, a, b);
                    if (border_ip.point_type == PointType::kStart) {
                        // In that case the i-1 node was outside of the border, and i was inside.
                        // Then create a new coastline path - if not yet there.
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath = bip_to_coastlinepath_[border_ip];  //????? WTF?
                        current_coastlinepath->start = border_ip;
                        current_coastlinepath->points.push_back({.x=top_pt->x,.y=top_pt->y});
                    } else if (border_ip.point_type == PointType::kEnd) {
                        // In that case the i-1 node was inside of the border, and i was outside
                        // TODO: if current_coastlinepath == nullptr then create it.
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath->end = border_ip;
                        current_coastlinepath->points.push_back({.x=top_pt->x,.y=top_pt->y});
                        current_coastlinepath = nullptr;
                    }
                    border_type_to_intersection_point_[BorderType::kTop].insert(border_ip);
                    way_to_border_intersection_point_[w].push_back(border_ip);
                    border_intersection_point_to_way_[border_ip] = w;
                    intersection_points.insert(*top_pt);
                }
            }
            std::shared_ptr<Point<double>> right_pt = Intersection({.start=a,.end=b}, right);
            if (right_pt) {
                if (intersection_points.find(*right_pt) == intersection_points.end()) {
                    BorderIntersectionPoint border_ip;
                    border_ip.p = right_pt;
                    border_ip.w = w;
                    border_ip.border_type = BorderType::kRight;
                    border_ip.border = top;
                    SetIntersectionPointType(border_ip, a, b);
                    if (border_ip.point_type == PointType::kStart) {
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath = bip_to_coastlinepath_[border_ip];
                        current_coastlinepath->start = border_ip;
                        current_coastlinepath->points.push_back({.x=right_pt->x,.y=right_pt->y});
                    } else if (border_ip.point_type == PointType::kEnd) {
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath->end = border_ip;
                        current_coastlinepath->points.push_back({.x=right_pt->x,.y=right_pt->y});
                        current_coastlinepath = nullptr;
                    }
                    border_type_to_intersection_point_[BorderType::kRight].insert(border_ip);
                    way_to_border_intersection_point_[w].push_back(border_ip);
                    border_intersection_point_to_way_[border_ip] = w;
                    intersection_points.insert(*right_pt);
                }
            }
            std::shared_ptr<Point<double>> bottom_pt = Intersection({.start=a,.end=b}, bottom);
            if (bottom_pt) {
                if (intersection_points.find(*bottom_pt) == intersection_points.end()) {
                    BorderIntersectionPoint border_ip;
                    border_ip.p = bottom_pt;
                    border_ip.w = w;
                    border_ip.border_type = BorderType::kBottom;
                    border_ip.border = top;
                    SetIntersectionPointType(border_ip, a, b);
                    if (border_ip.point_type == PointType::kStart) {
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath = bip_to_coastlinepath_[border_ip];
                        current_coastlinepath->start = border_ip;
                        current_coastlinepath->points.push_back({.x=bottom_pt->x,.y=bottom_pt->y});
                    } else if (border_ip.point_type == PointType::kEnd) {
                        if (current_coastlinepath == nullptr) {
                            // Is this code ever reached???
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath; // Also set the reference for the endpoint.
                        current_coastlinepath->end = border_ip;
                        current_coastlinepath->points.push_back({.x=bottom_pt->x,.y=bottom_pt->y});
                        current_coastlinepath = nullptr;
                    }
                    border_type_to_intersection_point_[BorderType::kBottom].insert(border_ip);
                    way_to_border_intersection_point_[w].push_back(border_ip);
                    border_intersection_point_to_way_[border_ip] = w;
                    intersection_points.insert(*bottom_pt);
                }
            }
            std::shared_ptr<Point<double>> left_pt = Intersection({.start=a,.end=b}, left);
            if (left_pt) {
                if (intersection_points.find(*left_pt) == intersection_points.end()) {
                    BorderIntersectionPoint border_ip;
                    border_ip.p = left_pt;
                    border_ip.w = w;
                    border_ip.border_type = BorderType::kLeft;
                    border_ip.border = top;
                    SetIntersectionPointType(border_ip, a, b);
                    if (border_ip.point_type == PointType::kStart) {
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath = bip_to_coastlinepath_[border_ip];
                        current_coastlinepath->start = border_ip;
                        current_coastlinepath->points.push_back({.x=left_pt->x,.y=left_pt->y});
                    } else if (border_ip.point_type == PointType::kEnd) {
                        if (current_coastlinepath == nullptr) {
                            current_coastlinepath = std::make_shared<CoastlinePath>();
                        }
                        bip_to_coastlinepath_[border_ip] = current_coastlinepath;
                        current_coastlinepath->end = border_ip;
                        current_coastlinepath->points.push_back({.x=left_pt->x,.y=left_pt->y});
                        current_coastlinepath = nullptr;
                    }
                    border_type_to_intersection_point_[BorderType::kLeft].insert(border_ip);
                    way_to_border_intersection_point_[w].push_back(border_ip);
                    border_intersection_point_to_way_[border_ip] = w;
                    intersection_points.insert(*left_pt);
                }
            }

            if (current_coastlinepath != nullptr) {
                // Keep pushing the nodes / points as long as there is a current (open) path.
                // That means a startpoint was found and opens the way, and not yet an endpoint which closes the way.
                current_coastlinepath->points.push_back(b);
            }
        }
    }
}

std::vector<osm::Point<double>> OceanLandmassFactory::CloseCoastlinePath(std::vector<BorderIntersectionPoint>& path)
{
    std::vector<osm::Point<double>> coastline_polygon;

    // There is a special case if there are only two points. Then all the corners need to be added, too.
    if (path.size() == 2) {
        //std::cout << "Special case: just two points\n";
        auto p_0 = path[0].p;
        auto p_1 = path[1].p;

        // Add the first point p_0.
        coastline_polygon.push_back({.x=p_0->x,.y=p_0->y});

        // Then add all the corners betweeen the first (p_0) and second point (p_1).
        if (path[0].point_type == PointType::kStart && path[1].point_type == PointType::kEnd) {
            std::vector<osm::Point<double>> corners = GetCornerPoints(path[0], path[1]);
            for (auto c_it = corners.begin(); c_it != corners.end(); ++c_it) {
                coastline_polygon.push_back(*c_it);
            }
        }

        // Add the second point p_1.
        coastline_polygon.push_back({.x=p_1->x,.y=p_1->y});

        /*
        // The two points are on oppsite sides.
        if (p_0->y == 0 && p_1->y == render_height_) {
            // Add corners to the right of the line (top right, bottom right).
            coastline_polygon.push_back({.x=p_0->x,.y=p_0->y});
            coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=0});
            coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)});
            coastline_polygon.push_back({.x=p_1->x,.y=p_1->y});
        } else if (p_0->y == render_height_ && p_1->y == 0) {
            // Add corners to the right of the line (bottom left, top left).
            coastline_polygon.push_back({.x=p_0->x,.y=p_0->y});
            coastline_polygon.push_back({.x=0,.y=static_cast<double>(render_height_)});
            coastline_polygon.push_back({.x=0,.y=0});
            coastline_polygon.push_back({.x=p_1->x,.y=p_1->y});
        }  else if (p_0->x == 0 && p_1->x == render_width_) {
            // Add corners to the right of the line (top right, top left).
            coastline_polygon.push_back({.x=p_0->x,.y=p_0->y});
            coastline_polygon.push_back({.x=0,.y=0});
            coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=0});
            coastline_polygon.push_back({.x=p_1->x,.y=p_1->y});
        }  else if (p_0->x == render_width_ && p_1->x == 0) {
            // Add corners to the right of the line (bottom left, bottom right).
            coastline_polygon.push_back({.x=p_0->x,.y=p_0->y});
            coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)});
            coastline_polygon.push_back({.x=0,.y=static_cast<double>(render_height_)});
            coastline_polygon.push_back({.x=p_1->x,.y=p_1->y});
        }


        // The two points are on orthogonal sides.
        if (p_0->y == 0 && p_1->y > 0 && p_1->y < render_height_ && p_1->x == render_width_) {

        }
         */

        // And now add all the points between p_1 and p_0.
        // I assume that p_0 is always the STARTpoint. We fill up the polygon from end to start (p_1 to p_0).
        // Added note after reading this: HOW DOES THIS WORK? You can't start with the begin() of one entry (path[1]), and check
        // for an end() condition with another entry (path[0]). Or do path[0] and path[1] point to the same item??
        qDebug() << QString::fromStdString(path[0].to_string());
        qDebug() << QString::fromStdString(path[1].to_string());
        /*for (auto it = bip_to_coastlinepath_[path[1]]->points.begin(); it != bip_to_coastlinepath_[path[0]]->points.end(); ++it) {
            coastline_polygon.push_back(*it);
        }*/
        for (auto it = bip_to_coastlinepath_[path[1]]->points.rbegin(); it != bip_to_coastlinepath_[path[1]]->points.rend(); ++it) {
            coastline_polygon.push_back(*it);
        }
        //for (auto it = bip_to_coastlinepath_[path[0]]->points.begin(); it != bip_to_coastlinepath_[path[0]]->points.end(); ++it) {
        //    coastline_polygon.push_back(*it);
        //}
        return coastline_polygon;
    }

    // Add all points of all paths to the polygon.
    coastline_polygon.push_back({.x=path[0].p->x,.y=path[0].p->y});
    for (size_t i = 1; i < path.size(); ++i) {
        auto p_im1 = path[i-1].p;
        auto p_i = path[i].p;
        CoastlinePath* cp_im1 = bip_to_coastlinepath_[path[i-1]].get();
        CoastlinePath* cp_i = bip_to_coastlinepath_[path[i]].get();
        // If they're on the same border line, then just connect them directly.
        // If not then check clockwise for border / corner crossing. TODO: But only consider the corner cases if their coastline path is the same.
        /*if ((p_i->x == 0 && p_im1->x == 0) || (p_i->x == render_width_ && p_im1->x == render_width_)
            || (p_i->y == 0 && p_im1->y == 0) || (p_i->y == render_height_ && p_im1->y == render_height_)) {
            //coastline_polygon.push_back({.x=static_cast<int>(p_i->x),.y=static_cast<int>(p_i->y)});
        }*/

        /*
        // If it is the same coastline and i-1 is the startpoint and i is the endpoint
        // (that means the corner is on the landmass) then add the corners, too.
        if (//cp_im1 == cp_i &&
            path[i-1].point_type == PointType::kStart && path[i].point_type == PointType::kEnd) {
            if (p_im1->x == 0 && p_im1->y > 0 && p_im1->y < render_height_ && p_i->x > 0 && p_i->x < render_width_ && p_i->y == 0) { // Check if the two points are across the top left corner
                std::cout << "    Add top left corner\n";
                coastline_polygon.push_back({.x=0,.y=0});
                //coastline_polygon.push_back({.x=p_i->x,.y=p_i->y});
            } else if (p_im1->x > 0 && p_im1->x < render_width_ && p_im1->y == 0 && p_i->x == render_width_ && p_i->y > 0 && p_i->y < render_height_) { // Check if the two points are across the top right corner
                std::cout << "    Add top right corner\n";
                coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=0});
                //coastline_polygon.push_back({.x=p_i->x,.y=p_i->y});
            } else if (p_im1->x == render_width_ && p_im1->y > 0 && p_im1->y < render_height_ && p_i->x > 0 && p_i->x < render_width_ && p_i->y == render_height_) { // Check if the two points are across the bottom right corner
                std::cout << "    Add bottom right corner\n";
                coastline_polygon.push_back({.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)});
                //coastline_polygon.push_back({.x=p_i->x,.y=p_i->y});
            } else if (p_im1->x > 0 && p_im1->x < render_width_ && p_im1->y == render_height_ && p_i->x == 0 && p_i->y > 0 && p_i->y < render_height_) { // Check if the two points are across the bottom left corner
                std::cout << "    Add bottom left corner\n";
                coastline_polygon.push_back({.x=0,.y=static_cast<double>(render_height_)});
                //coastline_polygon.push_back({.x=p_i->x,.y=p_i->y});
            }
        }
         */
        if (path[i-1].point_type == PointType::kStart && path[i].point_type == PointType::kEnd) {
            std::vector<osm::Point<double>> corners = GetCornerPoints(path[i-1], path[i]);
            for (auto c_it = corners.begin(); c_it != corners.end(); ++c_it) {
                coastline_polygon.push_back(*c_it);
            }
        }

        // Add all points from the path to the polygon.
        // They're on different borders, so let's add the points in between them.
        // Always fill in the points from end to start to keep the flow clockwise.
        if (cp_im1 == cp_i && path[i-1].point_type == PointType::kStart && path[i].point_type == PointType::kEnd) {
            // Here we can assume that the point at position i is the ENDpoint of the path with the point at i-1 as STARTpoint.
            /*for (size_t p_idx=0; p_idx < bip_to_coastlinepath_[path[i]]->points.size(); ++p_idx) {
                coastline_polygon.push_back(bip_to_coastlinepath_[path[i]]->points[p_idx]);
            }*/
            for (auto it_pt = bip_to_coastlinepath_[path[i]]->points.rbegin(); it_pt != bip_to_coastlinepath_[path[i]]->points.rend(); ++it_pt) {
                coastline_polygon.push_back(*it_pt);
            }
        } else if (cp_im1 == cp_i && path[i-1].point_type == PointType::kEnd && path[i].point_type == PointType::kStart) {
            // Here we can assume that the point at position i is the STARTpoint of the path with the point at i as ENDpoint.
            /*for (size_t p_idx=0; p_idx < bip_to_coastlinepath_[path[i-1]]->points.size(); ++p_idx) {
                coastline_polygon.push_back(bip_to_coastlinepath_[path[i-1]]->points[p_idx]);
            }*/
            for (auto it_pt = bip_to_coastlinepath_[path[i-1]]->points.rbegin(); it_pt != bip_to_coastlinepath_[path[i-1]]->points.rend(); ++it_pt) {
                coastline_polygon.push_back(*it_pt);
            }
        }

        coastline_polygon.push_back({.x=p_i->x,.y=p_i->y});
    }

    if (bip_to_coastlinepath_[path[0]].get() == bip_to_coastlinepath_[path[path.size()-1]].get()) {
        auto points = (path[0].point_type == PointType::kStart && path[path.size()-1].point_type == PointType::kEnd)
                ? bip_to_coastlinepath_[path[path.size()-1]]->points : bip_to_coastlinepath_[path[0]]->points;
        /*for (size_t i = 0; i < points.size(); ++i) {
            coastline_polygon.push_back(points[i]);
        }*/
        for (auto it_pt = points.rbegin(); it_pt != points.rend(); ++it_pt) {
            coastline_polygon.push_back(*it_pt);
        }
    }

    return coastline_polygon;
}

std::vector<osm::Point<double>> OceanLandmassFactory::GetCornerPoints(BorderIntersectionPoint& start, BorderIntersectionPoint& end) {
    // I assume that start is really a startpoint, and end is really an endpoint.
    // If they're on the same border, then return an empty list of corner points.
    if (start.p->x == end.p->x || start.p->y == end.p->y) {
        // We can assume that the points are on the borders, so need to check for the respective
        // other coordinates.
        return {};
    }

    std::vector<osm::Point<double>> corners;
    osm::Point<double> point = *start.p;
    osm::Point<double> next_corner = GetNextCorner(point);
    while (!osm::utils::IsOnLine({.start=next_corner,.end=point}, *end.p)) {
        corners.push_back(next_corner);
        point = next_corner;
        next_corner = GetNextCorner(point);
    }

    return corners;
}

osm::Point<double> OceanLandmassFactory::GetNextCorner(osm::Point<double> p)
{
    if (p.x == 0 && p.y > 0 && p.y <= render_height_) return {.x=0,.y=0};
    else if (p.y == 0 && p.x >= 0 && p.x < render_width_) return {.x=static_cast<double>(render_width_),.y=0};
    else if (p.x == render_width_ && p.y >= 0 && p.y < render_height_) return {.x=static_cast<double>(render_width_),.y=static_cast<double>(render_height_)};
    else if (p.y == render_height_ && p.x > 0 && p.x <= render_width_) return {.x=0,.y=static_cast<double>(render_height_)};
}

void OceanLandmassFactory::FourthPass()
{
    // Now go over all the edges (boundaries) again, and connect startpoint and endpoints....
    std::queue<BorderIntersectionPoint> q;
    for (auto it = border_type_to_intersection_point_[BorderType::kTop].begin();
         it != border_type_to_intersection_point_[BorderType::kTop].end(); ++it) {
        q.push(*it);
    }
    for (auto it = border_type_to_intersection_point_[BorderType::kRight].begin();
         it != border_type_to_intersection_point_[BorderType::kRight].end(); ++it) {
        q.push(*it);
    }
    for (auto it = border_type_to_intersection_point_[BorderType::kBottom].begin();
         it != border_type_to_intersection_point_[BorderType::kBottom].end(); ++it) {
        q.push(*it);
    }
    for (auto it = border_type_to_intersection_point_[BorderType::kLeft].begin();
         it != border_type_to_intersection_point_[BorderType::kLeft].end(); ++it) {
        q.push(*it);
    }

    if (q.empty()) {
        return;
    }

    if (q.front().point_type == PointType::kEnd) {
        auto pt = q.front();
        q.pop();
        q.push(pt);
    }

    std::stack<std::vector<BorderIntersectionPoint>> s;
    while (!q.empty()) {
        auto bip = q.front();

        if (bip.point_type == PointType::kStart) {
            if (s.size() > 0) {
                if (s.top().size() > 0) { // Should always be true
                    //if (bip.w == p.back().w
                    if (bip_to_coastlinepath_[bip] == bip_to_coastlinepath_[s.top().back()]
                        && s.top().back().point_type == PointType::kEnd) {
                        // If the coastline path of the next in the queue is the same as the last
                        // one added to the stack (which was an endpoint), then add the current bip
                        // (which is a startpoint) to the stack.
                        s.top().push_back(bip);
                    } else {
                        // In this case the startpoint belongs to a new path, and we add it to the stack.
                        s.push({});
                        s.top().push_back(bip);
                    }
                }
            } else {
                s.push({});
                s.top().push_back(bip);
            }
        } else if (bip.point_type == PointType::kEnd) {
            if (s.size() > 0) {
                if (s.top().size() > 0) { // Should always be true
                    //if (bip.w == p.back().w
                    if (bip_to_coastlinepath_[bip] == bip_to_coastlinepath_[s.top().front()]
                        && s.top().front().point_type == PointType::kStart) {
                        s.top().push_back(bip);
                        coastline_polygons_.insert(CloseCoastlinePath(s.top()));
                        s.pop();
                    } else {
                        s.top().push_back(bip);
                    }
                }
            }
        }

        q.pop();
    }
}

void OceanLandmassFactory::GetClosestPoint(const Way* const way)
{

}

double OceanLandmassFactory::Distance(double a_lat, double a_lon, double b_lat, double b_lon)
{
    return osm::utils::HaversineKm(a_lat, a_lon, b_lat, b_lon);
}

std::shared_ptr<Point<double>> OceanLandmassFactory::Intersection(const Line<double>& a, const Line<double>& b)
{
    double a1 = a.end.y - a.start.y;
    double b1 = a.start.x - a.end.x;
    double c1 = a1 * a.start.x + b1 * a.start.y;

    double a2 = b.end.y - b.start.y;
    double b2 = b.start.x - b.end.x;
    double c2 = a2 * b.start.x + b2 * b.start.y;

    double determinant = a1 * b2 - a2 * b1;

    if (determinant == 0) {
        // Lines are parallel.
        return nullptr;
    }

    std::shared_ptr<Point<double>> c_out = std::make_shared<Point<double>>();
    c_out->x = (b2 * c1 - b1 * c2) / determinant;
    c_out->y = (a1 * c2 - a2 * c1) / determinant;

    // Ok, now we're sure that there is an intersection point - somewhere.
    // Let's see if it is within the start- and endpoints of both lines.
    if (osm::utils::IsOnLine(a, *c_out) && osm::utils::IsOnLine(b, *c_out)) {
        return c_out;
    }

    return nullptr;
}

}  // namespace osm
