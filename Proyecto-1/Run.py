import subprocess

SYSTEM_COUNT = 4
SYSTEM_START = 0
SYSTEM_STOP  = 2
SHIFT                        = "1e-5"
DURATION                     = "10.0"
GRAVITY                      = "0 -9.81"
TIME_SCALE                   = "5.0"
SLIDING_FRICTION_COEFFICIENT = "0.3"
ROLLING_FRICTION_COEFFICIENT = "0.15"

process = subprocess.run(["./x64/Release/Proyecto-1.exe",
	"--system-count", str(SYSTEM_COUNT),
	"--system-output-start", str(SYSTEM_START),
	"--system-output-end", str(SYSTEM_STOP + 1),
	"--x-shift", SHIFT,
	"--duration", DURATION,
	"--gravity", GRAVITY.split()[0], GRAVITY.split()[1],
	"--sliding-friction", SLIDING_FRICTION_COEFFICIENT,
	"--rolling-friction", ROLLING_FRICTION_COEFFICIENT,
	"--time-scale", TIME_SCALE
])

PARTICLE_START = 0
PARTICLE_STOP = 4

with open("Outputs.md", "w", encoding="utf-8") as file:
	for system in range(SYSTEM_START, SYSTEM_STOP):
		file.write(f"# System {system}\n")
		file.write("| Label | " + " | ".join([f"Particle {i}" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write("| -- | " + " | ".join([":--:" for _ in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Positions| " + " | ".join([f"![img](./Outputs/Position_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Velocities | " + " | ".join([f"![img](./Outputs/Velocity_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Angular Velocities | " + " | ".join([f"![img](./Outputs/Angular_Velocity_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Accelerations | " + " | ".join([f"![img](./Outputs/Acceleration_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write("\n")