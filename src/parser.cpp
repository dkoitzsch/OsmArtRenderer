#include "parser.h"
#include "mapdata.h"
#include "types.h"

#include <QFile>
#include <QList>
#include <QPointF>
#include <QXmlStreamReader>

#include <stdexcept>
#include <string>

namespace osm {

enum State {
    NODE, WAY, NONE
};

Parser::Parser()
{

}

MapData* Parser::Parse(QString filename, std::function<void(Way*)> grabber)
{
    MapData* osm_map_data = new MapData();
    osm_map_data->min_xy_.x = -1;
    osm_map_data->min_xy_.y = -1;
    osm_map_data->max_xy_.x = -1;
    osm_map_data->max_xy_.y = -1;

    QFile xml_file(filename);
    if (!xml_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::logic_error("Could not open OSM data file \"" + filename.toStdString() + "\".");
    }
    QXmlStreamReader xml_reader(&xml_file);

    while(!xml_reader.atEnd() && !xml_reader.hasError()) {
        QXmlStreamReader::TokenType token = xml_reader.readNext();
        if(token == QXmlStreamReader::StartDocument) {
            continue;
        }
        if(token == QXmlStreamReader::StartElement) {
            if (xml_reader.name() == "bounds") {
                HandleBounds(xml_reader, osm_map_data);
            } else if (xml_reader.name() == "node") {
                HandleNode(xml_reader, osm_map_data);
            } else if (xml_reader.name() == "way") {
                Way* way = HandleWay(xml_reader, osm_map_data);
                if (grabber) {
                    grabber(way);
                }
            }
        }
    }

    if(xml_reader.hasError()) {
        throw std::logic_error("Error while parsing \"" + filename.toStdString()
                               + "\": " + xml_reader.errorString().toStdString());
    }

    // Close reader and flush file
    xml_reader.clear();
    xml_file.close();

    return osm_map_data;
}

void Parser::HandleNode(QXmlStreamReader& reader, MapData* osm_map_data)
{
    Node* node = new Node();
    node->id = ReadAttr(reader, "id").toStdString();
    node->lat = ReadAttr(reader, "lat").toFloat();
    node->lon = ReadAttr(reader, "lon").toFloat();
    osm_map_data->nodes_[node->id] = node;
    osm_map_data->nodes_for_deletion_.push_back(node);

    // Iterate child xml nodes.
    while(!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if(token == QXmlStreamReader::EndElement && reader.name() == "node") {
            return;
        } else if (token == QXmlStreamReader::StartElement && reader.name() == "tag") {
            Tag* tag = new Tag;
            tag->key = ReadAttr(reader, "k").toStdString();
            tag->value = ReadAttr(reader, "v").toStdString();
            node->tags.push_back(tag);
            osm_map_data->tags_for_deletion_.push_back(tag);
        }
    }
}

Way* Parser::HandleWay(QXmlStreamReader& reader, MapData* map_data)
{
    Way* way = new Way();
    way->id = ReadAttr(reader, "id").toStdString();
    map_data->ways_.push_back(way);

    // Iterate child xml nodes.
    while(!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if(token == QXmlStreamReader::EndElement && reader.name() == "way") {
            return way;
        } else if (token == QXmlStreamReader::StartElement && reader.name() == "tag") {
            Tag* tag = new Tag;
            tag->key = ReadAttr(reader, "k").toStdString();
            tag->value = ReadAttr(reader, "v").toStdString();
            way->tags.push_back(tag);
            map_data->tags_for_deletion_.push_back(tag);
        } else if (token == QXmlStreamReader::StartElement && reader.name() == "nd") {
            auto nodeRef = map_data->nodes_[ReadAttr(reader, "ref").toStdString()];
            way->nodes.push_back(nodeRef);
            if (way->nodes.size() > 1 && way->nodes[0] != nullptr && (way->nodes[0]->id == nodeRef->id)) {
                way->is_closed = true;
            }
        }
    }
    return way;
}

void Parser::HandleBounds(const QXmlStreamReader& reader, MapData* map_data)
{
    map_data->min_lon_ = ReadAttr(reader, "minlon").toFloat();
    map_data->max_lon_ = ReadAttr(reader, "maxlon").toFloat();
    map_data->min_lat_ = ReadAttr(reader, "minlat").toFloat();
    map_data->max_lat_ = ReadAttr(reader, "maxlat").toFloat();
}

QString Parser::ReadAttr(const QXmlStreamReader& reader, QString attr_name)
{
    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
        if (attr.name().toString() == attr_name) {
            QString attribute_value = attr.value().toString();
            return attribute_value;
        }
    }
    return "";
}

}  // namespace osm
