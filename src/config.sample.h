#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_SERVER ""
String baseTopic = "home/GarageSensor-1/";


// Setup Pins


#define PIRPIN    D6
#define DHTPIN    D7
#define DHTTYPE   DHT22
#define TRIGPIN   D5
#define ECHOPIN   D2


// Define Vars

long duration;
int distance;
char msg[30];
char charBuf[50];
char ipAdd[20];
char macAdd[30];
String topic;

long lastReconnectAttempt = 0;

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHTesp dht;

//  PIR

int pirValue;
int pirStatus;
String motionStatus;
String ip;
String mac;

double ftemp;
