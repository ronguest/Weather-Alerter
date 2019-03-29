//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#include <WiFi101.h>
#include "WeatherClient.h"

WeatherClient::WeatherClient(boolean foo) {
}

void WeatherClient::updateConditions(String apiKey, String location) {
  doUpdate(443, "api.darksky.net", "/forecast/" + apiKey + "/" + location + "?exclude=minutely,hourly");
}

void WeatherClient::doUpdate(int port, char server[], String url) {
  JsonStreamingParser parser;
  // It might be better to have separate objects for each weather data source
  // As it is now the same code is processing keywords from diverse sources leading to potential conflictss
  // Currently this is handled by a bit of a kludge: checking the parent keyword to see if it also matches the desired feed
  parser.setListener(this);
  WiFiClient client;
  // Red LED output on the M0 Feather
  const int ledPin = 13;

  Serial.print("Connect to Server: "); Serial.println(server);
  Serial.println("URL: " + url);
  digitalWrite(ledPin, HIGH);   // Turn on ledPin, it will stay on if we get an error

  if (port == 443) {
    if (!client.connectSSL(server, port)) {
      Serial.println("SSL connection failed");
      return;
    }
  } else {
    if (!client.connect(server, port)) {
      Serial.println("connection failed");
      return;
    }
  }

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" +
               "Connection: close\r\n\r\n");

  // Wait some seconds to start receiving a response
  int retryCounter = 0;
  while(!client.available()) {
    delay(1000);
    retryCounter++;
    if (retryCounter > 15) {
      Serial.println(F("Retry timed out"));
      return;
    }
  }

  boolean isBody = false;
  char c;
  int size = 0;
  //client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  }
  client.stop();          // We're done, shut down the connection

  digitalWrite(ledPin, LOW);    // Turn LED off to show we're done
}

// The key basically tells us which set of data from the JSON is coming
void WeatherClient::key(String key) {
  currentKey = String(key);
}

// This is the heart of processing the JSON file
// The JSON parser calls the appropriate function as it encounters them in the JSON stream
// Some are arrays of values so we index those as we get them
// There is a bounds check at the end of the function.
// If a value = "null" for several of the below we ignore those due to how the Wunderground feed works
// Should only be null afer 3pm which is an arbitrary cut off by WU ?
void WeatherClient::value(String value) {
  if (currentKey == "tempf") {        // Only thing we take from Ambient Weather
    Serial.println("Get tempf " + value);
    currentTemp = value;
  }

  if (currentKey == "temperatureMax") {
    forecastHighTemp[currentForecastPeriod++] = value;
  }
  if (currentKey == "temperatureMin") {
    forecastLowTemp[currentForecastPeriod++] = value;
  }
  if (currentKey == "iconCode") {
    // I leave values as strings because we will display them
    // iconCode however is used as constant in a switch statement so worth converting here
    forecastIcon[currentForecastPeriod++] = value.toInt();
  }
  // There are two "narrative" arrays from WU, one is day of week the other is day + night, we want day + night
  if ((currentKey == "narrative") && (currentParent == "daypart")) {
    forecastText[currentForecastPeriod++] = value;
  }
  if (currentKey == "sunriseTimeLocal") {
    sunriseTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "sunsetTimeLocal") {
    sunsetTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "moonriseTimeLocal") {
      moonriseTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "moonsetTimeLocal") {
      moonsetTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "moonPhaseDay") {
    moonAge[currentForecastPeriod++] = value;
  }
  if (currentKey == "dayOfWeek") {
    forecastDayOfWeek[currentForecastPeriod++] = value;
  }

  // Prevent currentForecastPeriod going out of bounds (shouldn't happen but...)
  if (currentForecastPeriod >= MAX_FORECAST_PERIODS) {
    currentForecastPeriod = MAX_FORECAST_PERIODS - 1;
  }
}

String WeatherClient::getCurrentTemp() {
  return currentTemp;
}

// Icon getters
String WeatherClient::getTodayIcon() {
	if (forecastIcon[0] == 0) {
		return "null";
	} else {
		return getMeteoconIcon(forecastIcon[0]);
	}
}
String WeatherClient::getTonightIcon() {
  return getMeteoconIcon(forecastIcon[1]);
}
String WeatherClient::getTomorrowIcon() {
  return getMeteoconIcon(forecastIcon[2]);
}
String WeatherClient::getTomorrowNightIcon() {
  return getMeteoconIcon(forecastIcon[3]);
}

