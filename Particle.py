from PySide6.QtWidgets import *
from PySide6.QtCore import *
from PySide6.QtGui import *
from typing import *
import random
import math
import sys

GRAVITY = QPointF(0, -9.81)

class Particle(QGraphicsEllipseItem):
	position: QPointF
	velocity: QPointF
	restitution: float
	radius: float
	mass: float
	def __init__(self, position: QPointF, velocity: QPointF, restitution: float, radius: float, mass: float):
		self.position = position - QPointF(radius, radius) / 2
		self.velocity = velocity
		self.restitution = restitution
		self.radius = radius
		self.mass = mass
		super().__init__(position.x(), position.y(), radius, radius)
		self.setBrush(QBrush(QColor("blue")))
		self.setPen(QPen(QColor("black"), 0))

	def tick(self, delta_time: float, bounding_box: QRectF):
		self.update_position(delta_time)
		#self.handle_border_collision(bounding_box)
		self.setPos(self.position)

	def update_position(self, delta_time):
		self.velocity += GRAVITY * self.mass * delta_time
		self.position += self.velocity * delta_time

	def handle_border_collision(self, scene_rect: QRectF):
		if self.position.x() - self.radius < scene_rect.left():
			self.position.setX(scene_rect.left() + self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)
		elif self.position.x() + self.radius > scene_rect.right():
			self.position.setX(scene_rect.right() - self.radius)
			self.velocity.setX(-self.velocity.x() * self.restitution)

		if self.position.y() - self.radius < scene_rect.top():
			self.position.setY(scene_rect.top() + self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)
		elif self.position.y() + self.radius > scene_rect.bottom():
			self.position.setY(scene_rect.bottom() - self.radius)
			self.velocity.setY(-self.velocity.y() * self.restitution)

	def handle_particle_collision(self, other: 'Particle'):
		distance = self.position - other.position
		distance_length = distance.manhattanLength()

		if distance_length < self.radius + other.radius:
			collision_normal = distance / distance_length
			relative_velocity = self.velocity - other.velocity
			velocity_along_normal = relative_velocity.dotProduct(collision_normal)

			if velocity_along_normal > 0:
				return

			restitution = min(self.restitution, other.restitution)
			impulse = -(1 + restitution) * velocity_along_normal
			impulse /= (1 / self.mass + 1 / other.mass)

			self.velocity += impulse / self.mass * collision_normal
			other.velocity -= impulse / other.mass * collision_normal