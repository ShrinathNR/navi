
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include "DHT.h"
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <OneWire.h>
#include <DallasTemperature.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "VETRI ASHWATH"
#define WIFI_PASSWORD "9283239821"

//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyDcXYk-psOIRVRulXiRUDNRqcBr4EldERs"

/* 3. Define the RTDB URL */
#define DATABASE_URL "healthmoniteringsys-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "ashwathv20@gmail.com"
#define USER_PASSWORD "Welc0me$"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

#define DHTPIN 5
#define DS18B20PIN 14   
// Digital pin connected to the DHT sensor
#define REPORTING_PERIOD_MS     1000
float BPM, SpO2,btempinc,btempinf,hum,rtempinc,rtempinf,hic,hif;
#define DHTTYPE DHT11   // DHT 1
PulseOximeter pox;
uint32_t tsLastReport = 0;
OneWire oneWire(DS18B20PIN);
DallasTemperature sensor(&oneWire);

void onBeatDetected()
{
  Serial.println("Beat Detected!");
}
DHT dht(DHTPIN, DHTTYPE);

void setup()
{

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  //Or use legacy authenticate method
  //config.database_url = DATABASE_URL;
  //config.signer.tokens.legacy_token = "<database secret>";

  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
  pinMode(19, OUTPUT);
  
  Serial.print("Initializing pulse oximeter..");

  if (!pox.begin()) {
    Serial.println("FAILED");
    //for (;;);
  } else {
    Serial.println("SUCCESS");

    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  Serial.println(F("DHTxx test!"));
  dht.begin();
  sensor.begin();
}

void loop()
{
  //Flash string (PROGMEM and  (FPSTR), String, C/C++ string, const char, char array, string literal are supported
  //in all Firebase and FirebaseJson functions, unless F() macro is not supported.
  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  rtempinc = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  rtempinf = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(rtempinc) || isnan(rtempinf)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  hif = dht.computeHeatIndex(rtempinf, hum);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(rtempinc, hum, false);
  pox.update();
  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();
  sensor.requestTemperatures(); 
  btempinc = sensor.getTempCByIndex(0);
  btempinf = sensor.getTempFByIndex(0);
 

  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {

    Serial.print("BPM: ");
    Serial.println(BPM);

    Serial.print("SpO2: ");
    Serial.print(SpO2);
    Serial.println("%");

    Serial.println("*********************************");
    Serial.println();
    Serial.print("Temperature = ");
    Serial.print(btempinc);
    Serial.println("ºC |");
    Serial.print(btempinf);
    Serial.println("ºF");
    delay(3000);
    Serial.print(F(" Humidity: "));
    Serial.print(hum);
    Serial.print(F("%  Temperature: "));
    Serial.print(rtempinc);
    Serial.print(F("C "));
    Serial.print(rtempinf);
    Serial.print(F("F  Heat index: "));
    Serial.print(hic);
    Serial.print(F("C "));
    Serial.print(hif);
    Serial.println(F("F"));

    tsLastReport = millis();
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    /*Serial.printf("Set bool... %s\n", Firebase.setBool(fbdo, "/test/bool", count % 2 == 0) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get bool... %s\n", Firebase.getBool(fbdo, "/test/bool") ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());

    bool bVal;
    Serial.printf("Get bool ref... %s\n", Firebase.getBool(fbdo, "/test/bool", &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());

    Serial.printf("Set int... %s\n", Firebase.setInt(fbdo, "/test/int", count) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get int... %s\n", Firebase.getInt(fbdo, "/test/int") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
*/
   //Serial.printf("Get int ref... %s\n", Firebase.getInt(fbdo, "/test/int", &iVal) ? String(iVal).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/bodytemperatureinc",btempinc) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/bodytemperatureinf",btempinf) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/roomtemperatureinc",rtempinc) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/roomtemperatureinf",rtempinf) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/humidity",hum) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/heatindexinc",hic) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, "/ms/heatindexinf",hif) ? "ok" : fbdo.errorReason().c_str());

    /*Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&btempinc) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinf",&btempinf) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/roomtemperatureinc",&rtempinc) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&rtempinf) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&hum) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&hif) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&hic) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&BPM) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    Serial.printf("Get float... %s\n", Firebase.getFloat(fbdo, "/ms/bodytemperatureinc",&SpO2) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
   //Serial.printf("Set double... %s\n", Firebase.setDouble(fbdo, "/test/double", count + 35.517549723765) ? "ok" : fbdo.errorReason().c_str());

    //Serial.printf("Get double... %s\n", Firebase.getDouble(fbdo, "/test/double") ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    //Serial.printf("Set string... %s\n", Firebase.setString(fbdo, "/test/string", "Hello World!") ? "ok" : fbdo.errorReason().c_str());

    //Serial.printf("Get string... %s\n", Firebase.getString(fbdo, "/test/string") ? fbdo.to<const char *>() : fbdo.errorReason().c_str());

    //For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
    */
    FirebaseJson json;

    if (count == 0)
    {
      json.set("value/round/" + String(count), "cool!");
      json.set("vaue/ts/.sv", "timestamp");
      Serial.printf("Set json... %s\n", Firebase.set(fbdo, "/test/json", json) ? "ok" : fbdo.errorReason().c_str());
    }
    else
    {
      json.add(String(count), "smart!");
      Serial.printf("Update node... %s\n", Firebase.updateNode(fbdo, "/test/json/value/round", json) ? "ok" : fbdo.errorReason().c_str());
    }

    Serial.println();

    //For generic set/get functions.

    //For generic set, use Firebase.set(fbdo, <path>, <any variable or value>)

    //For generic get, use Firebase.get(fbdo, <path>).
    //And check its type with fbdo.dataType() or fbdo.dataTypeEnum() and
    //cast the value from it e.g. fbdo.to<int>(), fbdo.to<std::string>().

    //The function, fbdo.dataType() returns types String e.g. string, boolean,
    //int, float, double, json, array, blob, file and null.

    //The function, fbdo.dataTypeEnum() returns type enum (number) e.g. fb_esp_rtdb_data_type_null (1),
    //fb_esp_rtdb_data_type_integer, fb_esp_rtdb_data_type_float, fb_esp_rtdb_data_type_double,
    //fb_esp_rtdb_data_type_boolean, fb_esp_rtdb_data_type_string, fb_esp_rtdb_data_type_json,
    //fb_esp_rtdb_data_type_array, fb_esp_rtdb_data_type_blob, and fb_esp_rtdb_data_type_file (10)

    count++;
  }
}

/// PLEASE AVOID THIS ////

//Please avoid the following inappropriate and inefficient use cases
/**
 * 
 * 1. Call get repeatedly inside the loop without the appropriate timing for execution provided e.g. millis() or conditional checking,
 * where delay should be avoided.
 * 
 * Everytime get was called, the request header need to be sent to server which its size depends on the authentication method used, 
 * and costs your data usage.
 * 
 * Please use stream function instead for this use case.
 * 
 * 2. Using the single FirebaseData object to call different type functions as above example without the appropriate 
 * timing for execution provided in the loop i.e., repeatedly switching call between get and set functions.
 * 
 * In addition to costs the data usage, the delay will be involved as the session needs to be closed and opened too often
 * due to the HTTP method (GET, PUT, POST, PATCH and DELETE) was changed in the incoming request. 
 * 
 * 
 * Please reduce the use of swithing calls by store the multiple values to the JSON object and store it once on the database.
 * 
 * Or calling continuously "set" or "setAsync" functions without "get" called in between, and calling get continuously without set 
 * called in between.
 * 
 * If you needed to call arbitrary "get" and "set" based on condition or event, use another FirebaseData object to avoid the session 
 * closing and reopening.
 * 
 * 3. Use of delay or hidden delay or blocking operation to wait for hardware ready in the third party sensor libraries, together with stream functions e.g. Firebase.RTDB.readStream and fbdo.streamAvailable in the loop.
 * 
 * Please use non-blocking mode of sensor libraries (if available) or use millis instead of delay in your code.
 * 
 * 4. Blocking the token generation process.
 * 
 * Let the authentication token generation to run without blocking, the following code MUST BE AVOIDED.
 * 
 * while (!Firebase.ready()) <---- Don't do this in while loop
 * {
 *     delay(1000);
 * }
 * 
 */
