#pragma execution_character_set( "utf-8" )

#include "Include.hpp"
#include "Particle.hpp"

#undef slots

#define WITHOUT_NUMPY
#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

struct ParticleSimulation : QGraphicsScene {
	map<string, dvec1> args;
	map<uint64, map<Particle<vec1, vec2>*, tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>>> f_system_data;
	map<uint64, map<Particle<dvec1, dvec2>*, tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>>> d_system_data;
	vector<vector<Particle<vec1,vec2>*>> f_systems;
	vector<vector<Particle<dvec1,dvec2>*>> d_systems;
	QRectF bounding_box;
	QElapsedTimer timer;
	QGraphicsRectItem* f_rect_item;
	QGraphicsRectItem* d_rect_item;
	QGraphicsRectItem* rect_item;

	ParticleSimulation(const map<string, dvec1>& args, QMainWindow* parent = nullptr) :
		args(args),
		QGraphicsScene(parent)
	{
		f_systems.reserve(d_to_ul(args.at("System Count")));
		d_systems.reserve(d_to_ul(args.at("System Count")));
		bounding_box = QRectF(-args.at("Bounds Width") * 0.5, 0, args.at("Bounds Width"), args.at("Bounds Height"));

		auto scene_rect = QRectF(-args.at("Bounds Width") * 1.1 - 50, -50, args.at("Bounds Width") * 2.2 + 100, args.at("Bounds Height") + 100);
		rect_item = new QGraphicsRectItem(scene_rect);
		rect_item->setBrush(QBrush(QColor("transparent")));
		rect_item->setPen(QPen(QColor("red"), 2.0));

		f_rect_item = new QGraphicsRectItem(bounding_box.translated(QPointF(-args.at("Bounds Width") * 0.6, 0)));
		f_rect_item->setBrush(QBrush(QColor("transparent")));
		f_rect_item->setPen(QPen(QColor("black"), 2.0));

		d_rect_item = new QGraphicsRectItem(bounding_box.translated(QPointF(args.at("Bounds Width") * 0.6, 0)));
		d_rect_item->setBrush(QBrush(QColor("transparent")));
		d_rect_item->setPen(QPen(QColor("black"), 2.0));

		addItem(f_rect_item);
		addItem(d_rect_item);
		addItem(rect_item);
		addLine(scene_rect.x(), 0, scene_rect.x() + scene_rect.width(), 0);
		auto item_a = addText("32 bits");
		auto item_b = addText("64 bits");
		auto transform = QTransform();
		transform.scale(1.4, -1.4);
		item_a->setPos(bounding_box.translated(QPointF(-args.at("Bounds Width") * 0.6, 0)).bottomLeft());
		item_b->setPos(bounding_box.translated(QPointF(args.at("Bounds Width") * 0.6, 0)).bottomLeft());
		item_a->setTransform(transform);
		item_b->setTransform(transform);
		setup_particles();
		setSceneRect(scene_rect);
	}

	void setup_particles() {
		auto F_PARAMETERS = Particle_Params<vec1, vec2>::parseParticleParams(readFile("./Params.txt"));
		auto D_PARAMETERS = Particle_Params<dvec1, dvec2>::parseParticleParams(readFile("./Params.txt"));
		for (uint64 i = 0; i < d_to_ul(args.at("System Count")); ++i) {
			f_system_data[i] = {};
			d_system_data[i] = {};
			vector<Particle<vec1, vec2>*> f_params;
			vector<Particle<dvec1, dvec2>*> d_params;
			for (uint64 j = 0; j < F_PARAMETERS.size(); j++) {
				auto f_particle = new Particle<vec1, vec2>(F_PARAMETERS[j], -bounding_box.width() * 0.6);
				auto d_particle = new Particle<dvec1, dvec2>(D_PARAMETERS[j], bounding_box.width() * 0.6);
				f_particle->TIME_SCALE = args.at("Time Scale");
				d_particle->TIME_SCALE = args.at("Time Scale");
				f_particle->GRAVITY = dvec2(args.at("Gravity X"), args.at("Gravity Y"));
				d_particle->GRAVITY = dvec2(args.at("Gravity X"), args.at("Gravity Y"));
				f_particle->SLIDING_FRICTION_COEFFICIENT = args.at("Sliding Friction");
				d_particle->SLIDING_FRICTION_COEFFICIENT = args.at("Sliding Friction");
				f_particle->ROLLING_FRICTION_COEFFICIENT = args.at("Rolling Friction");
				d_particle->ROLLING_FRICTION_COEFFICIENT = args.at("Rolling Friction");

				f_params.push_back(f_particle);
				d_params.push_back(d_particle);
				f_system_data[i][f_particle] = tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>({}, {}, {}, {}, {});
				d_system_data[i][d_particle] = tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>({}, {}, {}, {}, {});
			}
			f_params[d_to_ul(args.at("Shifter"))]->setCenter(f_params[d_to_ul(args.at("Shifter"))]->params.center + vec2(args.at("Shift X"), args.at("Shift Y")) * ul_to_f(i));
			d_params[d_to_ul(args.at("Shifter"))]->setCenter(d_params[d_to_ul(args.at("Shifter"))]->params.center + dvec2(args.at("Shift X"), args.at("Shift Y")) * ul_to_d(i));
			f_systems.push_back(f_params);
			d_systems.push_back(d_params);
		}

		for (uint64 i = 0; i < f_systems.size(); ++i) {
			QColor color;
			color.setHsv(d_to_i((i / args.at("System Count")) * 360.0), 150, 255, d_to_i(args.at("Opacity") * 255.0));

			for (uint64 j = 0; j < f_systems[i].size(); ++j) {
				const auto& f_particle = f_systems[i][j];
				const auto& d_particle = d_systems[i][j];
				
				f_particle->setBrush(color);
				f_particle->setZValue((i + 1) * (j + 1));
				d_particle->setBrush(color);
				d_particle->setZValue((i + 1) * (j + 1));
				addItem(f_particle);
				addItem(d_particle);
			}
		}
	}

