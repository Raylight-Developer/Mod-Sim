from Particle import *

PARAMETERS : List[Particle] = [
	# center, velocity, restitution, radius, mass
	Particle(QPointF(0, 180), QPointF(0, 0), 0.9, 10, 1 ),
	Particle(QPointF(0, 150), QPointF(0, 0), 0.8, 20, 2 ),
	Particle(QPointF(0, 100), QPointF(0, 0), 0.7, 30, 4 )
]

BOUNDING_BOX = QRectF(-200, 0, 400, 600)

class ParticleSimulation(QGraphicsScene):
	def __init__(self, parent: QMainWindow):
		super().__init__(parent)
		self.particles: List[Particle] = []
		self.bounding_box = BOUNDING_BOX
		self.addRect(self.bounding_box)
		self.addLine(-400, 0, 400, 0)
		self.setup_particles()
		self.setSceneRect(BOUNDING_BOX)
	
	def setup_particles(self):
		for particle in PARAMETERS:
			self.addItem(particle)
			self.particles.append(particle)
	
	def update_particles(self, delta_time):
		for i, particle_a in enumerate(self.particles):
			particle_a.tick(delta_time, BOUNDING_BOX)
			for j, particle_b in enumerate(self.particles):
				if i < j:
					particle_a.handle_particle_collision(particle_b)
	
	def advance(self, delta_time):
		self.update_particles(delta_time)

class MainWindow(QMainWindow):
	def __init__(self):
		super().__init__()
		self.showMaximized()
		self.view = QGraphicsView(self)
		self.scene = ParticleSimulation(self)
		self.view.setScene(self.scene)
		self.setCentralWidget(self.view)

		transform = QTransform().scale(1.5, -1.5)
		self.view.translate(0, -self.view.height())
		self.view.setTransform(transform)

		self.timer = QTimer()
		self.timer.timeout.connect(self.update_scene)
		self.timer.start(0)

		self.fps_timer = QTimer()
		self.fps_timer.timeout.connect(self.print_fps)
		self.fps_timer.start(1000)

		self.elapsed_timer = QElapsedTimer()
		self.elapsed_timer.start()

		self.frame_count = 0

	def update_scene(self):
		delta_time = self.elapsed_timer.elapsed() / 1000.0
		self.elapsed_timer.restart()

		self.scene.advance(delta_time)
		self.view.viewport().update()
		self.frame_count += 1

	def print_fps(self):
		fps = self.frame_count
		self.frame_count = 0
		print(f"FPS: {fps}")
		
if __name__ == "__main__":
	app = QApplication(sys.argv)
	window = MainWindow()
	sys.exit(app.exec())