// High/Low getters
String WeatherClient::getTodayForecastHigh() {
  return forecastHighTemp[0];
}
String WeatherClient::getTomorrowForecastHigh() {
  return forecastHighTemp[1];
}
String WeatherClient::getTodayForecastLow() {
  return forecastLowTemp[0];
}
String WeatherClient::getTomorrowForecastLow() {
  return forecastLowTemp[1];
}

// Forecast text getters
String WeatherClient::getTodayForecastTextAM() {
  return forecastText[0];
}
String WeatherClient::getTodayForecastTextPM() {
  return forecastText[1];
}
String WeatherClient::getTomorrowForecastTextAM() {
  return forecastText[2];
}
String WeatherClient::getTomorrowForecastTextPM() {
  return forecastText[3];
}

// Day of week names (Mon, Tue, etc)
String WeatherClient::getTodayName() {
  return forecastDayOfWeek[0];
}
String WeatherClient::getTomorrowName() {
  return forecastDayOfWeek[1];
}

// Sun and Moon times
String WeatherClient::getMoonAge() {
  return moonAge[0];
}
String WeatherClient::getSunriseTime() {
  return sunriseTime[0];
}
String WeatherClient::getSunsetTime() {
  return sunsetTime[0];
}
String WeatherClient::getMoonriseTime() {
  return moonriseTime[0];
}
String WeatherClient::getMoonsetTime() {
  return moonsetTime[0];
}

void WeatherClient::whitespace(char c) {
  //Serial.println(F("whitespace"));
}

void WeatherClient::startDocument() {
  //Serial.println(F("start document"));
}
void WeatherClient::endDocument() {

}

// startArray lets us know the key has a set of values
// When we see this we set the index to Zero
// It should be incremented in the value function when a value is added
void WeatherClient::startArray() {
  //Serial.println("startArray");
  currentForecastPeriod = 0;
}
// We don't need to do anything special when we get an endArray
void WeatherClient::endArray() {
  //Serial.println("endArray");
}


void WeatherClient::startObject() {
  // I have not seen this to be reliable (?). Unused for now.
  currentParent = currentKey;
  //Serial.println("startObject: " + currentParent);
}
void WeatherClient::endObject() {
  //Serial.println("endObject: " + currentParent);
  currentParent = "";
}

// Converts the WU icon code to the file name for the M0
String WeatherClient::getMeteoconIcon(int iconCode) {
  switch (iconCode) {
  case 0:
    return "hazy";      // this is really a tornado
  case 3:
    return "tstorms";
  case 4:
    return "tstorms";
  case 5:
    return "snow";
  case 6:
    return "sleet";
  case 7:
    return "sleet";
  case 8:
    return "sleet";
  case 9:
    return "rain";
  case 10:
    return "sleet";
  case 11:
    return "rain";
  case 12:
    return "rain";
  case 13:
    return "flurries";
  case 14:
    return "snow";
  case 15:
    return "snow";
  case 16:
    return "snow";
  case 17:
    return "tstorms";
  case 18:
    return "sleet";
  case 19:
    return "fog";
  case 20:
    return "fog";
  case 21:
    return "hazy";
  case 22:
    return "hazy";
  case 23:
    return "hazy";
  case 24:
    return "hazy";
  case 25:
    return "sleet";
  case 26:
    return "cloudy";
  case 27:
    return "mcloudy";
  case 28:
    return "mcloudy";
  case 29:
    return "pcloudy";
  case 30:
    return "pcloudy";
  case 31:
    return "clear";
  case 32:
    return "sunny";
  case 33:
    return "psunny";
  case 34:
    return "psunny";
  case 35:
    return "tstorms";
  case 36:
    return "sunny";
  case 37:
    return "tstorms";
  case 38:
    return "tstorms";
  case 39:
    return "rain";
  case 40:
    return "rain";
  case 41:
    return "flurries";
  case 42:
    return "snow";
  case 43:
    return "snow";
  case 45:
    return "rain";
  case 46:
    return "snow";
  case 47:
    return "tstorms";
  default:
    Serial.println("Got icon code: " + iconCode);
    return "unknown";
  }
}
