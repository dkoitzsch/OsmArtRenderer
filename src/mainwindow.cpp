#include "mainwindow.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "constants.h"
#include "oceanlandmassfactory.h"
#include "utils.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      watercolor_effect_(nullptr),
      current_canvas_(nullptr),
      canvas_(nullptr),
      canvas_pm_(nullptr),
      map_data_(nullptr),
      current_config_widget_(nullptr),
      render_status_(RenderStatus::NONE) {
  SetupUi();
  SetupObjectsRepository();
  SetupWatercolorEffect();
}

MainWindow::~MainWindow() {
  delete canvas_;
  delete canvas_pm_;
  if (map_data_ != nullptr) {
    delete map_data_;
    map_data_ = nullptr;
  }

  for (auto it = watercolor_effect_configurations_.begin();
       it != watercolor_effect_configurations_.end(); ++it) {
    delete it.value();
  }
  delete watercolor_effect_;
  delete watercolor_effect_config_layout_;
  delete objects_config_layout_;
  delete objects_list_;
  delete objects_config_tabs_;
}

void MainWindow::keyReleaseEvent(QKeyEvent* ke) {
  if (ke->key() == Qt::Key_Escape) {
    ke->accept();
    this->close();
    return;
  } else {
    // Allow for the default handling of the key
    QMainWindow::keyReleaseEvent(ke);  // aka the base class implementation
  }
}

void MainWindow::SetupUi() {
  QPushButton* btn_open = new QPushButton(tr("Open OSM File"));
  QObject::connect(btn_open, &QPushButton::clicked, this,
                   &MainWindow::OpenFile);
  QPushButton* btn_render_bw = new QPushButton(tr("Render B/W Map"));
  QObject::connect(btn_render_bw, &QPushButton::clicked, this, [this] {
    RenderMap(ChangeCanvasTo(
        canvas_));  // Think about passing the canvas as an argument.
  });
  QPushButton* btn_render_pm = new QPushButton(tr("Render P/M Map"));
  QObject::connect(btn_render_pm, &QPushButton::clicked, this,
                   [this] { RenderMap(ChangeCanvasTo(canvas_pm_)); });
  QPushButton* btn_render_watercolor_effect =
      new QPushButton(tr("Render Watercolor Effect"));
  QObject::connect(btn_render_watercolor_effect, &QPushButton::clicked, this,
                   [this] {
                     ChangeCanvasTo(canvas_);
                     RenderEffect(watercolor_effect_);
                   });

  QVBoxLayout* button_layout = new QVBoxLayout();
  button_layout->addWidget(btn_open);
  button_layout->addWidget(btn_render_bw);
  button_layout->addWidget(btn_render_pm);
  button_layout->addWidget(btn_render_watercolor_effect);
  canvas_ = new osm::Canvas(300, 300, &objects_repository_);  //, this);
  canvas_list_.push_back(canvas_);
  canvas_container_ = new QVBoxLayout;
  canvas_container_->addWidget(canvas_);
  button_layout->addLayout(canvas_container_);
  // debug_widget_ = new QOpenGLWidget();
  // debug_widget_->setFixedSize(300,300);
  // canvas_container_->addWidget(debug_widget_);

  QHBoxLayout* main_layout = new QHBoxLayout;
  main_layout->addLayout(button_layout);

  // ****** Set up the configuration widgets *********
  QWidget* watercolor_effect_config_widget = new QWidget;
  QWidget* objects_config_widget = new QWidget;
  watercolor_effect_config_layout_ = new QVBoxLayout;
  objects_config_layout_ = new QVBoxLayout;
  watercolor_effect_config_widget->setLayout(watercolor_effect_config_layout_);
  objects_config_widget->setLayout(objects_config_layout_);
  QGridLayout* conf_layout = new QGridLayout;
  objects_config_tabs_ = new QTabWidget;
  objects_config_tabs_->addTab(objects_config_widget, "Objects Configuration");
  objects_config_tabs_->addTab(watercolor_effect_config_widget,
                               "Watercolor Effect Configuration");
  QObject::connect(objects_config_tabs_, &QTabWidget::currentChanged, this,
                   [this] { this->ObjectsListSelected(); });
  objects_list_ = new QListWidget;
  objects_list_->addItem(osm::kBuildingsName);
  objects_list_->addItem(osm::kHighwaysName);
  objects_list_->addItem(osm::kHighwaysExtName);
  objects_list_->addItem(osm::kWaterwaysName);
  objects_list_->addItem(osm::kGreenlandName);
  objects_list_->addItem(osm::kDevelopedLandName);
  objects_list_->addItem(osm::kOceanName);
  objects_list_->addItem(osm::kLandmassName);
  QObject::connect(objects_list_, &QListWidget::itemSelectionChanged, this,
                   &MainWindow::ObjectsListSelected);
  objects_list_->setFixedWidth(200);
  conf_layout->addWidget(objects_list_, 0, 0, /*rowSpan=*/1, /*columnSpan=*/1);
  conf_layout->addWidget(objects_config_tabs_, 0, 1, /*rowSpan=*/1,
                         /*columnSpan=*/3);
  main_layout->addLayout(conf_layout);

  QWidget* placeholder_widget = new QWidget();
  placeholder_widget->setLayout(main_layout);
  setCentralWidget(placeholder_widget);

  canvas_pm_ = new osm::CanvasPietMondrien(300, 300, &objects_repository_);
  canvas_list_.push_back(canvas_pm_);
  current_canvas_ = canvas_;
}

