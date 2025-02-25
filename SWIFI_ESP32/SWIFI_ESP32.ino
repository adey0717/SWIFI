#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Thermal.h>

#define SIGNAL_FINISH 5 // Define an LED Pin. Change GPIO pin number as needed

const char* ssid = "TP-Link_E916"; //--> Your wifi name or SSID.
const char* password = "Eyabong11302019"; //--> Your wifi password.

#define rxPin 2
#define txPin 4
HardwareSerial ESP32_Serial(1);

#define RX_PIN 16 // RX pin connected to printer (GPIO3)
#define TX_PIN 17 // TX pin connected to printer (GPIO1)

HardwareSerial printerSerial(Serial2); // Use hardware serial port 1
Adafruit_Thermal printer(&printerSerial); // Create an Adafruit_Thermal object

char c;
String dataIn;


float Converted_Weight;
String myInts[9];
int incomingByte = 0;
String Data = "";
int until = 8;
int arrayIndex = 0;

String LGU, CITY, ZIP;

const int stringLength = 8;
const int EEPROM_address = 0;
const int numStrings = 8; // Number of strings to store in EEPROM
String randomString;

// Web Server address / IPv4
const char *urlSaveCode = "http://192.168.1.100/rvm-p2p/saveCode.php?";
const char *urlGetAddress = "http://192.168.1.100/rvm-p2p/getAddress.php?";

boolean connectedFlag = false;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(500);
  pinMode(SIGNAL_FINISH, OUTPUT);
  digitalWrite(SIGNAL_FINISH, HIGH);
  ESP32_Serial.begin(9600, SERIAL_8N1, rxPin, txPin);
  printerSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Initialize hardware serial for printer
  printer.begin(); // Initialize the thermal printer
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }
  randomSeed(analogRead(0));
  connectedFlag = true;
}

void loop() {

  if (ESP32_Serial.available() > 0) { // Check if data is available to read
    String receivedString = ESP32_Serial.readString(); // Read the incoming data as a string
    Serial.print("Received:");
    Serial.println(receivedString); // Print the received string for debugging

    Converted_Weight = receivedString.toFloat();
    if (Converted_Weight != 0.0)
    {
      START_PRINTER();
    }
  }
}
void Save_Code(String code) {
  if (connectedFlag) {
    HTTPClient http;
    String data = "code=" + code;

    http.begin(urlSaveCode);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(data);
    String payload = http.getString();

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.println("Error parsing JSON");
    } else {
      // Extract daily limit and weekly limit
      const char* status = doc["status"];
      Serial.print("Status: ");
      Serial.println(status);
    }
    http.end();
    Serial.println("Response: " + payload);
  }
}

void Get_Address(String id) {
  if (connectedFlag) {
    HTTPClient http;
    String data = "id=" + id;

    http.begin(urlGetAddress);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(data);
    String payload = http.getString();

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.println("Error parsing JSON");
    } else {
      // Extract data from JSON
      const char* lgu = doc["lgu"];
      const char* city = doc["city"];
      const char* zip = doc["zip"];
      LGU = String(lgu);
      CITY = String(city);
      ZIP = String(zip);
      Serial.print("LGU: ");
      Serial.println(LGU);
      Serial.print("CITY: ");
      Serial.println(CITY);
      Serial.print("ZIP: ");
      Serial.println(ZIP);
    }
    http.end();
    Serial.println("Response: " + payload);
  }
}

String generateRandomString(int length) {
  String charSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String randomString = "";

  for (int i = 0; i < length; i++) {
    int randomIndex = random(0, charSet.length());
    randomString += charSet[randomIndex];
  }

  return randomString;
}

bool isDuplicate(String str) {
  for (int i = 0; i < numStrings; i++) {
    String storedString;
    EEPROM.begin(stringLength);
    EEPROM.get(i * stringLength, storedString);
    EEPROM.end();
    if (str.equals(storedString)) {
      return true; // String found in EEPROM
    }
  }
  return false; // String not found in EEPROM
}

void storeString(String str) {
  int index = -1;
  for (int i = 0; i < numStrings; i++) {
    String storedString;
    EEPROM.begin(stringLength);
    EEPROM.get(i * stringLength, storedString);
    if (storedString.length() == 0) {
      index = i;
      break;
    }
    EEPROM.end();
  }
  if (index != -1) {
    EEPROM.begin(stringLength);
    EEPROM.put(index * stringLength, str);
    EEPROM.commit();
    EEPROM.end();
  }
}

void START_PRINTER()
{
  Get_Address(String(1));
  randomString = generateRandomString(stringLength);

  if (!isDuplicate(randomString)) {
    Serial.println(randomString);
    Save_Code(randomString);
    printer.justify('C');
    printer.setSize('L');
    printer.println(F("    RVM P2P    "));
    printer.setSize('S');
    printer.println(LGU + ",");
    printer.println(CITY + " ," + ZIP);
    printer.println(F("                     "));
    printer.setSize('S');
    printer.justify('L');
    printer.print("Weight: " + String(Converted_Weight));
    printer.println( " g");
    printer.print("Price:_____________");
    printer.println( " .00");
    printer.print("Total:_____________");
    printer.println(F("                     "));
    printer.justify('C');
    printer.setSize('M');
    printer.boldOn();
    printer.println(F("CODE TO VERIFY"));
    printer.boldOff();
    printer.setSize('S');
    printer.justify('C');
    printer.println(randomString);
    printer.println(F("                     "));
    printer.println(F("                     "));
    printer.println(F("                     "));
    printer.println(F("                     "));
    digitalWrite(SIGNAL_FINISH, LOW);
    delay(2000);
    digitalWrite(SIGNAL_FINISH, HIGH);
    delay(2000);
    storeString(randomString);
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  //This line hides the viewing of ESP as wifi hotspot
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}
