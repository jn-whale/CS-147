from flask import Flask, render_template, send_file
import pandas as pd
import matplotlib.pyplot as plt
import io
import matplotlib.dates as mdates
from datetime import datetime

app = Flask(__name__)

file_path = "fall_history"

@app.route('/')
def index():
    data = pd.read_csv(file_path, parse_dates=['date'])
    data = data[data['fall_data'] > 400]
    plt.scatter(data['date'], data['fall_data'], color='blue', label='Fall Severity')
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d %H:%M"))
    plt.gca().xaxis.set_major_locator(mdates.AutoDateLocator())
    plt.xlabel('Date and Time')
    plt.ylabel('Fall Severity')
    plt.title('Fall Data Over Time')
    plt.grid(True)
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    
    img = io.BytesIO()
    plt.savefig(img, format='png')
    img.seek(0)
    plt.close()
    
    return send_file(img, mimetype='image/png')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)

