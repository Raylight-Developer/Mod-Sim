#pragma execution_character_set( "utf-8" )

#include "Include.hpp"
#include "Particle.hpp"
#include "Viewport.hpp"

#undef slots

#define WITHOUT_NUMPY
#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

tuple<vector<dvec1>, vector<dvec1>> calculate_delta(const vector<vec1>& vec1, const vector<dvec1>& vec2) {
	vector<dvec1> delta(vec1.size());
	vector<dvec1> counter(vec1.size());
	for (size_t i = 0; i < vec1.size(); ++i) {
		delta[i] = f_to_d(vec1[i]) - vec2[i];
		counter[i] = i;
	}

	return tie(counter, delta);
}

struct ParticleSimulation : QGraphicsScene {
	unordered_map<string, dvec1> args;
	// Position<0>, Velocity<1>, Acceleration<2>, Angular_Velocity<3>, Kinetic Energy<4>
	unordered_map<uint64, unordered_map<Particle<vec1, vec2>*, tuple<vector<vec2>, vector<vec2>, vector<vec2>, vector<vec1>, vector<vec1>>>> f_system_data;
	unordered_map<uint64, unordered_map<Particle<dvec1, dvec2>*, tuple<vector<dvec2>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec1>>>> d_system_data;
	vector<vector<Particle<vec1,vec2>*>> f_systems;
	vector<vector<Particle<dvec1,dvec2>*>> d_systems;
	vector<uint64> time_stamps;
	QRectF bounding_box;
	QElapsedTimer timer;
	QGraphicsRectItem* f_rect_item;
	QGraphicsRectItem* d_rect_item;
	QGraphicsRectItem* rect_item;

	uint64 frame_count;

	ParticleSimulation(const unordered_map<string, dvec1>& args, QMainWindow* parent = nullptr) :
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
		frame_count = 0;
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
				f_system_data[i][f_particle] = tuple<vector<vec2>, vector<vec2>, vector<vec2>, vector<vec1>, vector<vec1>>({});
				d_system_data[i][d_particle] = tuple<vector<dvec2>, vector<dvec2>, vector<dvec2>, vector<dvec1>, vector<dvec1>>({});
			}
			f_params[d_to_ul(args.at("Shifter"))]->setCenter(f_params[d_to_ul(args.at("Shifter"))]->params.center + vec2(args.at("Shift Pos X"), args.at("Shift Pos Y")) * ul_to_f(i));
			d_params[d_to_ul(args.at("Shifter"))]->setCenter(d_params[d_to_ul(args.at("Shifter"))]->params.center + dvec2(args.at("Shift Pos X"), args.at("Shift Pos Y")) * ul_to_d(i));

			f_params[d_to_ul(args.at("Shifter"))]->params.velocity += (vec2(args.at("Shift Vel X"), args.at("Shift Vel Y")) * ul_to_f(i));
			d_params[d_to_ul(args.at("Shifter"))]->params.velocity += (dvec2(args.at("Shift Vel X"), args.at("Shift Vel Y")) * ul_to_d(i));

			f_systems.push_back(f_params);
			d_systems.push_back(d_params);
		}

		QPen pen = QPen(QColor(0, 0, 0, 100), 0.8);
		for (uint64 i = 0; i < f_systems.size(); ++i) {
			QColor color;
			color.setHsv(d_to_i((i / args.at("System Count")) * 360.0), 150, 255, d_to_i(args.at("Opacity") * 255.0));

			for (uint64 j = 0; j < f_systems[i].size(); ++j) {
				const auto& f_particle = f_systems[i][j];
				const auto& d_particle = d_systems[i][j];
				
				f_particle->setBrush(color);
				f_particle->setPen(pen);
				f_particle->setZValue((i + 1) * (j + 1));
				d_particle->setBrush(color);
				d_particle->setPen(pen);
				d_particle->setZValue((i + 1) * (j + 1));
				addItem(f_particle);
				addItem(d_particle);
			}
		}
	}

	void update_particles(const dvec1& delta_time) {
		const uint system_count = f_system_data.size();
		const uint particle_count = f_system_data[0].size();

		for (uint i = 0; i < system_count; ++i) {
			for (uint j = 0; j < particle_count; ++j) {
				auto& f_particle_a = f_systems[i][j];
				f_particle_a->tick(d_to_f(delta_time), bounding_box, d_to_f(- bounding_box.width() * 0.6));

				for (uint k = j + 1; k < particle_count; ++k) {
					auto& f_particle_b = f_systems[i][k];
					f_particle_a->handle_particle_collision(f_particle_b);
				}
				get<0>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.center);
				get<1>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.velocity);
				get<2>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.acceleration);
				get<3>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.angular_velocity);
				get<4>(f_system_data[i][f_particle_a]).push_back(f_particle_a->params.kinetic_energy);

				auto& d_particle_a = d_systems[i][j];
				d_particle_a->tick(delta_time, bounding_box, bounding_box.width() * 0.6);

				for (uint k = j + 1; k < particle_count; ++k) {
					auto& d_particle_b = d_systems[i][k];
					d_particle_a->handle_particle_collision(d_particle_b);
				}
				get<0>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.center);
				get<1>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.velocity);
				get<2>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.acceleration);
				get<3>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.angular_velocity);
				get<4>(d_system_data[i][d_particle_a]).push_back(d_particle_a->params.kinetic_energy);


			}
		}
		if (args["Realtime"] >= 0.5) {
			time_stamps.push_back(timer.elapsed());
		}
		else {
			time_stamps.push_back(d_to_ul(ul_to_d(frame_count) * delta_time * 1000.0));
		}
	}

	void advance(const dvec1& delta_time) {
		update_particles(delta_time);
		frame_count++;
	}
};

