#include <WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

//#define USE_SPI       // Uncomment this to use SPI

#define SERIAL_PORT Serial

#define SPI_PORT SPI // Your desired SPI port.       Used only when "USE_SPI" is defined
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1

#ifdef USE_SPI
ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
// ICM_20948_SPI prevSense; // prevSensor SPI object 
#else
ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object
// ICM_20948_I2C prevSense; // prevSensor I2C object 
#endif

#define MG_TO_MSS_CONVERSION 0.00980665

// WiFi
const char *ssid = SECRET_SSID; // Enter your WiFi name
const char *password = SECRET_PASS;  // Enter WiFi password

// MQTT Broker
// const char *mqtt_broker = "broker.emqx.io";
// const char *topic = "ece180d/test";
// const char *mqtt_username = "emqx";
// const char *mqtt_password = "public";
// const int mqtt_port = 1883;

// MQTT Broker
const char *mqtt_broker = "mqtt.eclipseprojects.io";
const char *topic = "ece180d/test";
// const char *mqtt_username = "emqx";
// const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

#define BUFFER_SIZE 16
#define DOF 9

float prevAccelX;
float prevSensorVals[DOF];
float prevIntegralVals[DOF];

float** senseBuf;
float** senseVelocityBuf;
int bufInd = 0;

float sensorReadingBuf[BUFFER_SIZE];

unsigned long prevMillis;
unsigned long currentMillis;

void printRawAGMT(ICM_20948_AGMT_t agmt);
void printPaddedInt16b(int16_t val);
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals);

#ifdef USE_SPI
void printScaledAGMT(ICM_20948_SPI *sensor);
bool detect_push(ICM_20948_SPI *sensor, float prevSensor);
bool detect_pull(ICM_20948_SPI *sensor, float prevSensor);

#else
void printScaledAGMT(ICM_20948_I2C *sensor);
bool detect_push(ICM_20948_I2C *sensor, float prevSensor);
bool detect_pull(ICM_20948_I2C *sensor, float prevSensor);

#endif

void setPrevArr(ICM_20948_I2C *sensor, float* prev_val);
void printSenseBuf(float** senseBuf);
void updateSenseBuf(float** senseBuf, float* prev_val, int* bufInd);

void setup() {
 // Set software serial baud to 115200;
 Serial.begin(115200);
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");
 //connecting to a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str())) { //, mqtt_username, mqtt_password)) {
         Serial.println("mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
 client.publish(topic, "Hi I'm ESP32 ^^f");
 client.subscribe(topic);

  #ifdef USE_SPI
    SPI_PORT.begin();
  #else
    WIRE_PORT.begin();
    WIRE_PORT.setClock(400000);
  #endif

  bool initialized = false;
  while (!initialized) {
    #ifdef USE_SPI
      myICM.begin(CS_PIN, SPI_PORT);
    #else
      myICM.begin(WIRE_PORT, AD0_VAL);
    #endif

    SERIAL_PORT.print(F("Initialization of the sensor returned: "));
    SERIAL_PORT.println(myICM.statusString());
    if (myICM.status != ICM_20948_Stat_Ok) {
      SERIAL_PORT.println("Trying again...");
      delay(500);
    } else {
      initialized = true;
    }
  }

  senseBuf = (float **)malloc(DOF * sizeof(float *));
  for (int r = 0; r < DOF; r++) {
      senseBuf[r] = (float *)malloc(BUFFER_SIZE * sizeof(float));
  }
  
  senseVelocityBuf = (float **)malloc(DOF * sizeof(float *));
  for (int r = 0; r < DOF; r++) {
      senseVelocityBuf[r] = (float *)malloc(BUFFER_SIZE * sizeof(float));
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop()
{
  
  if (myICM.dataReady())
  {
    myICM.getAGMT();         // The values are only updated when you call 'getAGMT'
                             //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    
    currentMillis = millis();
    // printScaledAGMT(&myICM);

    if (detect_push(&myICM, prevAccelX))
    {
      SERIAL_PORT.println("PUSH FORWARD DETECTED");
    }
    else if (detect_pull(&myICM, prevAccelX))
    {
      SERIAL_PORT.println("PULL BACK DETECTED");
    }
    else if (detect_down(senseBuf, senseVelocityBuf))
    {
      SERIAL_PORT.println("DOWN DETECTED");
    }
    else if (detect_up(senseBuf, senseVelocityBuf))
    {
      SERIAL_PORT.println("UP DETECTED");
    }
    else if (detect_left(senseBuf, senseVelocityBuf))
    {
      SERIAL_PORT.println("LEFT DETECTED");
    }    
    else if (detect_right(senseBuf, senseVelocityBuf))
    {
      SERIAL_PORT.println("RIGHT DETECTED");
    }    
    else 
    {
      SERIAL_PORT.println("IDLE");
    }

    // Create a circular buffer for values 
    setPrevArr(&myICM, prevSensorVals);
    updateSenseBuf(senseBuf, prevSensorVals, bufInd);

    // Take integral of circular buffer
    updateTimerBuf(sensorReadingBuf, bufInd, currentMillis, prevMillis);
    getIntegrals(senseBuf, prevIntegralVals, sensorReadingBuf);
    updateSenseBuf(senseVelocityBuf, prevIntegralVals, bufInd);

    // Print "accelerations and velocities"
    // printSenseBuf(senseBuf);
    // Serial.println("");
    // printSenseBuf(senseVelocityBuf);
    
    // Update times and buffer indecies
    prevMillis = currentMillis;
    updateBufferIndex(&bufInd);
    // delay(3000);
    // delay(1000);

    // Query the client for a recieved message
    client.loop();
  }
  else
  {
    SERIAL_PORT.println("Waiting for data");
    client.loop();
    delay(500);
  }
}

// Below here are some helper functions to print the data nicely!

float convertAccelValues(float val)
{
  return MG_TO_MSS_CONVERSION * val;
}

void printPaddedInt16b(int16_t val)
{
  if (val > 0)
  {
    SERIAL_PORT.print(" ");
    if (val < 10000)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 1000)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 100)
    {
      SERIAL_PORT.print("0");
    }
    if (val < 10)
    {
      SERIAL_PORT.print("0");
    }
  }
  else
  {
    SERIAL_PORT.print("-");
    if (abs(val) < 10000)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 1000)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 100)
    {
      SERIAL_PORT.print("0");
    }
    if (abs(val) < 10)
    {
      SERIAL_PORT.print("0");
    }
  }
  SERIAL_PORT.print(abs(val));
}

