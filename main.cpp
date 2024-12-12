#include <Arduino.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>
#include <Adafruit_AHTX0.h>
#include <FS.h>
#include "SPIFFS.h"
#include "SparkFunLSM6DSO.h"
#include <Wire.h>

const int BUZZ_PIN = 13;
const int BUTTON_PIN = 17;
const int RESET_PIN = 2;

#define SDA_ACCEL 33
#define SCL_ACCEL 32
#define SDA_TEMP 21
#define SCL_TEMP 22
#define FALL_THRESH 570
//Just change to the current AWS server IP
#define SERVER_IP "3.129.44.38"

void buzz(int pin);
void buzz_off(int pin);
void start_accel();
void start_temp();
float detect_fall();
void read_gyro();
void read_accel();
void read_temp();
void read_temp_hum();
void calibrate_fall();
void connect_internet();
void send_message_cloud(char* payload);
float max(float a, float b, float c);

esp_err_t err;
TwoWire I2C_ACCEL = TwoWire(0);
TwoWire I2C_TEMP = TwoWire(1);
Adafruit_AHTX0 aht;
LSM6DSO myIMU;
sensors_event_t humidity, temp;
char ssid[50]; 
char pass[50]; 
const char kHostname[] = "worldtimeapi.org";
const char kPath[] = "/api/timezone/Europe/London.txt";
const int kNetworkTimeout = 30 * 1000;
const int kNetworkDelay = 1000;
char payload[100];

void nvs_access() {
  // Initialize NVS
   err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open
  Serial.printf("\n");
  Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) 
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  } 
  else 
  {
    Serial.printf("Done\n");
    Serial.printf("Retrieving SSID/PASSWD\n");
    size_t ssid_len;
    size_t pass_len;
    err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
    err |= nvs_get_str(my_handle, "pass", pass, &pass_len);
    switch (err) 
    {
      case ESP_OK:
      Serial.printf("Done\n");
      //Serial.printf("SSID = %s\n", ssid);
      //Serial.printf("PASSWD = %s\n", pass);
      break;
      case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("The value is not initialized yet!\n");
      break;
      default:
      Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }
  // Close
  nvs_close(my_handle);
}

void setup()
{
  Serial.begin(9600);
  delay(1000);
  nvs_access();
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  //Sets up the I2C pins
  I2C_ACCEL.begin(SDA_ACCEL, SCL_ACCEL, 10000000);
  I2C_TEMP.begin(SDA_TEMP, SCL_TEMP, 100000);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RESET_PIN, INPUT);
  start_temp();
  start_accel();
  Serial.print("Ready to Go!");
}


void loop() 
{
    aht.getEvent(&humidity, &temp);
    float fall_data = detect_fall();
    if (fall_data)
    {
        sprintf(payload, "/?Temp=%.2f&Humidity=%.2f&FallData=%.2f&Message=%s", temp.temperature, humidity.relative_humidity,fall_data,"fall");
        buzz(BUZZ_PIN);
        send_message_cloud(payload);
    }
    else if(digitalRead(BUTTON_PIN) == HIGH)
    {
        sprintf(payload, "/?Temp=%.2f&Humidity=%.2f&FallData=0&Message=%s", temp.temperature, humidity.relative_humidity,"manual_alert");
        buzz(BUZZ_PIN);
        send_message_cloud(payload);
    }
    else if((temp.temperature > 32) or (humidity.relative_humidity > 70) or (temp.temperature < 19))
    {
        sprintf(payload, "/?Temp=%.2f&Humidity=%.2f&FallData=0&Message=%s", temp.temperature, humidity.relative_humidity,"dangerous_condition");
        buzz(BUZZ_PIN);
        send_message_cloud(payload);
    }
    else if(digitalRead(RESET_PIN) == HIGH)
        buzz_off(BUZZ_PIN);
    
}

