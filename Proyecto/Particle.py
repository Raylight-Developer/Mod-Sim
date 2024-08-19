from PySide6.QtWidgets import *
from PySide6.QtCore import *
from PySide6.QtGui import *
from typing import *
import math
import sys
import os

TIME_SCALE = 5
GRAVITY = QPointF(0, -9.81)
SLIDING_FRICTION_COEFFICIENT = 0.3
ROLLING_FRICTION_COEFFICIENT = 0.15

def length(point: QPointF):
	return math.sqrt(point.x()**2 + point.y()**2)

def normalize_point(point: QPointF):
	length = math.sqrt(point.x()**2 + point.y()**2)
	if length == 0:
		return QPointF(0, 0)
	normalized_point = QPointF(point.x() / length, point.y() / length)
	return normalized_point

def dot_product(p1: QPointF, p2: QPointF):
	return p1.x() * p2.x() + p1.y() * p2.y()

class Particle(QGraphicsEllipseItem):
	name: str
	center: QPointF
	velocity: QPointF
	acceleration: QPointF
	restitution: float
	radius: float
	mass: float
	inertia: float
	angular_velocity: float
	colliding: bool
	positions: List[Tuple[float,float]]
	velocities: List[Tuple[float,float]]
	accelerations: List[Tuple[float,float]]
	def __init__(self, name: str, center: QPointF, velocity: QPointF, restitution: float, radius: float, mass: float):
		super().__init__(center.x() - radius, center.y() - radius, 2*radius, 2*radius)
		self.setBrush(QBrush(QColor("blue")))
		self.setPen(QPen(QColor("black"), 1))
		self.name = name
		self.center = center
		self.velocity = velocity
		self.acceleration = QPointF(0,0)
		self.restitution = restitution
		self.radius = radius
		self.mass = mass
		self.inertia = (2 / 5) * mass * radius ** 2
		self.angular_velocity = 0.0
		self.colliding = False
		self.positions = []
		self.velocities = []
		self.accelerations = []

	def tick(self, delta_time: float, bounding_box: QRectF):
		self.apply_friction(delta_time * TIME_SCALE)
		self.update_position(delta_time * TIME_SCALE)
		self.handle_border_collision(bounding_box)
		self.setRect(QRectF(self.center.x() - self.radius, self.center.y() - self.radius, self.radius*2, self.radius*2))

		self.positions.append((self.center.x(), self.center.y()))
		self.velocities.append((self.velocity.x(), self.velocity.y()))
		self.accelerations.append((self.acceleration.x(), self.acceleration.y()))

	def apply_friction(self, delta_time):
		self.acceleration = QPointF(0,0)
		if self.colliding:
			if abs(self.velocity.x()) < abs(self.angular_velocity * self.radius):
				angular_friction = ROLLING_FRICTION_COEFFICIENT * self.angular_velocity
				self.angular_velocity -= angular_friction * delta_time
				linear_friction = angular_friction * self.radius
				self.acceleration.setX(linear_friction * delta_time)
			else:
				friction_force = SLIDING_FRICTION_COEFFICIENT * self.mass * (-GRAVITY.y())
				friction_acceleration = friction_force / self.mass
				friction_vector = QPointF(-self.velocity.x(), -self.velocity.y())
				friction_vector = friction_vector * (friction_acceleration / self.velocity.manhattanLength())
				self.acceleration = friction_vector * delta_time

	def update_position(self, delta_time):
		self.acceleration += (GRAVITY * math.sqrt(self.mass)) * delta_time
		self.velocity += self.acceleration
		self.center += self.velocity * delta_time

	def handle_border_collision(self, bounding_box: QRectF):
		if self.center.x() - self.radius < bounding_box.left():
			self.center.setX(bounding_box.left() + self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)
			self.colliding = True
		elif self.center.x() + self.radius > bounding_box.right():
			self.center.setX(bounding_box.right() - self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)
			self.colliding = True
		else:
			self.colliding = False
		if self.center.y() - self.radius < bounding_box.top():
			self.center.setY(bounding_box.top() + self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)
			self.colliding = True
		elif self.center.y() + self.radius > bounding_box.bottom():
			self.center.setY(bounding_box.bottom() - self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)
			self.colliding = True
		else:
			self.colliding = False

	def detect_collision(self, other: 'Particle'):
		distance = length(self.center - other.center)
		return distance < (self.radius + other.radius)

	def resolve_overlap(self, other: 'Particle'):
		distance_vector = other.center - self.center
		distance = length(distance_vector)

		overlap = (self.radius + other.radius) - distance
		
		if overlap > 0:
			direction = distance_vector / distance

			correction = direction * (overlap / 2)

			self.center -= correction
			other.center += correction

	def handle_particle_collision(self, other: 'Particle'):
		if self.detect_collision(other):
			self.colliding = True
			self.resolve_overlap(other)

			collision_normal = other.center - self.center
			collision_normal = normalize_point(collision_normal)

			relative_velocity = other.velocity - self.velocity

			velocity_along_normal = dot_product(relative_velocity, collision_normal)

			if velocity_along_normal > 0:
				return

			restitution = min(self.restitution, other.restitution)

			impulse_scalar = -(1 + restitution) * velocity_along_normal
			impulse_scalar /= (1 / self.mass + 1 / other.mass)

			impulse = impulse_scalar * collision_normal
			self.velocity -= (1 / self.mass) * impulse
			other.velocity += (1 / other.mass) * impulse
		else:
			self.colliding = False