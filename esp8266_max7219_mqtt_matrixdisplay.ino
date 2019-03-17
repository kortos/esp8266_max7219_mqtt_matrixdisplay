#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Belegung
//DOT Matrix:       ESP8266 NodeMCU:
//VCC               5V (VUSB)
//GND               GND
//DIN               D7 (GPIO13)
//CS                D3 (GPIO0)
//CLK               D5 (GPIO14)

int pinCS = 0;                           //Für den PIN Select habe ich GPIO0 gewählt
int numberOfHorizontalDisplays = 12;     //Anzahl der Module Horizontal
int numberOfVerticalDisplays = 1;        //Anzahl der Module Vertikal

const char* ssid = "<WIFI-SSID>";
const char* password = "<WIFI-PWD>";
const char* mqtt_server = "<MQTT-Broker-IP>";

WiFiClient espClient;
PubSubClient client(espClient);

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

int ScrollWait = 70;                  // Zeit in ms für Scroll Geschwindigkeit
int helligkeit = 3;             // Helligkeit des DisplaysDefault Helligkeit 0 bis 15
int spacer = 1;                 // Länge eines Leerzeichens
int width = 5 + spacer;         // Schriftgröße
String MatrixText = "...";

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void MQTTCallback(char* topic, byte* payload, unsigned int length) {
  String PayloadString = "";
  for (int i = 0; i < length; i++) { PayloadString = PayloadString + (char)payload[i]; }

  Serial.println("New message arrived");  
  Serial.println(topic);  
  Serial.println(PayloadString);  
  
  if(strcmp(topic, "MatrixDisplay2/text") == 0) {  
    Serial.println("set new Text");      
    MatrixText = PayloadString;
  } 
 
  if(strcmp(topic, "MatrixDisplay2/intensity") == 0) { 
    Serial.println("set new Intensity");      
    helligkeit = PayloadString.toInt();
    matrix.setIntensity(helligkeit);
  }  
  
  if(strcmp(topic, "MatrixDisplay2/scrollwait") == 0) { 
    Serial.println("set new ScrollWait");      
    ScrollWait = PayloadString.toInt();
  }  
}

void reconnect() {
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("subscribe objects"); 
      client.subscribe("MatrixDisplay2/text");  
      client.subscribe("MatrixDisplay2/intensity");  
      client.subscribe("MatrixDisplay2/scrollwait");      
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
     
      delay(5000);
    }
  }
}

void setup() {
  //delay(1000);

  matrix.setIntensity(helligkeit);
  for (int matrixIndex=0 ; matrixIndex < numberOfHorizontalDisplays ; matrixIndex++ )
  {
    matrix.setRotation(matrixIndex, 1);        //Erste DOT Matrix Drehen 
  }

  Serial.begin(115200);
  Serial.println("Boot display...");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(MQTTCallback);  
}

void printMatrix() {
  for ( int i = 0 ; i < width * MatrixText.length() + matrix.width() - 1 - spacer; i++ ) {

    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; //Zentrieren des Textes Vertikal

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < MatrixText.length() ) {
        matrix.drawChar(x, y, MatrixText[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;

      client.loop(); 
    }

    matrix.write();
    delay(ScrollWait);
  }
}

void loop() {
  if (!client.connected())  {
    reconnect();
  }
  
  client.loop();    
  delay(500);
    
  printMatrix();      
}
