from PySide6.QtWidgets import *
from PySide6.QtCore import *
from PySide6.QtGui import *
from typing import *
import random
import math
import sys

GRAVITY = QPointF(0, -9.81)

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
	center: QPointF
	velocity: QPointF
	restitution: float
	radius: float
	mass: float
	def __init__(self, center: QPointF, velocity: QPointF, restitution: float, radius: float, mass: float):
		self.center = center
		self.velocity = velocity
		self.restitution = restitution
		self.radius = radius
		self.mass = mass
		super().__init__(center.x() - radius, center.y() - radius, 2*radius, 2*radius)
		self.setBrush(QBrush(QColor("blue")))
		self.setPen(QPen(QColor("black"), 1))

	def tick(self, delta_time: float, bounding_box: QRectF):
		self.update_position(delta_time)
		self.handle_border_collision(bounding_box)
		self.setRect(QRectF(self.center.x() - self.radius, self.center.y() - self.radius, self.radius*2, self.radius*2))

	def update_position(self, delta_time):
		self.velocity += GRAVITY * self.mass * delta_time
		self.center += self.velocity * delta_time

	def handle_border_collision(self, bounding_box: QRectF):
		if self.center.x() - self.radius < bounding_box.left():
			self.center.setX(bounding_box.left() + self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)
		elif self.center.x() + self.radius > bounding_box.right():
			self.center.setX(bounding_box.right() - self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)
		if self.center.y() - self.radius < bounding_box.top():
			self.center.setY(bounding_box.top() + self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)
		elif self.center.y() + self.radius > bounding_box.bottom():
			self.center.setY(bounding_box.bottom() - self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)

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