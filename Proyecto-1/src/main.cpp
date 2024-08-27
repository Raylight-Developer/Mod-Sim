#pragma execution_character_set( "utf-8" )

#include "Include.hpp"
#include "Particle.hpp"

#include "Params.hpp";

struct ParticleSimulation : QGraphicsScene {
	vector<vector<Particle*>> systems;
	QRectF bounding_box;

	ParticleSimulation(QMainWindow* parent = nullptr) :
		QGraphicsScene(parent)
	{
		systems.reserve(SAMPLES);
		bounding_box = BOUNDING_BOX;

		addRect(bounding_box);
		addLine(-400, 0, 400, 0);
		setup_particles();
		setSceneRect(bounding_box);
	}

	void setup_particles() {
		for (uint64 i = 0; i < SAMPLES; ++i) {
			vector<Particle*> params;
			for (const auto& param : PARAMETERS) {
				auto particle = new Particle(param);
				params.push_back(particle);
			}
			params[3]->setCenter(params[3]->params.center + dvec2(SHIFT * ul_to_f(i), 0));
			systems.push_back(params);
		}

		for (uint64 i = 0; i < systems.size(); ++i) {
			QColor color;
			color.setHsv(d_to_i((i / ul_to_d(SAMPLES)) * 360.0), 150, 255);

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
			}
		}
	}

	void advance(const dvec1& delta_time) {
		update_particles(delta_time);
	}
};

struct MainWindow : QMainWindow {
	MainWindow() {
		showMaximized();
		QTimer::singleShot(1000, this, &MainWindow::init);

		view = new QGraphicsView(this);
		simulation = new ParticleSimulation(this);
		view->setScene(simulation);
		setCentralWidget(view);

		QTransform transform;
		transform.scale(1.2, -1.2);
		view->translate(0, -view->height());
		view->setTransform(transform);
	}

	void init() {
		timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MainWindow::update_scene);
		timer->start(0);

		fps_timer = new QTimer(this);
		connect(fps_timer, &QTimer::timeout, this, &MainWindow::print_fps);
		fps_timer->start(1000);

		sim_timer = new QTimer(this);
		connect(sim_timer, &QTimer::timeout, this, &MainWindow::finish);
		sim_timer->start(SIM_TIME * 1000);

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

	MainWindow* window = new MainWindow();
	window->show();
	application->exec();
	return 0;
}