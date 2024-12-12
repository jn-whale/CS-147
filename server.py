from flask import Flask
from flask import request
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from datetime import datetime
import csv
import os

app = Flask(__name__)

def send_email(temp, humidity, message):
    from_email = "SECRET EMAIL USERNAME"
    from_password = "SECRET EMAIL PASSWORD"
    to_email = "CARETAKER'S EMAIL"

    temp_num = float(temp)
    humidity_num = float(humidity)
    msg = MIMEMultipart()
    msg['From'] = from_email
    msg['To'] = to_email
    msg['Subject'] = "AGE Alert!"

    if message == 'dangerous_condition':
        html_body = f"<b>Alert: Patient is in Dangerous Environment! </b><br> Please check in with the device owner. <br> Current Temperature is {temp}°C and Humidity is {humidity}%.<br> Dangerous environment levels recorded."
        msg.attach(MIMEText(html_body, 'html'))
    elif message == 'fall':
        html_body = f"<b>Alert: A possible fall is detected! </b><br> Please check in with the device owner. <br> Current Temperature is {temp}°C and Humidity is {humidity}%.<br> Health threatening conditions!"
        msg.attach(MIMEText(html_body, 'html'))
    elif message == 'manual_alert':
        html_body = f"<b>Alert: Patient requests help! </b><br> Please check in with the device owner. <br> Current Tempertaure is {temp}C and Humidity is {humidity}%.<br>Possible emergency!"
        msg.attach(MIMEText(html_body,'html'))


    try:
        server = smtplib.SMTP('email-smtp.us-east-2.amazonaws.com', 587)
        server.starttls()
        server.login("SMTP USERNAME", "SMTP PASSWORD")
        text = msg.as_string()
        server.sendmail(from_email, to_email, text)
        server.quit()
        return {"success": True, "message": "Email sent successfully!"}
    except Exception as e:
        return {"success": False, "message": str(e)}
def write_csv(fall_data,temp,humidity,message):
    date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    write_header = not os.path.exists('fall_history')
    with open('fall_history',mode='a',newline='') as file:
        writer = csv.writer(file)
        if write_header:
            writer.writerow(['date','fall_data','temperature','humidity','message'])
        writer.writerow([date,fall_data,temp,humidity,message])

@app.route("/",methods=["PUT","GET"])
def recieve_message():
    temp = request.args.get("Temp")
    humidity = request.args.get("Humidity")
    fall_data = request.args.get("FallData")
    message = request.args.get("Message")
    write_csv(fall_data,temp,humidity,message)
    print(send_email(temp,humidity,message))
    print(f"Temperature:{temp}, Humidity:{humidity},Message:{message}")
    return f"Temperature:{temp}, Humidity: {humidity}, Message:{message}"

if __name__ == "__main__":
    app.run(debug=True)

