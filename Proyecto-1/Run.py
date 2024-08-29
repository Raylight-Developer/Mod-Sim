import subprocess

RUN = False

SYSTEM_COUNT = 4
SYSTEM_START = 0
SYSTEM_STOP  = 2
SHIFT                        = "1e-5"
DURATION                     = "10.0"
GRAVITY                      = "0 -9.81"
TIME_SCALE                   = "5.0"
SLIDING_FRICTION_COEFFICIENT = "0.3"
ROLLING_FRICTION_COEFFICIENT = "0.15"

if RUN:
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

with open("Outputs.html", "w", encoding="utf-8") as file:
	file.write("""
	<html>
	<head>
		<style>
			body {
				background-color: #121212;
				color: #e0e0e0;
				font-family: Arial, sans-serif;
				margin: 0;
				padding: 20px;
			}
			h1 {
				color: #bb86fc;
			}
			table {
				width: 100%;
				border-collapse: collapse;
				margin-bottom: 20px;
			}
			th, td {
				padding: 10px;
				text-align: center;
				border: 1px solid #444;
			}
			th {
				background-color: #1f1f1f;
				color: #e0e0e0;
			}
			td {
				background-color: #2c2c2c;
			}
			img {
				max-width: 100%;
				height: auto;
			}
		</style>
	</head>
	<body>
	""")

	for system in range(SYSTEM_START, SYSTEM_STOP):
		file.write(f"<h1>System {system}</h1>\n")
		file.write("<table>\n")
		file.write("<tr><th>Label</th>" + "".join([f"<th>Particle {i}</th>" for i in range(PARTICLE_START, PARTICLE_STOP)]) + "</tr>\n")
		file.write("<tr><td>Positions</td>" + "".join([f'<td><img src="./Outputs/Position_{system}_{i}.png" alt="Position {i}"></td>' for i in range(PARTICLE_START, PARTICLE_STOP)]) + "</tr>\n")
		file.write("<tr><td>Velocities</td>" + "".join([f'<td><img src="./Outputs/Velocity_{system}_{i}.png" alt="Velocity {i}"></td>' for i in range(PARTICLE_START, PARTICLE_STOP)]) + "</tr>\n")
		file.write("<tr><td>Angular Velocities</td>" + "".join([f'<td><img src="./Outputs/Angular_Velocity_{system}_{i}.png" alt="Angular Velocity {i}"></td>' for i in range(PARTICLE_START, PARTICLE_STOP)]) + "</tr>\n")
		file.write("<tr><td>Accelerations</td>" + "".join([f'<td><img src="./Outputs/Acceleration_{system}_{i}.png" alt="Acceleration {i}"></td>' for i in range(PARTICLE_START, PARTICLE_STOP)]) + "</tr>\n")
		file.write("</table>\n")
	file.write("""
	</body>
	</html>
	""")

with open("Outputs.md", "w", encoding="utf-8") as file:
	for system in range(SYSTEM_START, SYSTEM_STOP):
		file.write(f"# System {system}\n")
		file.write("| Label | " + " | ".join([f"Particle {i}" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write("| -- | " + " | ".join([":--:" for _ in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Positions| " + " | ".join([f"![img](./Outputs/Position_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Velocities | " + " | ".join([f"![img](./Outputs/Velocity_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Angular Velocities | " + " | ".join([f"![img](./Outputs/Angular_Velocity_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")
		file.write(f"| Accelerations | " + " | ".join([f"![img](./Outputs/Acceleration_{system}_{i}.png)" for i in range(PARTICLE_START, PARTICLE_STOP)]) + " |\n")