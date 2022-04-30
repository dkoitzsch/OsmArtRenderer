#ifndef OSMTYPES_H
#define OSMTYPES_H

#include <string>
#include <vector>

namespace osm {

struct Tag {
    std::string key;
    std::string value;

    bool operator==(const Tag& rhs) const {
        if (rhs.key == key) {
            if (rhs.value == "*" || value == "*" || rhs.value == value) {
                return true;
            }
            if ((rhs.value[0] == '!' && rhs.value.substr(1) == value) || (value[0] == '!' && rhs.value == value.substr(1))) {
                return false;
            }
        }
        return false;
    }

    bool operator!=(const Tag& rhs) const {
        if (rhs.key != key) {
            return true;
        }
        if (rhs.value == "*") {
            return false;
        }
        if (value == "*") {
            return false;
        }
        if (rhs.value != value) {
            return true;
        }
        return false;
    }
};
typedef Tag MetaTag;

struct Node {
    std::string id;
    float lat, lon;
    float x, y;
    std::vector<Tag*> tags;
};

struct Way {
    std::string id;
    std::vector<Node*> nodes;
    std::vector<Tag*> tags;
    bool is_closed;
};

template <typename T>
struct Point {
    union {
        T x;
        T lon;
    };
    union {
        T y;
        T lat;
    };

    bool operator<(const Point& p)  const {
        return (x < p.x) || ((x == p.x) && (y < p.y));
    }

    bool operator==(const Point& p)  const {
        return p.x == x && p.y == y;
    }
};
// Note: See below for the declaration of: namespace std { template <typename T> struct hash<osm::Point<T>>...
template <typename T>
class PointHashFunction {
public:
    size_t operator()(const Point<T>& p) const
    {
        return ((uint64_t)p.x)<<32 | (uint64_t)p.y;
    }
};

template <typename T>
struct Line {
    Point<T> start;
    Point<T> end;
};

struct BoundingBox {
    float min_lat;
    float max_lat;
    float min_lon;
    float max_lon;
};

struct Transformation
{
    double scale;
    double translate_x;
    double translate_y;
};

} // namespace osm

namespace std {
template <typename T>
struct hash<osm::Point<T>>
{
    size_t operator() (const osm::Point<T> &p) const
    {
        //return size_t(p);
        return ((uint64_t)p.x)<<32 | (uint64_t)p.y;
    }
};
}

#endif // OSMTYPES_H