void MainWindow::SetupObjectsRepository() {
  objects_repository_.AddObjects(
      osm::kBuildingsName,
      new osm::Objects(osm::kBuildingsName,
                       {{.key = "building", .value = "*"}}));
  objects_repository_.AddObjects(
      osm::kDevelopedLandName,
      new osm::Objects(osm::kDevelopedLandName,
                       {{.key = "landuse", .value = "*"},
                        {.key = "landuse", .value = "!basin"},
                        {.key = "landuse", .value = "!reservoir"},
                        {.key = "natural", .value = "land"}}));
  objects_repository_.AddObjects(
      osm::kHighwaysName,
      new osm::Objects(osm::kHighwaysName, {{.key = "highway", .value = "*"}}));
  objects_repository_.AddObjects(
      osm::kHighwaysExtName,
      new osm::Objects(osm::kHighwaysExtName,
                       {{.key = "highway", .value = "primary"},
                        {.key = "highway", .value = "secondary"},
                        {.key = "highway", .value = "motorway"},
                        {.key = "highway", .value = "trunk"},
                        {.key = "highway", .value = "tertiary"},
                        {.key = "highway", .value = "primary_link"},
                        {.key = "highway", .value = "secondary_link"},
                        {.key = "highway", .value = "motorway_link"},
                        {.key = "highway", .value = "trunk_link"},
                        {.key = "highway", .value = "motorway_junction"}}));
  objects_repository_.AddObjects(
      osm::kWaterwaysName,
      new osm::Objects(osm::kWaterwaysName,
                       {{.key = "waterway", .value = "*"},
                        {.key = "natural", .value = "water"},
                        {.key = "natural", .value = "wetland"},
                        {.key = "natural", .value = "bay"},
                        {.key = "natural", .value = "strait"},
                        {.key = "place", .value = "ocean"},
                        {.key = "place", .value = "sea"}}));
  objects_repository_.AddObjects(
      osm::kGreenlandName,
      new osm::Objects(osm::kGreenlandName,
                       {{.key = "landuse", .value = "allotments"},
                        {.key = "landuse", .value = "farmland"},
                        {.key = "landuse", .value = "farmyard"},
                        {.key = "landuse", .value = "forest"},
                        {.key = "landuse", .value = "meadow"},
                        {.key = "landuse", .value = "orchard"},
                        {.key = "landuse", .value = "vineyard"},
                        {.key = "landuse", .value = "grass"},
                        {.key = "landuse", .value = "greenfield"},
                        {.key = "landuse", .value = "plant_nursery"},
                        {.key = "landuse", .value = "village_green"},
                        {.key = "leisure", .value = "leisure"},
                        {.key = "leisure", .value = "golf_course"},
                        {.key = "leisure", .value = "miniature_golf"},
                        {.key = "leisure", .value = "nature_preserve"},
                        {.key = "leisure", .value = "park"},
                        {.key = "natural", .value = "scrub"},
                        {.key = "natural", .value = "wood"},
                        {.key = "natural", .value = "grassland"},
                        {.key = "natural", .value = "heath"},
                        {.key = "natural", .value = "tree"},
                        {.key = "natural", .value = "tree_row"},
                        {.key = "wetland", .value = "marsh"},
                        {.key = "wetland", .value = "reedbed"},
                        {.key = "wetland", .value = "saltmarsh"},
                        {.key = "wetland", .value = "wet_meadow"},
                        {.key = "wetland", .value = "swamp"},
                        {.key = "wetland", .value = "mangrove"},
                        {.key = "wetland", .value = "bog"},
                        {.key = "wetland", .value = "fen"}}));
  objects_repository_.AddObjects(
      osm::kCoastlinesName,
      new osm::Objects(osm::kCoastlinesName,
                       {{.key = "natural", .value = "coastline"}}));

  objects_repository_.ObjectsConfiguration(
      osm::kBuildingsName,
      new osm::ObjectsConfiguration(true, 1, Qt::black, true, Qt::black, true));
  objects_repository_.ObjectsConfiguration(
      osm::kHighwaysName,
      new osm::ObjectsConfiguration(true, 4, Qt::black, true, Qt::black, true));
  objects_repository_.ObjectsConfiguration(
      osm::kHighwaysExtName,
      new osm::ObjectsConfiguration(true, 5, Qt::black, true, Qt::black, true));
  objects_repository_.ObjectsConfiguration(
      osm::kWaterwaysName,
      new osm::ObjectsConfiguration(true, 4, Qt::black, true, Qt::black, true));
  objects_repository_.ObjectsConfiguration(
      osm::kDevelopedLandName,
      new osm::ObjectsConfiguration(true, 1, QColor(230, 230, 230), true,
                                    QColor(230, 230, 230), true));
  objects_repository_.ObjectsConfiguration(
      osm::kGreenlandName,
      new osm::ObjectsConfiguration(true, 1, QColor(200, 200, 200), true,
                                    QColor(200, 200, 200), true));
  objects_repository_.ObjectsConfiguration(
      osm::kOceanName,
      new osm::ObjectsConfiguration(true, 1, Qt::black, true, Qt::black, true));
  objects_repository_.ObjectsConfiguration(
      osm::kLandmassName,
      new osm::ObjectsConfiguration(true, 1, QColor(250, 250, 250), true,
                                    QColor(250, 250, 250), true));

  /*QObject::connect(objects_repository_.ObjectsConfiguration(osm::kBuildingsName),
  &osm::ObjectsConfiguration::Updated, this, &MainWindow::ObjectsConfigUpdated);
  QObject::connect(objects_repository_.ObjectsConfiguration(osm::kHighwaysName),
  &osm::ObjectsConfiguration::Updated, this, &MainWindow::ObjectsConfigUpdated);
  QObject::connect(objects_repository_.ObjectsConfiguration(osm::kHighwaysExtName),
  &osm::ObjectsConfiguration::Updated, this, &MainWindow::ObjectsConfigUpdated);
  QObject::connect(objects_repository_.ObjectsConfiguration(osm::kWaterwaysName),
  &osm::ObjectsConfiguration::Updated, this, &MainWindow::ObjectsConfigUpdated);
  QObject::connect(objects_repository_.ObjectsConfiguration(osm::kOceanName),
  &osm::ObjectsConfiguration::Updated, this, &MainWindow::ObjectsConfigUpdated);
  QObject::connect(objects_repository_.ObjectsConfiguration(osm::kLandmassName),
  &osm::ObjectsConfiguration::Updated, this,
  &MainWindow::ObjectsConfigUpdated);*/
}

