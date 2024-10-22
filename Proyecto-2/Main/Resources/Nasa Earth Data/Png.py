from PIL import Image

image = Image.open("Nasa Earth Data/Cloud Optical Thickness LUT.png")
image = image.convert('RGB')
width, height = image.size

if height != 1:
	raise ValueError("The image is not a single pixel in height.")

rgb_values = []
for x in range(width):
	rgb_values.append(image.getpixel((x, 0)))

print("\n".join([str(val)[1:-1].replace(",", "") for val in rgb_values]))