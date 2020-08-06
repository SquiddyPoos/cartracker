from flask import Flask, render_template, url_for, request, session, redirect, flash
import random

app = Flask(__name__)

@app.route('/')
def mainPage():
	return render_template('home.html', data = "Hello, its a car tracker!")

@app.route('/fetch-data', methods = ["POST", "GET"])
def get_data():
	lat = random.randint(0, 1e10) / 1e10 * 180 - 90
	lon = random.randint(0, 1e10) / 1e10 * 260 - 180
	return f'{{"lat": {lat}, "long": {lon}}}'

