from matplotlib import pyplot as plt
from Particle import *

PARAMETERS : List[Particle] = [
	# name, center, velocity, restitution, radius, mass
	Particle("0", QPointF(0.5, 180), QPointF(0, 0), 0.8, 5,  2  ),
	Particle("1", QPointF(0.5, 170), QPointF(0, 0), 0.8, 10, 10 ),
	Particle("2", QPointF(0.5, 130), QPointF(0, 0), 0.8, 20, 25 ),
	Particle("3", QPointF(0  , 80 ), QPointF(0, 0), 0.7, 30, 40 )
]

SIM_TIME = 7.5
BOUNDING_BOX = QRectF(-200, 0, 400, 600)

class ParticleSimulation(QGraphicsScene):
	deltas: List[float]
	particles: List[Particle]
	bounding_box: QRectF
	time = 0

	def __init__(self, parent: QMainWindow):
		super().__init__(parent)
		self.deltas = []
		self.particles = []
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
		self.time += delta_time
		self.deltas.append(self.time)

class MainWindow(QMainWindow):
	def __init__(self):
		super().__init__()
		self.showMaximized()
		self.view = QGraphicsView(self)
		self.simulation = ParticleSimulation(self)
		self.view.setScene(self.simulation)
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
		
		self.sim_timer = QTimer()
		self.sim_timer.timeout.connect(self.finish)
		self.sim_timer.start(SIM_TIME * 1000)

		self.elapsed_timer = QElapsedTimer()
		self.elapsed_timer.start()

		self.frame_count = 0

	def update_scene(self):
		delta_time = self.elapsed_timer.elapsed() / 1000.0
		self.elapsed_timer.restart()

		self.simulation.advance(delta_time)
		self.view.viewport().update()
		self.frame_count += 1

	def print_fps(self):
		fps = self.frame_count
		self.frame_count = 0
		print(f"FPS: {fps}")

	def finish(self):
		self.timer.stop()
		self.fps_timer.stop()

		with open("README.md", "w", -1, "utf-8") as file:
			file.write("Alejandro Martinez - 21430\n\nSamuel Argueta - 211024\n\nHansel López - 19026")
			for particle in self.simulation.particles:
				values_x = [val[0] for val in particle.velocities]
				values_y = [val[1] for val in particle.velocities]

				plt.plot(self.simulation.deltas, values_x, label='X')
				plt.plot(self.simulation.deltas, values_y, label='Y')

				plt.xlabel('Time')
				plt.ylabel('Velocity')
				plt.legend()
				plt.title('Velocities')
				plt.savefig(f'./outputs/Velocity_{particle.name}.png')
				plt.clf()

				values_x = [val[0] for val in particle.accelerations]
				values_y = [val[1] for val in particle.accelerations]

				plt.plot(self.simulation.deltas, values_x, label='X')
				plt.plot(self.simulation.deltas, values_y, label='Y')

				plt.xlabel('Time')
				plt.ylabel('Acceleration')
				plt.legend()
				plt.title('Accelerations')
				plt.savefig(f'./outputs/Acceleration_{particle.name}.png')
				plt.clf()

				values_x = [val[0] for val in particle.positions]
				values_y = [val[1] for val in particle.positions]

				plt.plot(self.simulation.deltas, values_x, label='X')
				plt.plot(self.simulation.deltas, values_y, label='Y')

				plt.xlabel('Time')
				plt.ylabel('Position')
				plt.legend()
				plt.title('Positions')
				plt.savefig(f'./outputs/Position_{particle.name}.png')
				
				file.write(f"\n\n# {particle.name}")
				file.write(f'\n\n![""](./outputs/Velocity_{particle.name}.png)')
				file.write(f'\n![""](./outputs/Acceleration_{particle.name}.png)')
				file.write(f'\n![""](./outputs/Position_{particle.name}.png)')
				plt.clf()

			file.write("")
		exit()

if __name__ == "__main__":
	if not os.path.exists("./outputs"): os.makedirs("outputs")
	app = QApplication(sys.argv)
	window = MainWindow()
	sys.exit(app.exec())