#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "canvas.h"
#include "canvaspietmondrien.h"
#include "objectsrepository.h"
#include "parser.h"
#include "watercoloreffect.h"

#include <QHash>
#include <QImage>
#include <QList>
#include <QListWidget>
#include <QMainWindow>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void keyReleaseEvent(QKeyEvent*);

private:
    void SetupUi();
    void SetupObjectsRepository();
    void SetupWatercolorEffect();
    void OpenFile();
    void RenderMap(osm::Canvas* canvas);
    void RenderEffect(effects::Effect* effect);
    //void Save();
    osm::Canvas* ChangeCanvasTo(osm::Canvas* canvas);
    void ObjectsListSelected();
    void ObjectsConfigUpdated();
    void WatercolorEffectConfigUpdated();

    void GenerateCoastlines();

    //QOpenGLWidget* debug_widget_;
    QHash<QString, effects::WatercolorEffectConfiguration*> watercolor_effect_configurations_;
    effects::WatercolorEffect* watercolor_effect_;
    osm::Canvas* current_canvas_;
    osm::Canvas* canvas_;
    osm::CanvasPietMondrien* canvas_pm_;
    QList<osm::Canvas*> canvas_list_;
    QVBoxLayout* canvas_container_;
    osm::Parser parser_;
    osm::MapData* map_data_;
    osm::ObjectsRepository objects_repository_;
    QListWidget* objects_list_;
    QVBoxLayout* objects_config_layout_;
    QVBoxLayout* watercolor_effect_config_layout_;
    QTabWidget* objects_config_tabs_;
    QWidget* current_config_widget_;

    enum RenderStatus { NONE, MAP, EFFECT };
    RenderStatus render_status_;
};

#endif // MAINWINDOW_H
