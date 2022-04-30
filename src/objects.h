#ifndef OBJECT_H
#define OBJECT_H

#include "types.h"

namespace osm
{

enum ObjectsTypes {ANY, OCEAN, LANDMASS};

class Objects
{
public:
    explicit Objects() = default;
    Objects(const std::string& name, const std::vector<MetaTag>& validTags, ObjectsTypes type = ObjectsTypes::ANY);
    virtual ~Objects();

    ObjectsTypes ObjectsType() { return type_; }

    bool Grab(Way* way);
    int Size();
    std::vector<Way*>* Ways();
    void Clear();

    // For testing with coastlines only:
    void Ways(std::vector<Way*> ways) {this->ways_ = ways;}

    std::string const& Name() const { return name_; }
    void Name(std::string const& name) { name_ = name; }
    std::vector<MetaTag> const& ValidTagTypes() const { return validTagTypes_; }
    void ValidTagTypes(std::vector<MetaTag> const& validTagTypes) { validTagTypes_ = validTagTypes; }

protected:
    std::string name_;
    std::vector<MetaTag> validTagTypes_;
    std::vector<Way*> ways_;
    ObjectsTypes type_;
};

} // namespace osm

#endif // OBJECT_H
