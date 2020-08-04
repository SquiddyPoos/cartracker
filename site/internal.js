function onClick() {
	fetch("/fetch-data")
	.then(function(data) {
		document.getElementById("Content").innerHTML = data;
	})
}