void printRawAGMT(ICM_20948_AGMT_t agmt)
{
  SERIAL_PORT.print("RAW. Acc [ ");
  printPaddedInt16b(agmt.acc.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.acc.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.acc.axes.z);
  SERIAL_PORT.print(" ], Gyr [ ");
  printPaddedInt16b(agmt.gyr.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.gyr.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.gyr.axes.z);
  SERIAL_PORT.print(" ], Mag [ ");
  printPaddedInt16b(agmt.mag.axes.x);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.mag.axes.y);
  SERIAL_PORT.print(", ");
  printPaddedInt16b(agmt.mag.axes.z);
  SERIAL_PORT.print(" ], Tmp [ ");
  printPaddedInt16b(agmt.tmp.val);
  SERIAL_PORT.print(" ]");
  SERIAL_PORT.println();
}

void printFormattedFloat(float val, uint8_t leading, uint8_t decimals)
{
  float aval = abs(val);

  if (val < 0)
  {
    SERIAL_PORT.print("-");
  }
  else
  {
    SERIAL_PORT.print(" ");
  }
  for (uint8_t indi = 0; indi < leading; indi++)
  {
    uint32_t tenpow = 0;
    if (indi < (leading - 1))
    {
      tenpow = 1;
    }
    for (uint8_t c = 0; c < (leading - 1 - indi); c++)
    {
      tenpow *= 10;
    }
    if (aval < tenpow)
    {
      SERIAL_PORT.print("0");
    }
    else
    {
      break;
    }
  }
  if (val < 0)
  {
    SERIAL_PORT.print(-val, decimals);
  }
  else
  {
    SERIAL_PORT.print(val, decimals);
  }
}

