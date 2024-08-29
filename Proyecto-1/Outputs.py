SYSTEM_START = 0
SYSTEM_STOP = 2
PARTICLE_START = 0
PARTICLE_STOP = 4

with open("Outputs.md", "w", encoding="utf-8") as file:
	for system in range(SYSTEM_START, SYSTEM_STOP):
		file.write(f"# System {system}\n")
		file.write("| Label | " + " | ".join([f"Particle {i}" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write("| -- | " + " | ".join([":--:" for _ in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Positions| " + " | ".join([f"![img](Outputs/Position_{system}__{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Velocities | " + " | ".join([f"![img](Outputs/Velocity_{system}__{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Angular Velocities | " + " | ".join([f"![img](Outputs/Angular_Velocity_{system}__{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Accelerations | " + " | ".join([f"![img](Outputs/Acceleration_{system}__{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write("\n")