	void update_particles(const dvec1& delta_time) {
		for (uint64 i = 0; i < f_systems.size(); ++i) {
			for (uint64 j = 0; j < f_systems[i].size(); ++j) {
				auto& f_particle_a = f_systems[i][j];
				f_particle_a->tick(delta_time, bounding_box, -bounding_box.width() * 0.6);

				for (uint64 k = j + 1; k < f_systems[i].size(); ++k) {
					auto& f_particle_b = f_systems[i][k];
					f_particle_a->handle_particle_collision(f_particle_b);
				}
				get<0>(f_system_data[i][f_particle_a]).push_back(timer.elapsed());
				get<1>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.center);
				get<2>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.velocity);
				get<3>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.angular_velocity);
				get<4>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.acceleration);

				auto& d_particle_a = d_systems[i][j];
				d_particle_a->tick(delta_time, bounding_box, bounding_box.width() * 0.6);

				for (uint64 k = j + 1; k < d_systems[i].size(); ++k) {
					auto& d_particle_b = d_systems[i][k];
					d_particle_a->handle_particle_collision(d_particle_b);
				}
				get<0>(d_system_data[i][d_particle_a]).push_back(timer.elapsed());
				get<1>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.center);
				get<2>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.velocity);
				get<3>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.angular_velocity);
				get<4>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.acceleration);
			}
		}
	}

	void advance(const dvec1& delta_time) {
		update_particles(delta_time);
	}
};

struct MainWindow : QMainWindow {
	map<string, dvec1> args;
	MainWindow(const map<string, dvec1>& args) :
		args(args),
		QMainWindow()
	{

		view = new QGraphicsView(this);
		simulation = new ParticleSimulation(args, this);
		view->setScene(simulation);
		setCentralWidget(view);

		view->fitInView(simulation->f_rect_item, Qt::AspectRatioMode::KeepAspectRatio);
		QTransform transform;
		transform.scale(1, -1);
		view->translate(0, -view->height());
		view->setTransform(transform);
		showMaximized();

		QTimer::singleShot(1000, this, &MainWindow::init);
		QTimer::singleShot(100, this, [this]() { view->fitInView(simulation->rect_item, Qt::AspectRatioMode::KeepAspectRatio); });
	}

	void init() {
		simulation->timer.start();
		timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MainWindow::update_scene);
		timer->start(0);

		fps_timer = new QTimer(this);
		connect(fps_timer, &QTimer::timeout, this, &MainWindow::print_fps);
		fps_timer->start(1000);

		sim_timer = new QTimer(this);
		connect(sim_timer, &QTimer::timeout, this, &MainWindow::finish);
		sim_timer->start(args.at("Duration") * 1000.0);

