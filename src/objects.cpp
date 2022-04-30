#include "objects.h"

namespace osm {

Objects::Objects(const std::string& name, const std::vector<MetaTag>& validTags, ObjectsTypes type)
    : name_(name), validTagTypes_(validTags), type_(type)
{
}

Objects::~Objects()
{

}

bool Objects::Grab(Way* way)
{
    for (auto it = way->tags.begin(); it != way->tags.end(); ++it) {
        for (auto itValidTags = validTagTypes_.begin(); itValidTags != validTagTypes_.end(); ++itValidTags) {
            if (**it == *itValidTags) {
                ways_.push_back(way);
                return true;
            }
        }
    }
    return false;
}

int Objects::Size()
{
    return ways_.size();
}

std::vector<Way*>* Objects::Ways()
{
    return &ways_;
}

void Objects::Clear()
{
    ways_.clear();
}

}  // namespace osm
