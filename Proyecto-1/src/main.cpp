#pragma execution_character_set( "utf-8" )

#include "Include.hpp"
#include "Particle.hpp"

#include "Params.hpp";

#undef slots

#define WITHOUT_NUMPY
#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

struct ParticleSimulation : QGraphicsScene {
	map<string, dvec1> args;
	map<uint64, map<Particle*, tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>>> system_data;
	vector<vector<Particle*>> systems;
	QRectF bounding_box;
	QElapsedTimer timer;

	ParticleSimulation(const map<string, dvec1>& args, QMainWindow* parent = nullptr) :
		args(args),
		QGraphicsScene(parent)
	{
		systems.reserve(d_to_ul(args.at("System Count")));
		bounding_box = BOUNDING_BOX;

		addRect(bounding_box);
		addLine(-400, 0, 400, 0);
		setup_particles();
		setSceneRect(bounding_box);
	}

	void setup_particles() {
		for (uint64 i = 0; i < d_to_ul(args.at("System Count")); ++i) {
			system_data[i] = {};
			vector<Particle*> params;
			for (auto param : PARAMETERS) {
				param.system_id = to_string(i);
				auto particle = new Particle(param);
				particle->TIME_SCALE = args.at("Time Scale");
				particle->GRAVITY = dvec2(args.at("Gravity X"), args.at("Gravity Y"));
				particle->SLIDING_FRICTION_COEFFICIENT = args.at("Sliding Friction");
				particle->ROLLING_FRICTION_COEFFICIENT = args.at("Rolling Friction");
				params.push_back(particle);
				system_data[i][particle] = tuple<vector<uint64>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec2>>({}, {}, {}, {}, {});
			}
			params[3]->setCenter(params[3]->params.center + dvec2(args.at("Shift") * ul_to_f(i), 0));
			systems.push_back(params);
		}

		for (uint64 i = 0; i < systems.size(); ++i) {
			QColor color;
			color.setHsv(d_to_i((i / args.at("System Count")) * 360.0), 150, 255, 100);

			for (uint64 j = 0; j < systems[i].size(); ++j) {
				const auto& particle = systems[i][j];
				
				particle->setBrush(color);
				particle->setZValue((i + 1) * (j + 1));
				addItem(particle);
			}
		}
	}

	void update_particles(const dvec1& delta_time) {
		for (uint64 i = 0; i < systems.size(); ++i) {
			for (uint64 j = 0; j < systems[i].size(); ++j) {
				auto& particle_a = systems[i][j];
				particle_a->tick(delta_time, BOUNDING_BOX);

				for (uint64 k = j + 1; k < systems[i].size(); ++k) {
					auto& particle_b = systems[i][k];
					particle_a->handle_particle_collision(particle_b);
				}
				get<0>(system_data[i][particle_a]).push_back(timer.elapsed());
				get<1>(system_data[i][particle_a]).push_back(particle_a->params.center);
				get<2>(system_data[i][particle_a]).push_back(particle_a->params.velocity);
				get<3>(system_data[i][particle_a]).push_back(particle_a->params.angular_velocity);
				get<4>(system_data[i][particle_a]).push_back(particle_a->params.acceleration);
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
		showMaximized();
		QTimer::singleShot(1000, this, &MainWindow::init);

		view = new QGraphicsView(this);
		simulation = new ParticleSimulation(args, this);
		view->setScene(simulation);
		setCentralWidget(view);

		QTransform transform;
		transform.scale(1.2, -1.2);
		view->translate(0, -view->height());
		view->setTransform(transform);
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

		for (const auto& [system_id, system] : simulation->system_data) {
			if (system_id >= d_to_ul(args.at("Output Start")) and system_id <= d_to_ul(args.at("Output End"))) {
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
					plt::save("./Outputs/Position_" + to_string(system_id) + "_" + particle->params.name + ".png");
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
					plt::save("./Outputs/Velocity_" + to_string(system_id) + "_" + particle->params.name + ".png");
					plt::clf();

					// Plot Angular Velocities
					auto vel = angular_velocities;
					plt::plot(timestamps, vel_x);
					plt::xlabel("Time");
					plt::ylabel("Angular Velocity");
					plt::title("Angular Velocity");
					plt::save("./Outputs/Angular_Velocity_" + to_string(system_id) + "_" + particle->params.name + ".png");
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
					plt::save("./Outputs/Acceleration_" + to_string(system_id) + "_" + particle->params.name + ".png");
					plt::clf();
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
	args["Shift"] = 1e-8;
	args["Duration"] = 5.0;
	args["Gravity X"] = 0.0;
	args["Gravity Y"] = -9.81;
	args["Sliding Friction"] = 0.3;
	args["Rolling Friction"] = 0.15;
	args["Time Scale"] = 10.0;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--system-count") == 0 && i + 1 < argc) {
			args["System Count"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--system-output-start") == 0 && i + 1 < argc) {
			args["Output Start"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--system-output-end") == 0 && i + 2 < argc) {
			args["Output End"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--x-shift") == 0 && i + 1 < argc) {
			args["Shift"] = str_to_d(argv[++i]);
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
		} else {
			cerr << "Unknown or incomplete argument: " << argv[i] << endl;
		}
	}

	MainWindow* window = new MainWindow(args);
	window->show();
	application->exec();
	return 0;
}