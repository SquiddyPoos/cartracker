function fetch_coords() {
	fetch('./fetch-data')
	.then(response => response.json())
	.then(function(data) {
		document.getElementById("mapframe").src.replace("/&q=.+/", "&q="+data.lat+"+"+data.long);
	})
}
