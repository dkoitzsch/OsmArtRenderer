#ifndef OSMPARSER_H
#define OSMPARSER_H

#include "mapdata.h"

#include <functional>
#include <QList>
#include <QString>
#include <QXmlStreamReader>


namespace osm {

class Parser
{
public:
    Parser();
    // Throws an std::logic_error exception in case of any error.
    MapData* Parse(QString filename, std::function<void(Way*)> grabber = nullptr);

private:
    void HandleNode(QXmlStreamReader& reader, MapData* osm_map_data);
    Way* HandleWay(QXmlStreamReader& reader, MapData* osm_map_data);
    void HandleBounds(const QXmlStreamReader& reader, MapData* osm_map_data);
    QString ReadAttr(const QXmlStreamReader& reader, QString attr_name);
};

}  // namespace osm

#endif // OSMPARSER_H