#ifdef USE_SPI
void printScaledAGMT(ICM_20948_SPI *sensor)
{
#else
void printScaledAGMT(ICM_20948_I2C *sensor)
{
#endif
  SERIAL_PORT.print("Scaled. Acc (mg) [ ");
  printFormattedFloat(convertAccelValues(sensor->accX()), 5, 2);
  char buf[10];
  snprintf(buf, 10, "%f", sensor->accX());  
  client.publish(topic, buf);
  SERIAL_PORT.print(", ");
  printFormattedFloat(convertAccelValues(sensor->accY()), 5, 2);
  SERIAL_PORT.print(", ");
  printFormattedFloat(convertAccelValues(sensor->accZ()), 5, 2);
  SERIAL_PORT.print(" ], Gyr (DPS) [ ");
  printFormattedFloat(sensor->gyrX(), 5, 2);
  SERIAL_PORT.print(", ");
  printFormattedFloat(sensor->gyrY(), 5, 2);
  SERIAL_PORT.print(", ");
  printFormattedFloat(sensor->gyrZ(), 5, 2);
  SERIAL_PORT.print(" ], Mag (uT) [ ");
  printFormattedFloat(sensor->magX(), 5, 2);
  SERIAL_PORT.print(", ");
  printFormattedFloat(sensor->magY(), 5, 2);
  SERIAL_PORT.print(", ");
  printFormattedFloat(sensor->magZ(), 5, 2);
  SERIAL_PORT.print(" ], Tmp (C) [ ");
  printFormattedFloat(sensor->temp(), 5, 2);
  SERIAL_PORT.print(" ]");
  SERIAL_PORT.println();
}

#ifdef USE_SPI
bool detect_push(ICM_20948_SPI *sensor, float prevSensor)
{
#else
bool detect_push(ICM_20948_I2C *sensor, float prevSensor)
{
#endif
  // Detect a positive change in acceleration in the X direction
  // Show no rotation, no lift/decent

  // Serial.print("AccelX: ");
  // Serial.println(sensor->accX());
  // delay(500);

  if (sensor->accX() > 350 && prevSensor < 350) {
    return true;
  }
  return false;
}



#ifdef USE_SPI
bool detect_pull(ICM_20948_SPI *sensor, float prevSensor)
{
#else
bool detect_pull(ICM_20948_I2C *sensor, float prevSensor)
{
#endif
  // Detect a negative change in acceleration in the X direction
  // Show no rotation, no lift/decent
  // Detect a positive change in acceleration in the X direction
  // Show no rotation, no lift/decent

  // Serial.print("AccelX: ");
  // Serial.println(sensor->accX());
  // delay(500);
  
  if (sensor->accX() < -350 && prevSensor > -350) {
    return true;
  }
  return false;
}

float bufMean(float** senseBuf, int dof)
{
  float mean = 0.0;
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    mean += senseBuf[dof][i];
  }
  return mean/BUFFER_SIZE;
}

bool detect_up(float** senseBuf, float** intBuf)
{
  if (bufMean(intBuf, 2) < 150.0)
  {
    return true;
  }
  return false;
}

bool detect_down(float** senseBuf, float** intBuf)
{
  if (bufMean(intBuf, 2) > 200.0)
  {
    return true;
  }
  return false;
}

bool detect_left(float** senseBuf, float** intBuf)
{
  if (bufMean(intBuf, 1) < -5.0)
  {
    return true;
  }
  return false;
}

bool detect_right(float** senseBuf, float** intBuf)
{
  if (bufMean(intBuf, 1) > 15.0)
  {
    return true;
  }
  return false;
}

void setPrevArr(ICM_20948_I2C *sensor, float* prev_val)
{
  prev_val[0] = convertAccelValues(sensor->accX());
  prev_val[1] = convertAccelValues(sensor->accY());
  prev_val[2] = convertAccelValues(sensor->accZ());
  prev_val[3] = sensor->gyrX();
  prev_val[4] = sensor->gyrY();
  prev_val[5] = sensor->gyrZ();
  prev_val[6] = sensor->magX();
  prev_val[7] = sensor->magY();
  prev_val[8] = sensor->magZ();
}

void updateTimerBuf(float* sensorReadingBuf, int bufInd, unsigned long curr, unsigned long prev)
{
  sensorReadingBuf[bufInd] = (curr - prev)/1000;
}

void getIntegrals(float** senseBuf, float* integrals, float* sensorReadingTimeBuf)
{
  unsigned long totalBufTime = 0;
  float totalSensorVal = 0;
  for (int i = 0; i < DOF; i++) 
  {
    for (int j = 0; j < BUFFER_SIZE; j++)
    {
      if (senseBuf[i][j] > 0.75)
      {
        totalSensorVal += senseBuf[i][j];
        totalBufTime += sensorReadingTimeBuf[j];
      }
    }
    integrals[i] = totalSensorVal + float(totalBufTime); 

    totalBufTime = 0;
    totalSensorVal = 0;
  }
}

void printSenseBuf(float** senseBuf)
{
  for (int i = 0; i < DOF; i++)
  {
    for (int j = 0; j < BUFFER_SIZE; j++)
    {
      Serial.print(senseBuf[i][j]);
      Serial.print(" ");
    }
    Serial.println(" ");
  }  
  Serial.println(" ");
}

// Update the total sensor value circular buffer
void updateSenseBuf(float** senseBuf, float* prev_val, int bufInd)
{
  for (int j = 0; j < DOF; j++)
  {
    senseBuf[j][bufInd] = prev_val[j];
  }
}

void updateBufferIndex(int* index)
{
  *index = (*index+1) % BUFFER_SIZE;
}