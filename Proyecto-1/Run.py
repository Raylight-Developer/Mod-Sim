import subprocess

RUN = True
WRITE_LOG = True
WRITE_PARAMS = True
GENERATE_GRAPHICS = True

SYSTEM_COUNT                 = 8

SHIFT_INDEX                  = 1
SHIFT                        = ["1e-12", "0"]
GRAVITY                      = [0, -9.81]
SLIDING_FRICTION_COEFFICIENT = 0.3
ROLLING_FRICTION_COEFFICIENT = 0.15
PARTICLE_OPACITY             = 0.2
SIMULATION_BOUNDS            = [400, 800]

DURATION                     = 15.0
TIME_SCALE                   = 5.0
DURATION_STEPS               = 1500
DELTA_TIME                   = 0.005
ON_FRAME_READY               = False

"""PARAMETERS = [
"((0, 185), (0, 0), 0.8, 5.0 , 2.0 )",
"((0, 165), (0, 0), 0.8, 10.0, 10.0)",
"((0, 130), (0, 0), 0.8, 20.0, 25.0)",
"((0, 80 ), (0, 0), 0.7, 30.0, 40.0)"
]"""

SHIFT                        = ["1e-1", "0"]
PARAMETERS = [
"((0, 745), (-50, 25), 0.8, 5.0 , 5.0 )",
"((0, 710), (0, 0), 0.8, 15.0, 15.0)",
"((0, 655), (5, 5), 0.8, 25.0, 25.0)",
"((0, 580), (5, -10), 0.7, 35.0, 35.0)",
"((0, 485), (50, 10), 0.7, 45.0, 45.0)",
"((0, 370), (0, 0), 0.7, 55.0, 55.0)",
]

"""PARAMETERS = [
"((0, 745), (0, 0), 0.8, 5.0 , 5.0 )",
"((0, 710), (0, 0), 0.8, 15.0, 15.0)",
"((0, 655), (0, 0), 0.8, 25.0, 25.0)",
"((0, 580), (0, 0), 0.7, 35.0, 35.0)",
"((0, 485), (0, 0), 0.7, 45.0, 45.0)",
"((0, 370), (0, 0), 0.7, 55.0, 55.0)",
"((0, 235), (0, 0), 0.7, 65.0, 65.0)",
"((0, 80 ), (0, 0), 0.7, 75.0, 75.0)",
]"""


if SHIFT_INDEX > len(PARAMETERS) or SHIFT_INDEX < 0:
	print("Shift Index Out of Range")
	exit()

if WRITE_PARAMS:
	open("./Params.txt", "w", -1, "utf-8").write("\n".join(PARAMETERS))

if RUN:
	process = subprocess.run(["./x64/Release/Proyecto-1.exe",
		"--system-count", str(SYSTEM_COUNT),
		"--shift-index", str(SHIFT_INDEX),
		"--shift", SHIFT[0], SHIFT[1],
		"--duration", str(DURATION),
		"--gravity", str(GRAVITY[0]), str(GRAVITY[1]),
		"--sliding-friction", str(SLIDING_FRICTION_COEFFICIENT),
		"--rolling-friction", str(ROLLING_FRICTION_COEFFICIENT),
		"--time-scale", str(TIME_SCALE),
		"--particle-opacity", str(PARTICLE_OPACITY),
		"--bounds", str(SIMULATION_BOUNDS[0]), str(SIMULATION_BOUNDS[1]),
		"--generate-graphics", str(int(GENERATE_GRAPHICS)),
		"--generate-tick-graphics", str(int(GENERATE_GRAPHICS)),
		"--delta-step", str(DELTA_TIME),
		"--duration-steps", str(int(DURATION_STEPS)),
		"--realtime", str(int(ON_FRAME_READY))
	])