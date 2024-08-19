import matplotlib.pyplot as plt

time = 0
position = 10
velocity = 0
gravity = -9.81
restitution = 0.7
delta = 0.01

t = []
y = []

while True:
	t.append(time)
	y.append(position)

	velocity += gravity * delta
	position += velocity * delta

	if position < 0:
		position = 0
		velocity = -velocity * restitution
		if abs(velocity) < 0.1: break # Rest

	time += delta

plt.plot(t, y)
plt.show()