struct MainWindow : QMainWindow {
	unordered_map<string, dvec1> args;
	MainWindow(const unordered_map<string, dvec1>& args) :
		args(args),
		QMainWindow()
	{

		view = new Graphics_View(this);
		simulation = new ParticleSimulation(args, this);
		view->setScene(simulation);
		setCentralWidget(view);

		view->fitInView(simulation->f_rect_item, Qt::AspectRatioMode::KeepAspectRatio);
		QTransform transform;
		transform.scale(1, -1);
		view->translate(0, -view->height());
		view->setTransform(transform);
		showMaximized();

		QTimer::singleShot(d_to_ul(this->args["Delay"] * 1000.0), this, &MainWindow::init);
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

		if (args["Realtime"] >= 0.5) {
			sim_timer = new QTimer(this);
			connect(sim_timer, &QTimer::timeout, this, &MainWindow::finish);
			sim_timer->start(args.at("Duration") * 1000.0);
		}

		elapsed_timer.start();
		frame_count = 0;
		exec_count = 0;
	}

	void update_scene() {
		dvec1 delta_time;
		if (args["Realtime"] >= 0.5) {
			delta_time = elapsed_timer.elapsed() / 1000.0;
		}
		else {
			delta_time = args["Delta"];
			if (exec_count >= d_to_ul(args.at("Duration Steps"))) {
				finish();
			}
		}


		elapsed_timer.restart();

		simulation->advance(delta_time);
		view->viewport()->update();
		frame_count++;
		exec_count++;
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
			const uint particle_count = simulation->f_system_data[0].size();
			const uint system_count = simulation->f_system_data.size();

			vector<vec1>  f_sys_sum_position_x = vector(system_count, 0.0f);
			vector<vec1>  f_sys_sum_position_y = vector(system_count, 0.0f);
			vector<dvec1> d_sys_sum_position_x = vector(system_count, 0.0);
			vector<dvec1> d_sys_sum_position_y = vector(system_count, 0.0);

			vector<vec1>  f_sys_sum_velocity_x = vector(system_count, 0.0f);
			vector<vec1>  f_sys_sum_velocity_y = vector(system_count, 0.0f);
			vector<dvec1> d_sys_sum_velocity_x = vector(system_count, 0.0);
			vector<dvec1> d_sys_sum_velocity_y = vector(system_count, 0.0);

			vector<vec1>  f_sys_sum_acceleration_x = vector(system_count, 0.0f);
			vector<vec1>  f_sys_sum_acceleration_y = vector(system_count, 0.0f);
			vector<dvec1> d_sys_sum_acceleration_x = vector(system_count, 0.0);
			vector<dvec1> d_sys_sum_acceleration_y = vector(system_count, 0.0);

			vector<vec1>  f_sys_sum_angvelocity = vector(system_count, 0.0f);
			vector<dvec1> d_sys_sum_angvelocity = vector(system_count, 0.0);

			vector<vec1>  f_sys_sum_KE = vector(system_count, 0.0f);
			vector<dvec1> d_sys_sum_KE = vector(system_count, 0.0);

			vector<vec1>  f_part_sum_position_x = vector(particle_count, 0.0f);
			vector<vec1>  f_part_sum_position_y = vector(particle_count, 0.0f);
			vector<dvec1> d_part_sum_position_x = vector(particle_count, 0.0);
			vector<dvec1> d_part_sum_position_y = vector(particle_count, 0.0);

			vector<vec1>  f_part_sum_velocity_x = vector(particle_count, 0.0f);
			vector<vec1>  f_part_sum_velocity_y = vector(particle_count, 0.0f);
			vector<dvec1> d_part_sum_velocity_x = vector(particle_count, 0.0);
			vector<dvec1> d_part_sum_velocity_y = vector(particle_count, 0.0);

			vector<vec1>  f_part_sum_acceleration_x = vector(particle_count, 0.0f);
			vector<vec1>  f_part_sum_acceleration_y = vector(particle_count, 0.0f);
			vector<dvec1> d_part_sum_acceleration_x = vector(particle_count, 0.0);
			vector<dvec1> d_part_sum_acceleration_y = vector(particle_count, 0.0);

			vector<vec1>  f_part_sum_angvelocity = vector(particle_count, 0.0f);
			vector<dvec1> d_part_sum_angvelocity = vector(particle_count, 0.0);

			vector<vec1>  f_part_sum_KE = vector(particle_count, 0.0f);
			vector<dvec1> d_part_sum_KE = vector(particle_count, 0.0);

			for (uint64 i = 0; i < system_count; i++) { // System Count
				const auto f_system = simulation->f_system_data[i];
				const auto d_system = simulation->d_system_data[i];

				for (uint64 j = 0; j < particle_count; j++) { // Particle Count

					auto f_it = f_system.begin();
					auto d_it = d_system.begin();
					std::advance(f_it, j);
					std::advance(d_it, j);

					for (uint64 k = 0; k < particle_count; k++) {
						f_sys_sum_position_x[i]     += get<0>((*f_it).second)[k].x;
						f_sys_sum_position_y[i]     += get<0>((*f_it).second)[k].y;
						f_sys_sum_velocity_x[i]     += get<1>((*f_it).second)[k].x;
						f_sys_sum_velocity_y[i]     += get<1>((*f_it).second)[k].y;
						f_sys_sum_acceleration_x[i] += get<2>((*f_it).second)[k].x;
						f_sys_sum_acceleration_y[i] += get<2>((*f_it).second)[k].y;
						f_sys_sum_angvelocity[i]    += get<3>((*f_it).second)[k];
						f_sys_sum_KE[i]             += get<4>((*f_it).second)[k];

						d_sys_sum_position_x[i]     += get<0>((*d_it).second)[k].x;
						d_sys_sum_position_y[i]     += get<0>((*d_it).second)[k].y;
						d_sys_sum_velocity_x[i]     += get<1>((*d_it).second)[k].x;
						d_sys_sum_velocity_y[i]     += get<1>((*d_it).second)[k].y;
						d_sys_sum_acceleration_x[i] += get<2>((*d_it).second)[k].x;
						d_sys_sum_acceleration_y[i] += get<2>((*d_it).second)[k].y;
						d_sys_sum_angvelocity[i]    += get<3>((*d_it).second)[k];
						d_sys_sum_KE[i]             += get<4>((*d_it).second)[k];

						f_part_sum_position_x[k]     += get<0>((*f_it).second)[k].x;
						f_part_sum_position_y[k]     += get<0>((*f_it).second)[k].y;
						f_part_sum_velocity_x[k]     += get<1>((*f_it).second)[k].x;
						f_part_sum_velocity_y[k]     += get<1>((*f_it).second)[k].y;
						f_part_sum_acceleration_x[k] += get<2>((*f_it).second)[k].x;
						f_part_sum_acceleration_y[k] += get<2>((*f_it).second)[k].y;
						f_part_sum_angvelocity[k]    += get<3>((*f_it).second)[k];
						f_part_sum_KE[k]             += get<4>((*f_it).second)[k];

						d_part_sum_position_x[k]     += get<0>((*d_it).second)[k].x;
						d_part_sum_position_y[k]     += get<0>((*d_it).second)[k].y;
						d_part_sum_velocity_x[k]     += get<1>((*d_it).second)[k].x;
						d_part_sum_velocity_y[k]     += get<1>((*d_it).second)[k].y;
						d_part_sum_acceleration_x[k] += get<2>((*d_it).second)[k].x;
						d_part_sum_acceleration_y[k] += get<2>((*d_it).second)[k].y;
						d_part_sum_angvelocity[k]    += get<3>((*d_it).second)[k];
						f_part_sum_KE[k]             += get<4>((*d_it).second)[k];
					}
				}
			}

			{
				auto [xx, yx] = calculate_delta(f_sys_sum_position_x, d_sys_sum_position_x);
				auto [xy, yy] = calculate_delta(f_sys_sum_position_y, d_sys_sum_position_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("System_ID");
				plt::ylabel("Position");
				plt::title("Positions");
				plt::grid(true);
				plt::save("./Outputs/System_Delta_Positions.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_sys_sum_velocity_x, d_sys_sum_velocity_x);
				auto [xy, yy] = calculate_delta(f_sys_sum_velocity_y, d_sys_sum_velocity_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("System_ID");
				plt::ylabel("Velocity");
				plt::title("Velocities");
				plt::grid(true);
				plt::save("./Outputs/System_Delta_Velocities.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_sys_sum_acceleration_x, d_sys_sum_acceleration_x);
				auto [xy, yy] = calculate_delta(f_sys_sum_acceleration_y, d_sys_sum_acceleration_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("System_ID");
				plt::ylabel("Acceleration");
				plt::title("Accelerations");
				plt::grid(true);
				plt::save("./Outputs/System_Delta_Accelerations.png");
				plt::clf();
			}
			{
				auto [x, y] = calculate_delta(f_sys_sum_angvelocity, d_sys_sum_angvelocity);
				plt::plot(x, y);
				plt::xlabel("System_ID");
				plt::ylabel("Angular Velocity");
				plt::title("Angular Velocities");
				plt::grid(true);
				plt::save("./Outputs/System_Delta_Angular_Velocities.png");
				plt::clf();
			}
			
			{
				auto [x, y] = calculate_delta(f_sys_sum_KE, d_sys_sum_KE);
				plt::plot(x, y);
				plt::xlabel("System_ID");
				plt::ylabel("Kinetic Energy");
				plt::title("Kinetic Energies");
				plt::grid(true);
				plt::save("./Outputs/System_Delta_Kinetic_Energy.png");
				plt::clf();
			}

			{
				auto [xx, yx] = calculate_delta(f_part_sum_position_x, d_part_sum_position_x);
				auto [xy, yy] = calculate_delta(f_part_sum_position_y, d_part_sum_position_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Particle_ID");
				plt::ylabel("Position");
				plt::title("Positions");
				plt::grid(true);
				plt::save("./Outputs/Particle_Delta_Positions.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_part_sum_velocity_x, d_part_sum_velocity_x);
				auto [xy, yy] = calculate_delta(f_part_sum_velocity_y, d_part_sum_velocity_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Particle_ID");
				plt::ylabel("Velocity");
				plt::title("Velocities");
				plt::grid(true);
				plt::save("./Outputs/Particle_Delta_Velocities.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_part_sum_acceleration_x, d_part_sum_acceleration_x);
				auto [xy, yy] = calculate_delta(f_part_sum_acceleration_y, d_part_sum_acceleration_y);
				plt::plot(xx, yx, { {"label", "x"} });
				plt::plot(xy, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Particle_ID");
				plt::ylabel("Acceleration");
				plt::title("Accelerations");
				plt::grid(true);
				plt::save("./Outputs/Particle_Delta_Accelerations.png");
				plt::clf();
			}
			{
				auto [x, y] = calculate_delta(f_part_sum_angvelocity, d_part_sum_angvelocity);
				plt::plot(x, y);
				plt::xlabel("Particle_ID");
				plt::ylabel("Angular Velocity");
				plt::title("Angular Velocities");
				plt::grid(true);
				plt::save("./Outputs/Particle_Delta_Angular_Velocities.png");
				plt::clf();
			}
			
			{
				auto [x, y] = calculate_delta(f_part_sum_KE, d_part_sum_KE);
				plt::plot(x, y);
				plt::xlabel("Particle_ID");
				plt::ylabel("Kinetic Energy");
				plt::title("Kinetic Energies");
				plt::grid(true);
				plt::save("./Outputs/Particle_Delta_Kinetic_Energy.png");
				plt::clf();
			}

			vector<dvec1> fd_part_sum_KE;
			for (auto val : f_part_sum_KE) {
				fd_part_sum_KE.push_back(f_to_d(val));
			}
			{
				auto [x, y] = calculate_delta(f_part_sum_KE, d_part_sum_KE);
				plt::plot(x, y,              { {"label", "delta"} , {"linewidth", "5.0"} });
				plt::plot(x, fd_part_sum_KE, { {"label", "fp32" } , {"linewidth", "2.0"} , {"linestyle", "--" } });
				plt::plot(x, d_part_sum_KE , { {"label", "fp64" } , {"linewidth", "2.0"} , {"linestyle", "--" } });
				plt::legend();
				plt::xlabel("Particle_ID");
				plt::ylabel("Kinetic Energy");
				plt::title("Kinetic Energies");
				plt::grid(true);
				plt::save("./Outputs/Delta_Kinetic_Energy.png");
				plt::clf();
			}
		}

		if (d_to_i(args.at("Generate Tick Graphics")) == 1) {
			const uint particle_count = simulation->f_system_data[0].size();
			const uint system_count = simulation->f_system_data.size();

			vector<vec1>  f_sum_position_x;
			vector<vec1>  f_sum_position_y;
			vector<dvec1> d_sum_position_x;
			vector<dvec1> d_sum_position_y;

			vector<vec1>  f_sum_velocity_x;
			vector<vec1>  f_sum_velocity_y;
			vector<dvec1> d_sum_velocity_x;
			vector<dvec1> d_sum_velocity_y;

			vector<vec1>  f_sum_acceleration_x;
			vector<vec1>  f_sum_acceleration_y;
			vector<dvec1> d_sum_acceleration_x;
			vector<dvec1> d_sum_acceleration_y;

			vector<vec1>  f_sum_angvelocity;
			vector<dvec1> d_sum_angvelocity;

			vector<vec1>  f_sum_KE;
			vector<dvec1> d_sum_KE;

			for (uint64 s = 0; s < simulation->frame_count; s++) {
				vec1 ft_sum_position_x       = 0.0;
				vec1 ft_sum_position_y       = 0.0;
				vec1 ft_sum_velocity_x       = 0.0;
				vec1 ft_sum_velocity_y       = 0.0;
				vec1 ft_sum_acceleration_x   = 0.0;
				vec1 ft_sum_acceleration_y   = 0.0;
				vec1 ft_sum_angvelocity      = 0.0;
				vec1 ft_sum_KE               = 0.0;
				
				dvec1 dt_sum_position_x      = 0.0;
				dvec1 dt_sum_position_y      = 0.0;
				dvec1 dt_sum_velocity_x      = 0.0;
				dvec1 dt_sum_velocity_y      = 0.0;
				dvec1 dt_sum_acceleration_x  = 0.0;
				dvec1 dt_sum_acceleration_y  = 0.0;
				dvec1 dt_sum_angvelocity     = 0.0;
				dvec1 dt_sum_KE              = 0.0;
				for (uint64 i = 0; i < system_count; i++) { // System Count
					const auto f_system = simulation->f_system_data[i];
					const auto d_system = simulation->d_system_data[i];

					for (uint64 j = 0; j < particle_count; j++) { // Particle Count

						auto f_it = f_system.begin();
						auto d_it = d_system.begin();
						std::advance(f_it, j);
						std::advance(d_it, j);

						for (uint64 k = 0; k < particle_count; k++) {

							ft_sum_position_x     += get<0>((*f_it).second)[k].x;
							ft_sum_position_y     += get<0>((*f_it).second)[k].y;
							ft_sum_velocity_x     += get<1>((*f_it).second)[k].x;
							ft_sum_velocity_y     += get<1>((*f_it).second)[k].y;
							ft_sum_acceleration_x += get<2>((*f_it).second)[k].x;
							ft_sum_acceleration_y += get<2>((*f_it).second)[k].y;
							ft_sum_angvelocity    += get<3>((*f_it).second)[k];
							ft_sum_KE             += get<4>((*f_it).second)[k];

							dt_sum_position_x     += get<0>((*d_it).second)[k].x;
							dt_sum_position_y     += get<0>((*d_it).second)[k].y;
							dt_sum_velocity_x     += get<1>((*d_it).second)[k].x;
							dt_sum_velocity_y     += get<1>((*d_it).second)[k].y;
							dt_sum_acceleration_x += get<2>((*d_it).second)[k].x;
							dt_sum_acceleration_y += get<2>((*d_it).second)[k].y;
							dt_sum_angvelocity    += get<3>((*d_it).second)[k];
							dt_sum_KE             += get<4>((*d_it).second)[k];
						}
					}
				}
				f_sum_position_x    .push_back(ft_sum_position_x     );
				f_sum_position_y    .push_back(ft_sum_position_y     );
				f_sum_velocity_x    .push_back(ft_sum_velocity_x     );
				f_sum_velocity_y    .push_back(ft_sum_velocity_y     );
				f_sum_acceleration_x.push_back(ft_sum_acceleration_x );
				f_sum_acceleration_y.push_back(ft_sum_acceleration_y );
				f_sum_angvelocity   .push_back(ft_sum_angvelocity    );
				f_sum_KE            .push_back(ft_sum_KE             );

				d_sum_position_x    .push_back(dt_sum_position_x     );
				d_sum_position_y    .push_back(dt_sum_position_y     );
				d_sum_velocity_x    .push_back(dt_sum_velocity_x     );
				d_sum_velocity_y    .push_back(dt_sum_velocity_y     );
				d_sum_acceleration_x.push_back(dt_sum_acceleration_x );
				d_sum_acceleration_y.push_back(dt_sum_acceleration_y );
				d_sum_angvelocity   .push_back(dt_sum_angvelocity    );
				d_sum_KE            .push_back(dt_sum_KE             );
			}

			vector<dvec1> time_stamps;
			for (auto val : simulation->time_stamps) {
				time_stamps.push_back(val);
			}

			{
				auto [xx, yx] = calculate_delta(f_sum_position_x, d_sum_position_x);
				auto [xy, yy] = calculate_delta(f_sum_position_y, d_sum_position_y);
				plt::plot(time_stamps, yx, { {"label", "x"} });
				plt::plot(time_stamps, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Time (ms)");
				plt::ylabel("Position");
				plt::title("Positions");
				plt::grid(true);
				plt::save("./Outputs/Tick_Delta_Positions.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_sum_velocity_x, d_sum_velocity_x);
				auto [xy, yy] = calculate_delta(f_sum_velocity_y, d_sum_velocity_y);
				plt::plot(time_stamps, yx, { {"label", "x"} });
				plt::plot(time_stamps, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Time (ms)");
				plt::ylabel("Velocity");
				plt::title("Velocities");
				plt::grid(true);
				plt::save("./Outputs/Tick_Delta_Velocities.png");
				plt::clf();
			}
			{
				auto [xx, yx] = calculate_delta(f_sum_acceleration_x, d_sum_acceleration_x);
				auto [xy, yy] = calculate_delta(f_sum_acceleration_y, d_sum_acceleration_y);
				plt::plot(time_stamps, yx, { {"label", "x"} });
				plt::plot(time_stamps, yy, { {"label", "y"} });
				plt::legend();
				plt::xlabel("Time (ms)");
				plt::ylabel("Acceleration");
				plt::title("Accelerations");
				plt::grid(true);
				plt::save("./Outputs/Tick_Delta_Accelerations.png");
				plt::clf();
			}
			{
				auto [x, y] = calculate_delta(f_sum_angvelocity, d_sum_angvelocity);
				plt::plot(time_stamps, y);
				plt::xlabel("Time (ms)");
				plt::ylabel("Angular Velocity");
				plt::title("Angular Velocities");
				plt::grid(true);
				plt::save("./Outputs/Tick_Delta_Angular_Velocities.png");
				plt::clf();
			}

			{
				auto [x, y] = calculate_delta(f_sum_KE, d_sum_KE);
				plt::plot(time_stamps, y);
				plt::xlabel("Time (ms)");
				plt::ylabel("Kinetic Energy");
				plt::title("Kinetic Energies");
				plt::grid(true);
				plt::save("./Outputs/Tick_Delta_Kinetic_Energy.png");
				plt::clf();
			}
		}
	}

private:
	Graphics_View* view;
	ParticleSimulation* simulation;
	QTimer* timer;
	QTimer* fps_timer;
	QTimer* sim_timer;
	QElapsedTimer elapsed_timer;
	uint64 frame_count;
	uint64 exec_count;
};


int main(int argc, char* argv[]) {
	SetConsoleOutputCP(65001);
	QApplication::setAttribute(Qt::ApplicationAttribute::AA_NativeWindows);

	QApplication* application = new QApplication(argc, argv);

	unordered_map<string, dvec1> args = {};
	args["System Count"] = 4;
	args["Shifter"] = 1;
	args["Shift Pos X"] = 1e-8;
	args["Shift Pos Y"] = 0;
	args["Shift Vel X"] = 0;
	args["Shift Vel Y"] = 0;
	args["Gravity X"] = 0.0;
	args["Gravity Y"] = -9.81;
	args["Sliding Friction"] = 0.3;
	args["Rolling Friction"] = 0.15;
	args["Opacity"] = 0.35;
	args["Bounds Width"] = 400;
	args["Bounds Height"] = 800;
	args["Generate Graphics"] = 1;
	args["Generate Tick Graphics"] = 1;
	args["Duration"] = 5.0;
	args["Duration Steps"] = 500;
	args["Time Scale"] = 5.0;
	args["Delta"] = 0.01;
	args["Realtime"] = 0;
	args["Delay"] = 1.5;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--system-count") == 0 && i + 1 < argc) {
			args["System Count"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--shift-index") == 0 && i + 2 < argc) {
			args["Shifter"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--shift-pos") == 0 && i + 1 < argc) {
			args["Shift Pos X"] = str_to_d(argv[++i]);
			args["Shift Pos Y"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--shift-vel") == 0 && i + 1 < argc) {
			args["Shift Vel X"] = str_to_d(argv[++i]);
			args["Shift Vel Y"] = str_to_d(argv[++i]);
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
		} else if (strcmp(argv[i], "--delay") == 0 && i + 1 < argc) {
			args["Delay"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--particle-opacity") == 0 && i + 1 < argc) {
			args["Opacity"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--bounds") == 0 && i + 1 < argc) {
			args["Bounds Width"] = str_to_d(argv[++i]);
			args["Bounds Height"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--generate-graphics") == 0 && i + 1 < argc) {
			args["Generate Graphics"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--generate-tick-graphics") == 0 && i + 1 < argc) {
			args["Generate Tick Graphics"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--delta-step") == 0 && i + 1 < argc) {
			args["Delta"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--duration-steps") == 0 && i + 1 < argc) {
			args["Duration Steps"] = str_to_d(argv[++i]);
		} else if (strcmp(argv[i], "--realtime") == 0 && i + 1 < argc) {
			args["Realtime"] = str_to_d(argv[++i]);
		} else {
			cerr << "Unknown or incomplete argument: " << argv[i] << endl;
		}
	}

	MainWindow* window = new MainWindow(args);
	application->exec();
	return 0;
}