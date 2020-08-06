from flask import Flask, render_template, url_for, request, session, redirect, flash
import random
import os

app = Flask(__name__)
app.config.update(
	SECRET_KEY = eval(os.environ.get("SECRET_KEY")),
	API_KEY = os.environ.get("API_KEY")
)

@app.route('/')
def mainPage():
	return render_template('home.html', API_KEY = app.config["API_KEY"])

@app.route('/fetch-data', methods = ["POST", "GET"])
def get_data():
	lat = random.randint(0, 1e10) / 1e10 * 180 - 90
	lon = random.randint(0, 1e10) / 1e10 * 360 - 180
	return f'{{"lat": {lat}, "long": {lon}}}'

