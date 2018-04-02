#include "ConfigStatic.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <ESPmDNS.h>
#endif
#include <PubSubClient.h>

#define PIN_SWITCH D0
#define PIN_USB1 D1
#define PIN_USB2 D2
#define PIN_USBP D7
#define PIN_STATE1 D5
#define PIN_STATE2 D6

#define STATE_UNKNOWN 0
#define STATE_USB1 1
#define STATE_USB2 2

const char* mqttBroker = CFG_MQTT_SERVER;
const char* mqttUser = CFG_MQTT_NAME;
const char* mqttPassword = CFG_MQTT_PASS;

int powerUSB1 = -1;
int powerUSB2 = -1;
int powerUSBP = -1;
int lastState = -1;
WiFiClient wifi;
PubSubClient mqtt(wifi);

void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectIfNeeded();
void startupSwitch();

void setup() {
  Serial.begin(9600);
  Serial.println("setup");
  
  pinMode(PIN_SWITCH, OUTPUT); 
  
  pinMode(PIN_USB1, INPUT); 
  pinMode(PIN_USB2, INPUT); 
  pinMode(PIN_USBP, INPUT_PULLUP);
   
  pinMode(PIN_STATE1, INPUT); 
  pinMode(PIN_STATE2, INPUT); 
  digitalWrite(PIN_SWITCH, HIGH);
  delay(500);
  
  WiFi.mode(WIFI_STA);
  WiFi.hostname(CFG_HOSTNAME);
  mqtt.setServer(mqttBroker, 1883);
  mqtt.setCallback(mqttCallback);
}

void connectIfNeeded() {
  #if defined(CFG_MQTT_ACTIVE)
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("connect wifi");
      WiFi.begin(CFG_WIFI_NAME, CFG_WIFI_PASS);
      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        Serial.println(F("WiFi connected"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP()); 
      } else {
        Serial.println(F("failed"));
        delay(1000);
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqtt.connected()) {
        Serial.println("connect mqtt");
        
        if (mqtt.connect(CFG_MQTT_TOPIC, mqttUser, mqttPassword, CFG_MQTT_TOPIC, 0, true, "offline")) {
          mqtt.publish(CFG_MQTT_TOPIC, "online", true);
          Serial.println("subscribe mqtt");
          mqtt.subscribe(CFG_MQTT_TOPIC_COMMAND);
          lastState = -1;
          powerUSB1 = -1;
          powerUSB2 = -1;
          powerUSBP = -1;
        } else {
          Serial.print(F("failed, rc="));
          Serial.println(mqtt.state());
        }
      }
    }
  #endif
}

void loop() {
  int usb1 = digitalRead(PIN_USB1);
  int usb2 = digitalRead(PIN_USB2);
  int usbp = digitalRead(PIN_USBP);

  char buffer[2];
  if (powerUSB1 != usb1) {
    Serial.print("usb1: ");
    Serial.println(usb1);
    itoa(usb1, buffer, 2);
    mqtt.publish(CFG_MQTT_TOPIC_STATE_USB1, buffer);
  }
  if (powerUSB2 != usb2) {
    Serial.print("usb2: ");
    Serial.println(usb2);
    itoa(usb2, buffer, 2);
    mqtt.publish(CFG_MQTT_TOPIC_STATE_USB2, buffer);
  }
  if (powerUSBP != usbp) {
    Serial.print("usbp: ");
    Serial.println(usbp);
    itoa(usbp, buffer, 2);
    mqtt.publish(CFG_MQTT_TOPIC_STATE_USBP, buffer);
  }

  if (usb1 == HIGH && usb2 == LOW) {
    powerUSB1 = usb1;
    powerUSB2 = usb2;
    doSwitch(STATE_USB1);
  } else if (usb1 == LOW && usb2 == HIGH) {
    powerUSB1 = usb1;
    powerUSB2 = usb2;
    doSwitch(STATE_USB2);
  } else if (usb1 == HIGH && usb2 == HIGH) {
    #if defined(CFG_SWITCH_ON_SECOND_DEVICE)
      if (powerUSB1 == LOW) {
        doSwitch(STATE_USB1);
      } else if (powerUSB2 == LOW) {
        doSwitch(STATE_USB2);
      }
    #endif
    powerUSB1 = usb1;
    powerUSB2 = usb2;
  } else if (usb1 == LOW && usb2 == LOW) {
    powerUSB1 = usb1;
    powerUSB2 = usb2;
  }
  if (usbp != powerUSBP) {
    powerUSBP = usbp; 
  }
  getState();
  mqtt.loop();
  connectIfNeeded();
  delay(100);
}

void publishState(int state) {
  if (lastState != state) {
    if (state == STATE_USB1) {
      Serial.println("publish STATE_USB1");
      mqtt.publish(CFG_MQTT_TOPIC_STATE_SWITCH, "usb1");
    } else if (state == STATE_USB2) {
      Serial.println("publish STATE_USB2");
      mqtt.publish(CFG_MQTT_TOPIC_STATE_SWITCH, "usb2");
    } else {
      Serial.println("publish STATE_UNKNOWN");
      mqtt.publish(CFG_MQTT_TOPIC_STATE_SWITCH, "unknown");    
    }
    lastState = state;
  }
}

void doSwitch(int targetState) {
  int currentState = getState();
  while (currentState == STATE_UNKNOWN) {
    Serial.print("waiting STATE_UNKNOWN: ");
    delay(100);
    currentState = getState();
  }
  if (currentState != targetState) {
    Serial.print("currentState: ");
    Serial.println(currentState);
    Serial.print("targetState: ");
    Serial.println(targetState);
    switchState();
  }
}
void switchState() {
  digitalWrite(PIN_SWITCH, LOW);
  delay(50);
  digitalWrite(PIN_SWITCH, HIGH);
  delay(50);
}

int getState() {
  int state1 = digitalRead(PIN_STATE1);
  int state2 = digitalRead(PIN_STATE2);
  int state = STATE_UNKNOWN;

  if (state1 == HIGH && state2 == LOW) {
    state = STATE_USB1;
  } else if (state2 == HIGH && state1 == LOW) {
    state = STATE_USB2;
  }
  publishState(state);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (length == 1) {
    if ((char)payload[0] == '1') {
      doSwitch(STATE_USB1);
    }  else if ((char)payload[0] == '2')  {
      doSwitch(STATE_USB2);
    }
  } else if (length == 4) {
    if ((char)payload[0] == 'u' && (char)payload[1] == 's' && (char)payload[2] == 'b') {
      if ((char)payload[3] == '1') {
        doSwitch(STATE_USB1);
      }  else if ((char)payload[3] == '2')  {
        doSwitch(STATE_USB2);
      }
    }
  }
}

