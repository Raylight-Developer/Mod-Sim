#pragma execution_character_set( "utf-8" )

#include "Include.hpp"
#include "Particle.hpp"


#define SAMPLES 3ULL
#define SIM_TIME 50.0
#define BOUNDING_BOX QRectF(-200, 0, 400, 800)

const vector<Particle_Params> PARAMETERS = {
	Particle_Params("0", "-1", vec2(0, 185), vec2(0, 0), 0.8, 5.0 , 2.0 ),
	Particle_Params("1", "-1", vec2(0, 165), vec2(0, 0), 0.8, 10.0, 10.0),
	Particle_Params("2", "-1", vec2(0, 130), vec2(0, 0), 0.8, 20.0, 25.0),
	Particle_Params("3", "-1", vec2(0, 80 ), vec2(0, 0), 0.7, 30.0, 40.0)
};

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
			params[0]->setCenter(params[0]->params.center + vec2(5 * i, 0));
			systems.push_back(params);
		}

		for (uint64 i = 0; i < systems.size(); ++i) {
			QColor color;
			color.setHsv((i / static_cast<double>(SAMPLES)) * 360, 150, 255);

			for (uint64 j = 0; j < systems[i].size(); ++j) {
				const auto& particle = systems[i][j];
				
				particle->setBrush(color);
				particle->setZValue((i + 1) * (j + 1));
				addItem(particle);
			}
		}
	}

	void update_particles(double delta_time) {
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

	void advance(double delta_time) {
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
		transform.scale(1.5, -1.5);
		view->translate(0, -view->height());
		view->setTransform(transform);
	}

	void init() {
		timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &MainWindow::update_scene);
		timer->start(20);

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
		double delta_time = elapsed_timer.elapsed() / 1000.0;
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
	int frame_count;
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