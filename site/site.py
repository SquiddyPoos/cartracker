from flask import Flask, render_template, url_for, request, session, redirect, flash

app = Flask(__name__)
app.config.from_object(__name__)

@app.route('/')
def mainPage():
	return render_template('index.html', data = "Hello, its a car tracker!")

if __name__ == "__main__":
	app.debug = True
	app.run(host='127.0.0.1',port=8080)
