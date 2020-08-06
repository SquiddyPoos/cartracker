function fetch_coords() {
	fetch('./fetch-data')
	.then(response => response.json())
	.then(function(data) {
		var new_src = document.getElementById("mapframe").src.replace(/&center=.+&/, "&center="+data.lat+","+data.long);
		document.getElementById("mapframe").src = new_src;
	})
}
