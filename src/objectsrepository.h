#ifndef OBJECTSCOLLECTION_H
#define OBJECTSCOLLECTION_H

#include "objects.h"
#include "objectsconfiguration.h"

#include <QException>
#include <QHash>
#include <QString>
#include <QVector>

namespace osm {

class ObjectsRepository
{
public:
    explicit ObjectsRepository() = default;

    void AddObjects(QString name, Objects* objects);
    Objects* Objects(QString name);
    bool SetObjectsOrder(QVector<QString> order);
    QVector<QString>* OrderedObjectsNames() { return &ordered_objectsname_; }
    osm::ObjectsConfiguration* ObjectsConfiguration(QString name);
    void ObjectsConfiguration(QString name, osm::ObjectsConfiguration* config);

    void Clear();
    int Size();

private:
    QVector<QString> ordered_objectsname_;
    QHash<QString, osm::Objects*> objects_by_name_;
    QHash<QString, osm::ObjectsConfiguration*> objects_configuration_;
};

class ObjectsNotFound : QException
{
public:
    explicit ObjectsNotFound() = default;
    explicit ObjectsNotFound(QString message) {this->message_ = message;}
    void raise() const override { throw *this; }
    ObjectsNotFound *clone() const override { return new ObjectsNotFound(*this); }
    const QString Message() const {return message_;}
private:
    QString message_;
};

class ObjectsConfigurationNotFound : QException
{
public:
    explicit ObjectsConfigurationNotFound() = default;
    explicit ObjectsConfigurationNotFound(QString message) {this->message_ = message;}
    void raise() const override { throw *this; }
    ObjectsConfigurationNotFound *clone() const override { return new ObjectsConfigurationNotFound(*this); }
    const QString Message() const {return message_;}
private:
    QString message_;
};


} // namespace osm

#endif // OBJECTSCOLLECTION_H