void send_message_cloud(char* payload)
{
    WiFiClient c;
    HttpClient http(c);
    // This is the important bit
    err = http.get(SERVER_IP,5000, payload, NULL);
    //err = http.get("3.145.160.206",5000, "/?Temp=100", NULL);

    if (err == 0) 
    {
        Serial.println("startedRequest ok");
        err = http.responseStatusCode();
        if (err >= 0) {
        Serial.print("Got status code: ");
        Serial.println(err);
        // Usually you'd check that the response code is 200 or a
        // similar "success" code (200-299) before carrying on,
        // but we'll print out whatever response we get
        err = http.skipResponseHeaders();
        if (err >= 0) 
        {
            int bodyLen = http.contentLength();
            Serial.print("Content length is: ");
            Serial.println(bodyLen);
            Serial.println();
            Serial.println("Body returned follows:");
            // Now we've got to the body, so we can print it out
            unsigned long timeoutStart = millis();
            char c;
            // Whilst we haven't timed out & haven't reached the end of the body
            while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) 
            {
                if (http.available()) 
                {
                    c = http.read();
                    // Print out this character
                    Serial.print(c);
                    bodyLen--;
                    // We read something, reset the timeout counter
                    timeoutStart = millis();
                } 
                else 
                {
                    // We haven't got any data, so let's pause to allow some to
                    // arrive
                    delay(kNetworkDelay);
                }
            }
        }
        else 
        {
            Serial.print("Failed to skip response headers: ");
            Serial.println(err);
        }
        } 
        else 
        {
            Serial.print("Getting response failed: ");
            Serial.println(err);
        }
    } 
    else 
    {
    Serial.print("Connect failed: ");
    Serial.println(err);
    }
    http.stop();
    delay(1000);
}

void buzz(int pin)
{
    tone(pin, 500);
    delay(10);
}

void buzz_off(int pin)
{
    noTone(pin);
}

//Starts the accerometer I2C protocol
void start_accel()
{
   if(myIMU.begin(0x6B, I2C_ACCEL))
    Serial.println("Ready.");
  else 
  { 
    Serial.println("Could not connect to IMU.");
    Serial.println("Freezing");
  }
  if( myIMU.initialize(BASIC_SETTINGS) )
    Serial.println("Loaded Settings.");
}

//Starts temp/humidity sensor I2C
void start_temp()
{
    if (! aht.begin(&I2C_TEMP)) 
    {
        Serial.println("Could not find AHT? Check wiring");
        while (1) 
            delay(10);
    }
    Serial.println("AHT10 or AHT20 found");
}


float detect_fall()
{
    float accel_x = abs(myIMU.readFloatGyroX());
    float accel_y = abs(myIMU.readFloatGyroY());
    float accel_z = abs(myIMU.readFloatGyroZ());
    if (accel_x > FALL_THRESH or accel_y > FALL_THRESH or accel_z > FALL_THRESH)
        return max(accel_x,accel_y,accel_z);
    else
        return 0;
}   

//Reads gyroscope 
void read_gyro()
{
    //THIS IS NOT A MISTAKE, FOR SOME REASON readFloatAccel is the gyroscope, they are backwards
    Serial.print("\nGyrocsope:\n");
    Serial.print(" X = ");
    Serial.println(myIMU.readFloatAccelX(), 3);
    Serial.print(" Y = ");
    Serial.println(myIMU.readFloatAccelY(), 3);
    Serial.print(" Z = ");
    Serial.println(myIMU.readFloatAccelZ(), 3);
}


//Reads accelerometer
void read_accel()
{
    //SAME HERE, FOR SOME REASON readFloatGyro is the accelerometer
    Serial.print("\nAccelerometer:\n");
    Serial.print(" X = ");
    Serial.println(myIMU.readFloatGyroX(), 3);
    Serial.print(" Y = ");
    Serial.println(myIMU.readFloatGyroY(), 3);
    Serial.print(" Z = ");
    Serial.println(myIMU.readFloatGyroZ(), 3);
}

void read_temp()
{
  Serial.print("\nThermometer:\n");
  Serial.print(" Degrees F = ");
  Serial.println(myIMU.readTempF(), 3);
}

//Reads temp and humidty
void read_temp_hum()
{
    aht.getEvent(&humidity, &temp);
    //sprintf(payload, "/?Temp=%.2f&Humidity=%.2f", temp.temperature, humidity.relative_humidity);
}
float max(float a, float b, float c) 
{
    float max_val = a; // Assume 'a' is the largest
    if (b > max_val) {
        max_val = b; // Update if 'b' is larger
    }
    if (c > max_val) {
        max_val = c; // Update if 'c' is larger
    }
    return max_val; // Return the largest value
}
