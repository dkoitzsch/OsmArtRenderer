#include "objectsrepository.h"

#include <QException>

namespace osm {

void ObjectsRepository::AddObjects(QString name, osm::Objects* objects)
{
    if (objects_by_name_.find(name) == objects_by_name_.end()) {
        ordered_objectsname_.push_back(name);
    }
    objects_by_name_[name] = objects;
}

Objects* ObjectsRepository::Objects(QString name)
{
    if (objects_by_name_.find(name) == objects_by_name_.end()) {
        return nullptr;
    }
    return objects_by_name_[name];
}

bool ObjectsRepository::SetObjectsOrder(QVector<QString> order)
{
    for (auto it = order.begin(); it != order.end(); ++it) {
        if (objects_by_name_.find(*it) == objects_by_name_.end()) {
            //throw ObjectsNotFound(*it);
            return false;
        }
    }
    ordered_objectsname_ = order;
    return true;
}

osm::ObjectsConfiguration* ObjectsRepository::ObjectsConfiguration(QString name)
{
    if (objects_configuration_.find(name) == objects_configuration_.end()) {
        //throw ObjectsConfigurationNotFound(name);
        return nullptr;
    }
    return objects_configuration_[name];
}

void ObjectsRepository::ObjectsConfiguration(QString name, osm::ObjectsConfiguration* config)
{
    objects_configuration_[name] = config;
}

void ObjectsRepository::Clear()
{
    ordered_objectsname_.clear();
    qDeleteAll(objects_by_name_);
    objects_by_name_.clear();
    qDeleteAll(objects_configuration_);
    objects_configuration_.clear();
}

int ObjectsRepository::Size()
{
    return objects_by_name_.size();
}

}  // namespace osm
