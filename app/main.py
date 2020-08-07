from flask import Flask, render_template, url_for, request, session, redirect, flash
import random
import os
import ast
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
	lat = random.randint(0, 1e10) / 1e10 * 180 - 90
	lon = random.randint(0, 1e10) / 1e10 * 360 - 180
	return f'{{"lat": {lat}, "long": {lon}}}'

@app.route('/send-data', methods = ["POST"])
def process_gps_data():
	print(request.headers.get("id"), request.headers.get("gps_lat"), request.headers.get("gps_long"))
	recv_id = request.headers.get("id")
	gps_lat = request.headers.get("gps_lat")
	gps_long = request.headers.get("gps_long")
	curs.execute("INSERT INTO gps_data(id,lat,long) VALUES (%s, %s, %s)", (recv_id, gps_lat, gps_long))
	return "OK"
