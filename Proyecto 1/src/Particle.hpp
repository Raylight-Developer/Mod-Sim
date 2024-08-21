#pragma once

#include "Include.hpp"

#define TIME_SCALE 5.0f
#define GRAVITY vec2(0.0f, -9.81f)
#define SLIDING_FRICTION_COEFFICIENT 0.3f
#define ROLLING_FRICTION_COEFFICIENT 0.15f

struct Particle_Params {
	string name;
	string system_id;
	vec2 center;
	vec2 velocity;
	vec2 acceleration;
	float restitution;
	float radius;
	float mass;
	float inertia;
	float angular_velocity;
	bool  colliding;

	Particle_Params(const string& name, const string& system_id, const vec2& center, const vec2& velocity, float restitution, float radius, float mass) :
		name(name),
		system_id(system_id),
		center(center),
		velocity(velocity), 
		restitution(restitution),
		radius(radius),
		mass(mass),
		acceleration(vec2(0.0f, 0.0f)),
		inertia((2.0f / 5.0f) * mass * radius * radius),
		angular_velocity(0.0f), 
		colliding(false)
	{}
};

struct Particle : QGraphicsEllipseItem {
	Particle_Params params;

	Particle(const Particle_Params& params) :
		QGraphicsEllipseItem(params.center.x - params.radius, params.center.y - params.radius, 2.0f*params.radius, 2.0f*params.radius),
		params(params)
	{
		setBrush(QBrush(QColor("blue")));
		setPen(QPen(QColor("black"), 1.0));
	}

	void setCenter(const vec2& new_center) {
		params.center = new_center;
		setRect(QRectF(params.center.x - params.radius, params.center.y - params.radius, params.radius * 2.0f, params.radius * 2.0f));
	}

	void tick(float delta_time, const QRectF& bounding_box) {
		apply_friction(delta_time * TIME_SCALE);
		update_position(delta_time * TIME_SCALE);
		handle_border_collision(bounding_box);
		setRect(QRectF(params.center.x - params.radius, params.center.y - params.radius, params.radius * 2.0f, params.radius * 2.0f));
	}

	void apply_friction(float delta_time) {
		params.acceleration = vec2(0.0f, 0.0f);
		if (params.colliding) {
			if (abs(params.velocity.x) < abs(params.angular_velocity * params.radius)) {
				float angular_friction = ROLLING_FRICTION_COEFFICIENT * params.angular_velocity;
				params.angular_velocity -= angular_friction * delta_time;
				float linear_friction = angular_friction * params.radius;
				params.acceleration.x = linear_friction * delta_time;
			} else {
				float friction_force = SLIDING_FRICTION_COEFFICIENT * params.mass * (-GRAVITY.y);
				float friction_acceleration = friction_force / params.mass;
				vec2 friction_vector(-params.velocity.x, -params.velocity.y);
				friction_vector *= (friction_acceleration / length(params.velocity));
				params.acceleration = friction_vector * delta_time;
			}
		}
	}

	void update_position(float delta_time) {
		params.acceleration += (GRAVITY * sqrt(params.mass)) * delta_time;
		params.velocity += params.acceleration;
		params.center += params.velocity * delta_time;
	}

	void handle_border_collision(const QRectF& bounding_box) {
		if (params.center.x - params.radius < bounding_box.left()) {
			params.center.x = (bounding_box.left() + params.radius);
			params.velocity.x = (-params.velocity.x * params.restitution);
			params.colliding = true;
		} else if (params.center.x + params.radius > bounding_box.right()) {
			params.center.x = (bounding_box.right() - params.radius);
			params.velocity.x = (-params.velocity.x * params.restitution);
			params.colliding = true;
		} else {
			params.colliding = false;
		}

		if (params.center.y - params.radius < bounding_box.top()) {
			params.center.y = (bounding_box.top() + params.radius);
			params.velocity.y = (-params.velocity.y * params.restitution);
			params.colliding = true;
		} else if (params.center.y + params.radius > bounding_box.bottom()) {
			params.center.y = (bounding_box.bottom() - params.radius);
			params.velocity.y = (-params.velocity.y * params.restitution);
			params.colliding = true;
		} else {
			params.colliding = false;
		}
	}

	bool detect_collision(const Particle* other) const {
		float distance = length(params.center - other->params.center);
		return distance < (params.radius + other->params.radius);
	}

	void resolve_overlap(Particle* other) {
		vec2 distance_vector = other->params.center - params.center;
		float distance = length(distance_vector);
		float overlap = (params.radius + other->params.radius) - distance;

		if (overlap > 0) {
			vec2 direction = distance_vector / distance;
			vec2 correction = direction * (overlap / 2.0f);

			params.center -= correction;
			other->params.center += correction;
		}
	}

	void handle_particle_collision(Particle* other) {
		if (detect_collision(other)) {
			params.colliding = true;
			resolve_overlap(other);

			vec2 collision_normal = other->params.center - params.center;
			collision_normal = glm::normalize(collision_normal);

			vec2 relative_velocity = other->params.velocity - params.velocity;
			float velocity_along_normal = glm::dot(relative_velocity, collision_normal);

			if (velocity_along_normal > 0.0f) return;

			float restitution = min(this->params.restitution, other->params.restitution);
			float impulse_scalar = -(1.0f + restitution) * velocity_along_normal;
			impulse_scalar /= (1.0f / params.mass + 1.0f / other->params.mass);

			vec2 impulse = impulse_scalar * collision_normal;
			params.velocity -= (1.0f / params.mass) * impulse;
			other->params.velocity += (1.0f / other->params.mass) * impulse;
		} else {
			params.colliding = false;
		}
	}
};