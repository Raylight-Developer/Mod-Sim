import earthkit as ek

data = ek.data.from_source(
	'cds',
	'reanalysis-era5-single-levels',
	{
		'product_type': 'reanalysis',
		'variable': '2m_temperature',
		'year': '2023',
		'month': '07',
		'day': '3',
		'time': '12:00',
	},
)

ek.plots.globe(data)