void MainWindow::SetupWatercolorEffect() {
  double noise_scale_factor = 50.0;
  double final_blend_alpha = 0.8;
  double threshold_combiner_alpha = 0.7;
  double threshold_combiner_threshold = 0.85;

  // Note: Noisescale was 1 for all except 5 for developed land and greenland
  // initially. But I think 0.2 is nicer.
  watercolor_effect_configurations_[osm::kBuildingsName] =
      new effects::WatercolorEffectConfiguration(
          osm::kBuildingsName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/brown_42.jpg"));
  watercolor_effect_configurations_[osm::kHighwaysName] =
      new effects::WatercolorEffectConfiguration(
          osm::kHighwaysName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/orange_11.jpg"));
  watercolor_effect_configurations_[osm::kHighwaysExtName] =
      new effects::WatercolorEffectConfiguration(
          osm::kHighwaysExtName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/orange_10.jpg"));
  watercolor_effect_configurations_[osm::kWaterwaysName] =
      new effects::WatercolorEffectConfiguration(
          osm::kWaterwaysName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/blue_24.jpg"));

  watercolor_effect_configurations_[osm::kDevelopedLandName] =
      new effects::WatercolorEffectConfiguration(
          osm::kDevelopedLandName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/gray_04.jpg"));
  watercolor_effect_configurations_[osm::kGreenlandName] =
      new effects::WatercolorEffectConfiguration(
          osm::kGreenlandName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/green_09.jpg"));

  watercolor_effect_configurations_[osm::kOceanName] =
      new effects::WatercolorEffectConfiguration(
          osm::kOceanName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/blue_01.jpg"));
  watercolor_effect_configurations_[osm::kLandmassName] =
      new effects::WatercolorEffectConfiguration(
          osm::kLandmassName, noise_scale_factor, final_blend_alpha,
          threshold_combiner_alpha, threshold_combiner_threshold,
          QImage(":/textures/brown_40.jpg"));

  watercolor_effect_ =
      new effects::WatercolorEffect(watercolor_effect_configurations_);
}

void MainWindow::OpenFile() {
  const QString homefolder =
      QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString filename = QFileDialog::getOpenFileName(
      this, tr("Open OSM File"), homefolder, tr("OpenStreetMap Files (*.osm)"));
  if (!filename.isEmpty()) {
    for (auto it = objects_repository_.OrderedObjectsNames()->begin();
         it != objects_repository_.OrderedObjectsNames()->end(); ++it) {
      objects_repository_.Objects(*it)->Clear();
    }

    if (map_data_ != nullptr) {
      delete map_data_;
    }

    map_data_ = parser_.Parse(filename, [&](osm::Way* way) {
      for (auto it = objects_repository_.OrderedObjectsNames()->begin();
           it != objects_repository_.OrderedObjectsNames()->end(); ++it) {
        objects_repository_.Objects(*it)->Grab(way);
      }
    });

    for (auto it = canvas_list_.begin(); it != canvas_list_.end(); ++it) {
      (*it)->MapData(map_data_);
      (*it)->ResetTransformation();  // Not sure if I really want this here...
    }

    GenerateCoastlines();

    // try {
    if (!objects_repository_.SetObjectsOrder(
            {osm::kOceanName, osm::kLandmassName, osm::kDevelopedLandName,
             osm::kGreenlandName, osm::kCoastlinesName, osm::kWaterwaysName,
             osm::kHighwaysName, osm::kHighwaysExtName, osm::kBuildingsName})) {
      qDebug() << "Some objects are missing. Order could not be set.";
    }
    //} catch (const osm::ObjectsNotFound& e) {
    // qDebug() << "Objects with name " << e.Message() << "not found";
    //}
  }
}

void MainWindow::RenderMap(osm::Canvas* canvas) {
  // canvas_->enable_all_collections();

  if (objects_repository_.Size() == 0 || map_data_ == nullptr) {
    return;
  }

  float a =
      osm::utils::GetAspectRatio(map_data_->MinLat(), map_data_->MaxLat(),
                                 map_data_->MinLon(), map_data_->MaxLon());
  canvas->setFixedSize(osm::Canvas::kMaxHeight * a, osm::Canvas::kMaxHeight);
  QImage image = canvas->RenderToImage();
  canvas->ShowImage(image);

  render_status_ = RenderStatus::MAP;
}

void MainWindow::RenderEffect(effects::Effect* effect) {
  if (current_canvas_ != canvas_) {
    return;
  }

  float a =
      osm::utils::GetAspectRatio(map_data_->MinLat(), map_data_->MaxLat(),
                                 map_data_->MinLon(), map_data_->MaxLon());
  current_canvas_->setFixedSize(osm::Canvas::kMaxHeight * a,
                                osm::Canvas::kMaxHeight);

  QImage effect_img = effect->Apply(current_canvas_, true);
  current_canvas_->ShowImage(effect_img);

  render_status_ = RenderStatus::EFFECT;
}

osm::Canvas* MainWindow::ChangeCanvasTo(osm::Canvas* canvas) {
  if (current_canvas_ == nullptr) {
    current_canvas_ = canvas;
    return canvas;
  }
  current_canvas_->hide();
  current_canvas_->setParent(nullptr);
  canvas_container_->removeWidget(current_canvas_);
  current_canvas_ = canvas;
  canvas_container_->addWidget(current_canvas_);
  current_canvas_->setParent(canvas_container_->parentWidget());
  current_canvas_->show();
  return canvas;
}

void MainWindow::ObjectsListSelected() {
  if (objects_list_->selectedItems().size() == 0) {
    return;
  }

  QListWidgetItem* item = objects_list_->selectedItems()[0];
  if (current_config_widget_) {
    current_config_widget_->setParent(nullptr);
    // objects_config_layout_->removeWidget(current_config_widget_);
  }

  if (objects_config_tabs_->currentIndex() == 0) {
    if (objects_repository_.ObjectsConfiguration(item->text())) {
      objects_config_layout_->removeWidget(current_config_widget_);
      current_config_widget_ =
          objects_repository_.ObjectsConfiguration(item->text())->Widget();
      objects_config_layout_->addWidget(current_config_widget_);
    } else {
      qDebug() << "No configuration found for" << item->text();
      return;
    }
  } else if (objects_config_tabs_->currentIndex() == 1) {
    if (objects_repository_.ObjectsConfiguration(item->text())) {
      watercolor_effect_config_layout_->removeWidget(current_config_widget_);
      current_config_widget_ =
          watercolor_effect_configurations_[item->text()]->Widget();
      watercolor_effect_config_layout_->addWidget(current_config_widget_);
    } else {
      qDebug() << "No watercolor effect configuration found for"
               << item->text();
      return;
    }
  }

  // if (current_config_widget_) {
  //     objects_config_layout_->addWidget(current_config_widget_);
  //     current_config_widget_->setParent(objects_config_layout_->parentWidget());
  // }
}

void MainWindow::ObjectsConfigUpdated() {
  if (current_canvas_ && render_status_ == RenderStatus::MAP) {
    RenderMap(current_canvas_);
  } else if (render_status_ == RenderStatus::EFFECT) {
    RenderEffect(watercolor_effect_);
  }
}

void MainWindow::WatercolorEffectConfigUpdated() {}

void MainWindow::GenerateCoastlines() {
  float a =
      osm::utils::GetAspectRatio(map_data_->MinLat(), map_data_->MaxLat(),
                                 map_data_->MinLon(), map_data_->MaxLon());
  int w = osm::Canvas::kMaxHeight * a;
  int h = osm::Canvas::kMaxHeight;

  qDebug() << "Start generating ocean and landmass polygons...";
  QElapsedTimer timer;
  timer.start();
  // auto start = std::chrono::high_resolution_clock::now();
  osm::Objects* coastlines = objects_repository_.Objects(osm::kCoastlinesName);
  float scale = 1.0f;
  int diff_w = (w - w * scale) / 2.0f;
  int diff_h = (h - h * scale) / 2.0f;
  osm::OceanLandmassFactory factory(coastlines, w, h, scale,
                                    {.x = diff_w, .y = diff_h},
                                    {.min_lat = map_data_->MinLat(),
                                     .max_lat = map_data_->MaxLat(),
                                     .min_lon = map_data_->MinLon(),
                                     .max_lon = map_data_->MaxLon()});
  factory.Build();
  auto coastline_polygons = factory.CoastlinePolygon();

  /*debug_widget_->setFixedSize(w, h);
  QPainter painter(debug_widget_);
  QPen p(QColor(255,0,0));
  p.setWidth(3);
  QPen p2(QColor(0,255,0));
  p2.setWidth(1);
  QPen p3(QColor(0,255,255));
  p3.setWidth(1);
  painter.begin(debug_widget_);
  painter.setPen(p);

  //std::set<std::vector<osm::Point<double>>>&
  for (auto it = coastline_polygons.begin(); it != coastline_polygons.end();
  ++it) { for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
          //painter.drawEllipse(it2->x, it2->y, 5, 5);
      }
  }

  painter.setPen(p2);
  int idx = 0;
  for (auto it = coastline_polygons.begin(); it != coastline_polygons.end();
  ++it) { qDebug() << "\n-------------------\nDraw new
  polygon\n-------------------\n"; for (auto it2 = it->begin()+1; it2 !=
  it->end(); ++it2, ++idx) {
          //painter.setPen(p2);
          painter.drawLine((it2-1)->x, (it2-1)->y, it2->x, it2->y);
          int x = ((it2)->x - (it2-1)->x)/2 + (it2)->x;
          int y = ((it2)->y - (it2-1)->y)/2 + (it2)->y;
          //painter.setPen(p3);
          //painter.drawText(x, y, QString::number(idx));
          qDebug() << "Draw line from " << (it2-1)->x << "," << (it2-1)->y <<
  "to" << (it2)->x << "," << (it2)->y;
      }
  }

  painter.end();
  debug_widget_->repaint();*/

  // auto finish = std::chrono::high_resolution_clock::now();
  // auto elapsed = finish - start;
  // qDebug() << "Done -> took " << elapsed.count() << "s\n";
  qDebug() << "Done -> took " << timer.elapsed() << "ms\n";

  std::vector<osm::Way*> ways;
  // Now generate nodes and ways and add everything to an OsmObjCollection
  // instance.
  for (auto it = coastline_polygons.begin(); it != coastline_polygons.end();
       ++it) {
    osm::Way* way = new osm::Way;
    way->is_closed = true;
    ways.push_back(way);
    std::vector<osm::Point<double>> polygon = *it;
    int node_idx = 0;
    for (auto it_nodes = polygon.begin() /*+4*/; it_nodes != polygon.end();
         ++it_nodes, ++node_idx) {
      osm::Node* node = new osm::Node;
      node->lat = it_nodes->lat;
      node->lon = it_nodes->lon;
      node->x = it_nodes->x;
      node->y = h - it_nodes->y;
      way->nodes.push_back(node);
      // if (it_nodes <= polygon.begin()+10 || it_nodes >= polygon.end()-10) {
      //     qDebug() << node_idx << ":" << it_nodes->x << "," << it_nodes->y;
      // }
    }
    /*osm::Node* node = new osm::Node;
    node->lat = polygon.at(0).lat;
    node->lon = polygon.at(0).lon;
    node->x = polygon.at(0).x;
    node->y = h - polygon.at(0).y;
    way->nodes.push_back(node);*/
  }

  // Now "ways" contains all the newly created coastline polygons (which is the
  // landmass).

  osm::Objects* obj_ocean = objects_repository_.Objects(osm::kOceanName);
  if (obj_ocean == nullptr) {
    obj_ocean = new osm::Objects(osm::kOceanName, {}, osm::ObjectsTypes::OCEAN);
    objects_repository_.AddObjects(osm::kOceanName, obj_ocean);
  } else {
    obj_ocean->Clear();
  }
  obj_ocean->Ways(ways);

  osm::Objects* obj_landmass = objects_repository_.Objects(osm::kLandmassName);
  if (obj_landmass == nullptr) {
    obj_landmass =
        new osm::Objects(osm::kLandmassName, {}, osm::ObjectsTypes::LANDMASS);
    objects_repository_.AddObjects(osm::kLandmassName, obj_landmass);
  } else {
    obj_landmass->Clear();
  }
  obj_landmass->Ways(ways);
}
