def read_act_file(file_path):
	color_dict = []

	with open(file_path, 'rb') as file:
		# Read the header (first 8 bytes)
		header = file.read(8)
		
		# Extract number of colors from the header
		num_colors = int.from_bytes(header[0:2], 'little')
		
		# Read the color entries
		for i in range(num_colors):
			# Each color entry consists of 3 bytes (RGB)
			rgb = file.read(3)
			if not rgb:
				break  # In case we don't have enough colors
			if len(rgb) == 3:
				r, g, b = rgb
				color_dict.append({
					'red': r,
					'green': g,
					'blue': b
				})
	return color_dict

act_file_path = 'Nasa Earth Data/Land Surface Temperature.act'

color_table_dict = read_act_file(act_file_path)
for color in color_table_dict:
	print(f"uvec3({color['red']}, {color['green']}, {color['blue']}),")
