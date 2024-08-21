#pragma once

#include "Include.hpp"

#define TIME_SCALE 5.0
#define GRAVITY dvec2(0.0, -9.81)
#define SLIDING_FRICTION_COEFFICIENT 0.3
#define ROLLING_FRICTION_COEFFICIENT 0.15

struct Particle_Params {
	string name;
	string system_id;
	dvec2 center;
	dvec2 velocity;
	dvec2 acceleration;
	dvec1 restitution;
	dvec1 radius;
	dvec1 mass;
	dvec1 inertia;
	dvec1 angular_velocity;
	bool  colliding;

	Particle_Params(const string& name, const string& system_id, const dvec2& center, const dvec2& velocity, dvec1 restitution, dvec1 radius, dvec1 mass) :
		name(name),
		system_id(system_id),
		center(center),
		velocity(velocity), 
		restitution(restitution),
		radius(radius),
		mass(mass),
		acceleration(dvec2(0.0, 0.0)),
		inertia((2.0 / 5.0) * mass * radius * radius),
		angular_velocity(0.0), 
		colliding(false)
	{}
};

struct Particle : QGraphicsEllipseItem {
	Particle_Params params;

	Particle(const Particle_Params& params) :
		QGraphicsEllipseItem(params.center.x - params.radius, params.center.y - params.radius, 2.0*params.radius, 2.0*params.radius),
		params(params)
	{
		setBrush(QBrush(QColor("blue")));
		setPen(QPen(QColor("black"), 1.0));
	}

	void setCenter(const dvec2& new_center) {
		params.center = new_center;
		setRect(QRectF(params.center.x - params.radius, params.center.y - params.radius, params.radius * 2.0, params.radius * 2.0));
	}

	void tick(dvec1 delta_time, const QRectF& bounding_box) {
		apply_friction(delta_time * TIME_SCALE);
		update_position(delta_time * TIME_SCALE);
		handle_border_collision(bounding_box);
		setRect(QRectF(params.center.x - params.radius, params.center.y - params.radius, params.radius * 2.0, params.radius * 2.0));
	}

	void apply_friction(dvec1 delta_time) {
		params.acceleration = dvec2(0.0, 0.0);
		if (params.colliding) {
			if (abs(params.velocity.x) < abs(params.angular_velocity * params.radius)) {
				dvec1 angular_friction = ROLLING_FRICTION_COEFFICIENT * params.angular_velocity;
				params.angular_velocity -= angular_friction * delta_time;
				dvec1 linear_friction = angular_friction * params.radius;
				params.acceleration.x = linear_friction * delta_time;
			} else {
				dvec1 friction_force = SLIDING_FRICTION_COEFFICIENT * params.mass * (-GRAVITY.y);
				dvec1 friction_acceleration = friction_force / params.mass;
				dvec2 friction_vector(-params.velocity.x, -params.velocity.y);
				friction_vector *= (friction_acceleration / length(params.velocity));
				params.acceleration = friction_vector * delta_time;
			}
		}
	}

	void update_position(dvec1 delta_time) {
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
		dvec1 distance = length(params.center - other->params.center);
		return distance < (params.radius + other->params.radius);
	}

	void resolve_overlap(Particle* other) {
		dvec2 distance_vector = other->params.center - params.center;
		dvec1 distance = length(distance_vector);
		dvec1 overlap = (params.radius + other->params.radius) - distance;

		if (overlap > 0) {
			dvec2 direction = distance_vector / distance;
			dvec2 correction = direction * (overlap / 2.0);

			params.center -= correction;
			other->params.center += correction;
		}
	}

	void handle_particle_collision(Particle* other) {
		if (detect_collision(other)) {
			params.colliding = true;
			resolve_overlap(other);

			dvec2 collision_normal = other->params.center - params.center;
			collision_normal = glm::normalize(collision_normal);

			dvec2 relative_velocity = other->params.velocity - params.velocity;
			dvec1 velocity_along_normal = glm::dot(relative_velocity, collision_normal);

			if (velocity_along_normal > 0.0) return;

			dvec1 restitution = min(this->params.restitution, other->params.restitution);
			dvec1 impulse_scalar = -(1.0 + restitution) * velocity_along_normal;
			impulse_scalar /= (1.0 / params.mass + 1.0 / other->params.mass);

			dvec2 impulse = impulse_scalar * collision_normal;
			params.velocity -= (1.0 / params.mass) * impulse;
			other->params.velocity += (1.0 / other->params.mass) * impulse;
		} else {
			params.colliding = false;
		}
	}
};