		elapsed_timer.start();
		frame_count = 0;
	}

	void update_scene() {
		dvec1 delta_time = elapsed_timer.elapsed() / 1000.0;
		elapsed_timer.restart();

		simulation->advance(delta_time);
		view->viewport()->update();
		++frame_count;
	}

	void print_fps() {
		int fps = frame_count;
		frame_count = 0;
		qDebug() << "FPS:" << fps;
	}

	void finish() {
		timer->stop();
		fps_timer->stop();

		if (d_to_i(args.at("Generate Graphics")) == 1) {
			for (const auto& [system_id, system] : simulation->f_system_data) {
				if (system_id >= d_to_ul(args.at("Output Start")) and system_id <= d_to_ul(args.at("Output End"))) {
					uint64 i = 0;
					for (const auto& [particle, data] : system) {
						vector<double> x_vals, y_vals;

						const auto& [timestamps_uint64, positions, velocities, angular_velocities, accelerations] = data;
						vector<double> timestamps(timestamps_uint64.size());
						transform(timestamps_uint64.begin(), timestamps_uint64.end(), timestamps.begin(), [](uint64_t value) { return static_cast<double>(value) / 1000.0; });

						auto extract_component = [](const vector<dvec2>& vec, int component) {
							vector<double> result;
							for (const auto& v : vec) {
								result.push_back(component == 0 ? v.x : v.y);
							}
							return result;
							};

						// Plot Positions
						auto pos_x = extract_component(positions, 0);
						auto pos_y = extract_component(positions, 1);
						plt::plot(timestamps, pos_x, { {"label", "x"} });
						plt::plot(timestamps, pos_y, { {"label", "y"} });
						plt::xlabel("Time");
						plt::ylabel("Position");
						plt::legend();
						plt::title("Positions");
						plt::save("./Outputs/Position_" + to_string(system_id) + "_" + to_string(i) + ".png");
						plt::clf();

						// Plot Velocities
						auto vel_x = extract_component(velocities, 0);
						auto vel_y = extract_component(velocities, 1);
						plt::plot(timestamps, vel_x, { {"label", "x"} });
						plt::plot(timestamps, vel_y, { {"label", "y"} });
						plt::xlabel("Time");
						plt::ylabel("Velocity");
						plt::legend();
						plt::title("Velocities");
						plt::save("./Outputs/Velocity_" + to_string(system_id) + "_" + to_string(i) + ".png");
						plt::clf();

						// Plot Angular Velocities
						auto vel = angular_velocities;
						plt::plot(timestamps, vel_x);
						plt::xlabel("Time");
						plt::ylabel("Angular Velocity");
						plt::title("Angular Velocity");
						plt::save("./Outputs/Angular_Velocity_" + to_string(system_id) + "_" + to_string(i) + ".png");
						plt::clf();

						// Plot Accelerations
						auto acc_x = extract_component(accelerations, 0);
						auto acc_y = extract_component(accelerations, 1);
						plt::plot(timestamps, acc_x, { {"label", "x"} });
						plt::plot(timestamps, acc_y, { {"label", "y"} });
						plt::xlabel("Time");
						plt::ylabel("Acceleration");
						plt::legend();
						plt::title("Accelerations");
						plt::save("./Outputs/Acceleration_" + to_string(system_id) + "_" + to_string(i) + ".png");
						plt::clf();

						i++;
					}
				}
			}
		}
		QApplication::quit();
	}

private:
	QGraphicsView* view;
	ParticleSimulation* simulation;
	QTimer* timer;
	QTimer* fps_timer;
	QTimer* sim_timer;
	QElapsedTimer elapsed_timer;
	uint64 frame_count;
};


int main(int argc, char* argv[]) {
	SetConsoleOutputCP(65001);
	QApplication::setAttribute(Qt::ApplicationAttribute::AA_NativeWindows);
	QApplication::setAttribute(Qt::ApplicationAttribute::AA_UseDesktopOpenGL);


	QApplication* application = new QApplication(argc, argv);

	map<string, dvec1> args = {};
	args["System Count"] = 4;
	args["Output Start"] = 0;
	args["Output End"] = 2;
	args["Shifter"] = 1;
	args["Shift X"] = 1e-8;
	args["Shift Y"] = 0;
	args["Duration"] = 10.0;
	args["Gravity X"] = 0.0;
	args["Gravity Y"] = -9.81;
	args["Sliding Friction"] = 0.3;
	args["Rolling Friction"] = 0.15;
	args["Time Scale"] = 5.0;
	args["Opacity"] = 0.35;
	args["Bounds Width"] = 400;
	args["Bounds Height"] = 800;
	args["Generate Graphics"] = 1;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--system-count") == 0 && i + 1 < argc) {
			args["System Count"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--system-output-start") == 0 && i + 1 < argc) {
			args["Output Start"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--system-output-end") == 0 && i + 2 < argc) {
			args["Output End"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--shift-index") == 0 && i + 2 < argc) {
			args["Shifter"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--shift") == 0 && i + 1 < argc) {
			args["Shift X"] = str_to_d(argv[++i]);
			args["Shift Y"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
			args["Duration"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--gravity") == 0 && i + 1 < argc) {
			args["Gravity X"] = str_to_d(argv[++i]);
			args["Gravity Y"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--sliding-friction") == 0 && i + 1 < argc) {
			args["Sliding Friction"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--rolling-friction") == 0 && i + 1 < argc) {
			args["Rolling Friction"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--time-scale") == 0 && i + 1 < argc) {
			args["Time Scale"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--particle-opacity") == 0 && i + 1 < argc) {
			args["Opacity"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--bounds") == 0 && i + 1 < argc) {
			args["Bounds Width"] = str_to_d(argv[++i]);
			args["Bounds Height"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--generate-graphics") == 0 && i + 1 < argc) {
			args["Generate Graphics"] = str_to_d(argv[++i]);
		} else {
			cerr << "Unknown or incomplete argument: " << argv[i] << endl;
		}
	}

	MainWindow* window = new MainWindow(args);
	application->exec();
	return 0;
}