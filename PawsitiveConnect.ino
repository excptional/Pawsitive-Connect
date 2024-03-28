#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define your Firebase credentials and WiFi credentials
#define WIFI_SSID "WiFi Name"
#define WIFI_PASSWORD "WiFi Password"
#define FIREBASE_PROJECT_ID "Project Id"
#define FIREBASE_API_KEY "Web API Key"

FirebaseConfig fconfig;
FirebaseData fdata;
FirebaseAuth fauth;
FirebaseJson json;
bool isSendData = true;
bool shouldStop = false;
const long utcOffsetInSeconds = 0; 

WiFiServer server(80);
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  startWebserver();
  timeClient.begin();
  
//  configureFirebase();
}

void loop() {
  
  client = server.available();
  if (client) {
    Serial.println("New client connected");

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);

    // Check if the request is valid
    if (request.startsWith("GET /connect")) {
      // Send a response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("");
      client.println("Hello from Pawsitive Connect!");
    }else if (request.startsWith("GET /time")) {
      // Send a response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("");
      client.println(getCurrentTimeMillis());
    } else if (request.startsWith("POST /")) {
      // Look for empty line that separates headers from the body
      while (client.connected()) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          if (line == "\r") {
            break; // Empty line found, end of headers
          }
        }
      }

      // Read the POST body data
      String postData = client.readStringUntil('\r');
      Serial.println("POST Data: " + postData);

      // Process the data as needed
      // Here, we'll just send back a response with the received data
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println();
      client.print("Received data: ");
      client.print(postData);
    } else {
      // Send an error response
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: text/plain");
      client.println("");
      client.println("Invalid request");
    }

//    client.stop();
//    Serial.println("Client disconnected");
  }
  
// Wait before uploading again (optional)
//  delay(10000);
}

void startWebserver(){
  server.begin();
  Serial.println("HTTP server started");
}

void connectToWiFi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
}

void configureFirebase(){
  fconfig.api_key = FIREBASE_API_KEY;
  fauth.user.email = "bikash27dutta@gmail.com";
  fauth.user.password = "8777264639";
  fconfig.token_status_callback = tokenStatusCallback;

  Firebase.begin(&fconfig, &fauth);
  
  if (Firebase.ready()) {
    Serial.println("Connected to Firestore!");
  } else {
    Serial.println("Failed to connect to Firestore");
  }
  
  Firebase.reconnectWiFi(true);
}

void uploadDocument(String name, String phoneNumber) {

  json.set("fields/Name/stringValue", name);
  json.set("fields/Phone/stringValue", phoneNumber);

  if (Firebase.Firestore.patchDocument(&fdata, FIREBASE_PROJECT_ID, "", "users/data", json.raw(), "Name")
  && Firebase.Firestore.patchDocument(&fdata, FIREBASE_PROJECT_ID, "", "users/data", json.raw(), "Phone")) {
    Serial.println("Document uploaded successfully!");  
    isSendData = false;
  } else {
    Serial.println("Error uploading document: " + fdata.errorReason());
  }
}

String extractData(String postData) {
  // Find the index of '=' character
  int index = postData.indexOf('=');
  if (index != -1) {
    // Extract the data after '=' character
    String extractedData = postData.substring(index + 1);

    // Decode URL-encoded characters
    extractedData = URLdecode(extractedData);

    return extractedData;
  }
  return ""; // Return empty string if no data is found
}

String URLdecode(String str) {
  String decodedString = "";
  char tempBuffer[2];
  int len = str.length();
  int i = 0;
  while (i < len) {
    if (str.charAt(i) == '%') {
      tempBuffer[0] = str.charAt(i + 1);
      tempBuffer[1] = str.charAt(i + 2);
      decodedString += (char)strtol(tempBuffer, NULL, 16);
      i += 3;
    } else if (str.charAt(i) == '+') {
      decodedString += ' ';
      i++;
    } else {
      decodedString += str.charAt(i);
      i++;
    }
  }
  return decodedString;
}

unsigned long getCurrentTimeMillis() {
  timeClient.update();
  return timeClient.getEpochTime();
}


void stopServer() {
  shouldStop = true;
}
