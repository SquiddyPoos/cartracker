function onClick() {
	fetch('./fetch-data')
	.then(response => response.json())
	.then(function(data) {
		document.getElementById("content").innerHTML = document.getElementById("content").innerHTML + "<br />" + data.lat + ", " + data.long;
	})
}
