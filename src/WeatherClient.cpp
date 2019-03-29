//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#include "WeatherClient.h"
#include <WiFi101.h>

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

    Serial.print("Connect to Server: ");
    Serial.println(server);
    Serial.println("URL: " + url);
    digitalWrite(ledPin, HIGH);  // Turn on ledPin, it will stay on if we get an error

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
    while (!client.available()) {
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
    while (client.connected()) {
        while ((size = client.available()) > 0) {
            c = client.read();
            if (c == '{' || c == '[') {
                isBody = true;
            }
            if (isBody) {
                parser.parse(c);
            }
        }
    }
    client.stop();  // We're done, shut down the connection

    digitalWrite(ledPin, LOW);  // Turn LED off to show we're done
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
	if (currentParent == "currently") {
		if (currentKey == "nearestStormDistance") { 
			Serial.println("nearestStormDistance " + value);
			nearestStormDistance = value.toInt();
		}
		if (currentKey == "summary") {  
			Serial.println("summary " + value);
			summary = value;
		}
		if (currentKey == "currentIcon") { 
			Serial.println("currentIcon " + value);
			currentIcon = value;
		}
		if (currentKey == "precipProbability") { 
			Serial.println("precipProbability " + value);
			precipProbability = value.toInt();
		}
		if (currentKey == "temperature") { 
			Serial.println("temperature " + value);
			temperature = value.toFloat();
		}
		if (currentKey == "windSpeed") { 
			Serial.println("windSpeed " + value);
			windSpeed = value.toInt();
		}
		if (currentKey == "windGust") { 
			Serial.println("windGust " + value);
			windGust = value.toInt();
		}
	} else if (currentParent == "alerts") {
		if (currentKey == "description") { 
			Serial.println("description " + value);
			description[alertIndex] = value;
		}
		if (currentKey == "severity") { 
			Serial.println("severity " + value);
			severity[alertIndex] = value;
		}		
	} else if (currentParent == "data") {
		if (currentKey == "temperatureMax") {
			Serial.println("temperatureMax " + value);
			temperatureMax[dailyIndex++] = value.toFloat();
		}
	} else {
		//Serial.println("unused currentParent " + currentParent);
	}
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
    Serial.println("startArray");
	Serial.println("currentParent " + currentParent);
	Serial.println("currentKey " + currentKey);
	if (currentParent == "alerts") {
		alertIndex = 0;
	} else if (currentParent == "daily") {
		dailyIndex = 0;
	}
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
