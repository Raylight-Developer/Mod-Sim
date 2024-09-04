import subprocess

RUN = True
WRITE_LOG = False
WRITE_PARAMS = False

SYSTEM_COUNT                 = 64
SYSTEM_START                 = 0
SYSTEM_STOP                  = 4

SHIFT_INDEX                  = 1
SHIFT                        = ["1e-13", "0"]
DURATION                     = 25.0
GRAVITY                      = [0, -9.81]
SLIDING_FRICTION_COEFFICIENT = 0.3
ROLLING_FRICTION_COEFFICIENT = 0.15
TIME_SCALE                   = 5.0
PARTICLE_OPACITY             = 0.35
SIMULATION_BOUNDS            = [400, 800]

PARAMETERS = [
"((0, 185), (0, 0), 0.8, 5.0 , 2.0 )",
"((0, 165), (0, 0), 0.8, 10.0, 10.0)",
"((0, 130), (0, 0), 0.8, 20.0, 25.0)",
"((0, 80 ), (0, 0), 0.7, 30.0, 40.0)"
]

if SHIFT_INDEX > len(PARAMETERS) or SHIFT_INDEX < 0:
	print("Shift Index Out of Range")
	exit()

if WRITE_PARAMS:
	open("./Params.txt", "w", -1, "utf-8").write("\n".join(PARAMETERS))

if RUN:
	process = subprocess.run(["./x64/Release/Proyecto-1.exe",
		"--system-count", str(SYSTEM_COUNT),
		"--system-output-start", str(SYSTEM_START),
		"--system-output-end", str(SYSTEM_STOP + 1),
		"--shift-index", str(SHIFT_INDEX),
		"--shift", SHIFT[0], SHIFT[1],
		"--duration", str(DURATION),
		"--gravity", str(GRAVITY[0]), str(GRAVITY[1]),
		"--sliding-friction", str(SLIDING_FRICTION_COEFFICIENT),
		"--rolling-friction", str(ROLLING_FRICTION_COEFFICIENT),
		"--time-scale", str(TIME_SCALE),
		"--particle-opacity", str(PARTICLE_OPACITY),
		"--bounds", str(SIMULATION_BOUNDS[0]), str(SIMULATION_BOUNDS[1]),
		"--generate-graphics", str(int(WRITE_LOG))
	])

if WRITE_LOG:
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
			file.write("<tr><th>Label</th>" + "".join([f"<th>Particle {i}</th>" for i in range(len(PARAMETERS))]) + "</tr>\n")
			file.write("<tr><td>Positions</td>" + "".join([f'<td><img src="./Outputs/Position_{system}_{i}.png" alt="Position {i}"></td>' for i in range(len(PARAMETERS))]) + "</tr>\n")
			file.write("<tr><td>Velocities</td>" + "".join([f'<td><img src="./Outputs/Velocity_{system}_{i}.png" alt="Velocity {i}"></td>' for i in range(len(PARAMETERS))]) + "</tr>\n")
			file.write("<tr><td>Angular Velocities</td>" + "".join([f'<td><img src="./Outputs/Angular_Velocity_{system}_{i}.png" alt="Angular Velocity {i}"></td>' for i in range(len(PARAMETERS))]) + "</tr>\n")
			file.write("<tr><td>Accelerations</td>" + "".join([f'<td><img src="./Outputs/Acceleration_{system}_{i}.png" alt="Acceleration {i}"></td>' for i in range(len(PARAMETERS))]) + "</tr>\n")
			file.write("</table>\n")
		file.write("""
		</body>
		</html>
		""")

	with open("Outputs.md", "w", encoding="utf-8") as file:
		for system in range(SYSTEM_START, SYSTEM_STOP):
			file.write(f"# System {system}\n")
			file.write("| Label | " + " | ".join([f"Particle {i}" for i in range(len(PARAMETERS))]) + " |\n")
			file.write("| -- | " + " | ".join([":--:" for _ in range(len(PARAMETERS))]) + " |\n")
			file.write(f"| Positions| " + " | ".join([f"![img](./Outputs/Position_{system}_{i}.png)" for i in range(len(PARAMETERS))]) + " |\n")
			file.write(f"| Velocities | " + " | ".join([f"![img](./Outputs/Velocity_{system}_{i}.png)" for i in range(len(PARAMETERS))]) + " |\n")
			file.write(f"| Angular Velocities | " + " | ".join([f"![img](./Outputs/Angular_Velocity_{system}_{i}.png)" for i in range(len(PARAMETERS))]) + " |\n")
			file.write(f"| Accelerations | " + " | ".join([f"![img](./Outputs/Acceleration_{system}_{i}.png)" for i in range(len(PARAMETERS))]) + " |\n")