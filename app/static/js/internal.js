function fetch_coords() {
	fetch('./fetch-data', {method: 'POST', headers: {id: 1}})
	.then(response => response.json())
	.then(function(data) {
		if (data.lat == null) {
			document.getElementById("mapframe").class = "";
			document.getElementById("errormsg").class = "invisible";
			var new_src = document.getElementById("mapframe").src.replace(/&center=.+&/, "&center="+data.lat+","+data.long+"&");
			document.getElementById("mapframe").src = new_src;
		} else {
			document.getElementById("mapframe").class = "invisible";
			document.getElementById("errormsg").class = "";
			document.getElementById("errormsg").innerHTML = "No GPS data!";
		}
	})
}
