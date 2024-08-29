Alejandro Martinez - 21430

Samuel Argueta - 211024

Hansel López - 19026

# Simulation
[Run The Program](./Run.py)

[Simulation Parameters](./src/Params.hpp)
```cpp
#define BOUNDING_BOX QRectF(-200, 0, 400, 800)

const vector<Particle_Params> PARAMETERS = {
	Particle_Params("0", "-1", dvec2(0, 185), dvec2(0, 0), 0.8, 5.0 , 2.0 ),
	Particle_Params("1", "-1", dvec2(0, 165), dvec2(0, 0), 0.8, 10.0, 10.0),
	Particle_Params("2", "-1", dvec2(0, 130), dvec2(0, 0), 0.8, 20.0, 25.0),
	Particle_Params("3", "-1", dvec2(0, 80 ), dvec2(0, 0), 0.7, 30.0, 40.0)
};
```
Particle Parameters:
```cpp
struct Particle_Params {
	string name;
	string system_id;
	dvec2  center;
	dvec2  velocity;
	dvec2  acceleration;
	double restitution;
	double radius;
	double mass;
	double inertia;
	double angular_velocity;
	bool   colliding;
};
```
Main Simulation Functions:
```cpp
Particle::tick(const double& delta_time, const vec2& bounding_box) {
	apply_friction(delta_time * TIME_SCALE);
	update_position(delta_time * TIME_SCALE);
	handle_border_collision(bounding_box);
}

Particle::update_position(const double& delta_time) {
	params.acceleration += (GRAVITY * sqrt(params.mass)) * delta_time;
	params.velocity += params.acceleration;
	params.center += params.velocity * delta_time;
}
```