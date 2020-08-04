function onClick() {
	fetch('./fetch-data')
	.then(response => response.json())
	.then(function(data) {
		document.getElementById("content").innerHTML = data.lat + ", " + data.long;
	})
}
