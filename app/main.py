from flask import Flask, render_template, url_for, request, session, redirect, flash
import random
import os
import ast
import json
import psycopg2

app = Flask(__name__)
app.config.update(
	SECRET_KEY = ast.literal_eval(str(os.environ.get("SECRET_KEY"))),
	API_KEY = str(os.environ.get("API_KEY")),
	DATABASE_URL = str(os.environ.get('DATABASE_URL'))
)

con = psycopg2.connect(app.config["DATABASE_URL"], sslmode = "require")
curs = con.cursor()

@app.route('/')
def mainPage():
	return render_template('home.html', API_KEY = app.config["API_KEY"])

@app.route('/fetch-data', methods = ["POST"])
def get_data():
	recv_id = request.headers.get("id")
	if (recv_id):
		gps_lat = None
		gps_long = None
		curs.execute("SELECT * FROM gps_data WHERE id = %s", (recv_id))
		data = curs.fetchall()
		if (len(data) != 0):
			gps_lat = data[0][1]
			gps_long = data[0][2]
		json_data = {
			lat: gps_lat, 
			long: gps_long
		}
		return json.dumps(json_data, indent = 4)
	else:
		return "NO ID PROVIDED"

@app.route('/send-data', methods = ["POST"])
def process_gps_data():
	print(request.headers.get("id"), request.headers.get("gps_lat"), request.headers.get("gps_long"))
	recv_id = request.headers.get("id")
	gps_lat = request.headers.get("gps_lat")
	gps_long = request.headers.get("gps_long")
	curs.execute("SELECT * FROM gps_data WHERE id = %s", (recv_id))
	if (len(curs.fetchall()) == 0):
		curs.execute("INSERT INTO gps_data(id,lat,long) VALUES (%s, %s, %s)", (recv_id, gps_lat, gps_long))
		con.commit()
	else:
		curs.execute("UPDATE gps_data SET lat = %s, long = %s WHERE id = %s", (gps_lat, gps_long, recv_id))
		con.commit()
	return "OK"
