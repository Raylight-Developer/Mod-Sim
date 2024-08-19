import matplotlib.pyplot as plt
import math

# Simulation parameters
time = 0
theta = math.pi / 4  # Initial angle (45 degrees)
omega = 0  # Angular velocity
length = 1  # Length of the pendulum (in meters)
gravity = 9.81  # Acceleration due to gravity (in m/s^2)
delta = 0.01  # Time step (in seconds)
damping = 0.2  # Damping coefficient (for simulating air resistance)

# Lists to store time and angle data
t = []
angles = []

while time < 10:  # Simulate for 10 seconds
    t.append(time)
    angles.append(theta)
    
    # Angular acceleration (in radians/s^2)
    alpha = -(gravity / length) * math.sin(theta) - damping * omega
    
    # Update angular velocity and angle
    omega += alpha * delta
    theta += omega * delta
    
    # Update time
    time += delta

# Plotting the results
plt.plot(t, angles)
plt.xlabel('Time (s)')
plt.ylabel('Angle (rad)')
plt.title('Pendulum Simulation')
plt.show()
