Alejandro Martinez - 21430

Samuel Argueta - 211024

Hansel López - 19026

# Simulation
[Run The Program](./Run.py)

### Simulation Parameters:
```python
# Params
--system-count
--time-scale
--duration

--shift
--shift-index

# Physics
--gravity
--sliding-friction
--rolling-friction

# Dislpay
--bounds
--particle-opacity

# Outputs
--generate-graphics
--system-output-start
--system-output-end

```
### Particle Parameters:
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
### Main Simulation Functions:
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