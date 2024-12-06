from flask import Flask, request, jsonify
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

app = Flask(__name__)


def send_email(temp, humidity):
    from_email = "ageuci147@gmail.com"
    from_password = "anhr emfm eols kyjv"
    to_email = "cembabalik@yahoo.com.tr"
    
    temp_num = float(temp)
    humidity_num = float(humidity)
    msg = MIMEMultipart()
    msg['From'] = from_email
    msg['To'] = to_email
    msg['Subject'] = "AGE Alert!"
    
    
    if temp_num<40 and humidity_num<55:
        html_body = f"<b>Alert: A possible fall is detected! </b><br> Please check in with the device owner. <br> Current Temperature is {temp}°C and Humidity is {humidity}%.<br> Safe environment levels recorded."
        msg.attach(MIMEText(html_body, 'html'))
        
    else:
        html_body = f"<b>Alert: A possible fall is detected! </b><br> Please check in with the device owner. <br> Current Temperature is {temp}°C and Humidity is {humidity}%.<br> Hazardous environment levels recorded, health threatening conditions!"
        msg.attach(MIMEText(html_body, 'html'))


    try:
        server = smtplib.SMTP('smtp.gmail.com', 587)
        server.starttls()
        server.login(from_email, from_password)
        text = msg.as_string()
        server.sendmail(from_email, to_email, text)
        server.quit()
        return {"success": True, "message": "Email sent successfully!"}
    except Exception as e:
        return {"success": False, "message": str(e)}


@app.route('/data', methods=['GET'])
def get_data():
    temp = request.args.get('Temp')
    humidity = request.args.get('Humidity')
    
    
    email_result = send_email(temp, humidity)
    return jsonify(email_result), 200
    
        
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)
    

