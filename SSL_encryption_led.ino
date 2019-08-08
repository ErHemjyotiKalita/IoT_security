#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define ESP8266_LED (5)
//#define RELAY_SIGNAL_PIN (4)
#define SERIAL_DEBUG

const char caCert[] PROGMEM = R"EOF(
-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIB9XIF0flLcjauBZbIpjq0l7c1fDwuMsoBltzibNuhLZcoeFlpuF4t
ZGC4dAdfqdFcfo25hw4Qr+iIaeNLm/hS2xGgBwYFK4EEACOhgYkDgYYABACstLK4
eKApdOYUl0vy2x10311lzQ4taOW77Y0LmRQ1p1tj7luh5Bvm2GFFbQNaM/dyk44K
TVmg0VEq0510blsHVAAeD2dXZPy3F9YEzIwCfTeO+5csMpCsrigi5ryXI2UXSnvX
933F4LoZwnuWYzsA8Ze1cWJz98LmrT6d2kDzHpNWMQ==
-----END EC PRIVATE KEY-----
)EOF";

const uint8_t mqttCertFingerprint[] = {0xAE,0x8F,0x15,0x77,0xA4,0x6C,0xD0,0x01,0x18,0x7E,0xE7,0x9A,0x16,0xED,0xF6,0x43,0xCD,0xDD,0x9B,0xC4};

X509List caCertX509(caCert);
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

String clientId = "ESP8266Client-";
//#ifdef TLS_DEBUG
/* verifytls()
 *  Test WiFiClientSecure connection using supplied cert and fingerprint
 */
bool verifytls() {
  Serial.println("test3");
bool success = false;
    
#ifdef SERIAL_DEBUG
  Serial.print("Verifying TLS connection to ");
  Serial.println("192.168.43.119");
#endif

  success = espClient.connect("192.168.43.119", 12345);

#ifdef SERIAL_DEBUG
  if (success) {
    Serial.println("Connection complete, valid cert, valid fingerprint.");
  }
  else {
    Serial.println("Connection failed!");
  }
#endif

  return (success);
}
//#endif


void reconnect() {
  /* Loop until we're reconnected */
  while (!mqttClient.connected()) {
#ifdef SERIAL_DEBUG
    Serial.print("Attempting MQTT broker connection...");
#endif
    /* Attempt to connect */
    if (mqttClient.connect(clientId.c_str())) {
#ifdef SERIAL_DEBUG
      Serial.println("connected");
#endif
      /* Once connected, resubscribe */
      mqttClient.subscribe("abc");      
    } 
    else {
#ifdef SERIAL_DEBUG
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Trying again in 5 seconds...");
#endif
      /* Wait 5 seconds between retries */
      delay(5000);
    }
  }
}


void setup() 
{
  /* Set board's GPIO pins as an outputs */
  //pinMode(RELAY_SIGNAL_PIN, OUTPUT);
  pinMode(ESP8266_LED, OUTPUT);

#ifdef SERIAL_DEBUG
  /* Initialize serial output for debug */
  Serial.setDebugOutput(true);
  Serial.begin(9600, SERIAL_8N1);
  Serial.println();
#endif

  /*  Connect to local WiFi access point */
  WiFi.mode(WIFI_STA);
  WiFi.begin("NikitaNote5", "nikita12345");
  
#ifdef SERIAL_DEBUG
  Serial.print("Connecting");
#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#ifdef SERIAL_DEBUG
    Serial.print(".");
#endif
  }
#ifdef SERIAL_DEBUG
  /* When WiFi connection is complete, debug log connection info */
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
#endif
espClient.setTrustAnchors(&caCertX509);
//Serial.println("test");
espClient.allowSelfSignedCerts(); 
//Serial.println("test1");
espClient.setFingerprint(mqttCertFingerprint);
//Serial.println("test4");
//#ifdef TLS_DEBUG
verifytls();
Serial.println("test2");
//#endif
mqttClient.setServer("192.168.43.119",12345);
//mqttClient.publish("abc", "esppppppppp");
 mqttClient.setCallback(subCallback);
  clientId += String(random(0xffff), HEX);
}

void loop()
{
  /* Main loop. Attempt to re-connect to MQTT broker if connection drops, and service the mqttClient task. */
  if(!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
   /* long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqttClient.publish("outTopic", msg);
}*/
}



void subCallback(char *topic, byte *payload,  unsigned int length)
{
 
  static int pinStatus = LOW;
  DynamicJsonDocument doc(256);
  deserializeJson(doc, (char*)payload);  
  JsonObject root = doc.as<JsonObject>();
  
#ifdef SERIAL_DEBUG  
  serializeJson(root, Serial);
  Serial.println();
#endif

  if(!root["set"].isNull()) {
    if(root["set"] == "toggle") {
      pinStatus = !pinStatus;
    } else if (root["set"] == "on") {
      pinStatus = HIGH;
    } else if (root["set"] == "off") {
      pinStatus = LOW;
    } else {
      return;
    }
    digitalWrite(ESP8266_LED, pinStatus);
    mqttClient.publish("demo", pinStatus == LOW ? "off" : "on");
  } else if(!root["get"].isNull()) {
    if(root["get"] == "status") {
      mqttClient.publish("demo", pinStatus == LOW ? "off" : "on");
    }
  }
}
