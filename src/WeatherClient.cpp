//
//  Copyright (C) 2027-2029 Ronald Guest <http://ronguest.net>
//

#include "WeatherClient.h"
#include <WiFi101.h>

String parents[parentSize];
uint16_t parentIndex = 0;
boolean inArray = false;
uint16_t dailyIndex;
uint16_t alertIndex;

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
    const int ledPin = 23;

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
	//Serial.println("Push key " + key);
	push(key);
}

// Time to use the parent() to figure out which section of JSON we are in
// and current() to know which value we got
void WeatherClient::value(String value) {
	if (parent() == "currently") {
		if (current() == "nearestStormDistance") { 
			Serial.println("nearestStormDistance " + value);
			nearestStormDistance = value.toInt();
		}
		if (current() == "summary") {  
			Serial.println("summary " + value);
			summary = value;
		}
		if (current() == "currentIcon") { 
			Serial.println("currentIcon " + value);
			currentIcon = value;
		}
		if (current() == "precipProbability") { 
			Serial.println("precipProbability " + value);
			precipProbability = value.toInt();
		}
		if (current() == "temperature") { 
			Serial.println("temperature " + value);
			temperature = value.toFloat();
		}
		if (current() == "windSpeed") { 
			Serial.println("windSpeed " + value);
			windSpeed = value.toInt();
		}
		if (current() == "windGust") { 
			Serial.println("windGust " + value);
			windGust = value.toInt();
		}
	} else if (parent() == "alerts") {
		if (current() == "description") { 
			Serial.println("description " + value);
			description[alertIndex] = value;
		}
		if (current() == "severity") { 
			Serial.println("severity " + value);
			severity[alertIndex] = value;
		}		
	} else if (parent() == "data") {
		if (current() == "temperatureMax") {
			Serial.print("save tempMax to index " + String(dailyIndex));
			Serial.println(", temperatureMax " + value);
			temperatureMax[dailyIndex] = value.toFloat();
		}
	} else {
		//Serial.println("unused parent " + parent());
	}
	pop();
}

void WeatherClient::whitespace(char c) {
    //Serial.println(F("whitespace"));
}

void WeatherClient::startDocument() {
    Serial.println(F("start document"));
	alertIndex = 0;
	dailyIndex = 0;
}
void WeatherClient::endDocument() {
    Serial.println(F("end document"));
	for (int i=0; i<dailyIndex; i++) {
		Serial.println(temperatureMax[i]);
	}
}

// startArray lets us know the key has a set of values
void WeatherClient::startArray() {
	inArray = true;
    Serial.print("startArray ");
	Serial.print("parent " + parent());
	Serial.println(", current " + current());
}
void WeatherClient::endArray() {
	inArray = false;
    Serial.print("endArray ");
	Serial.print("parent " + parent());
	Serial.println(", current " + current());
}

void WeatherClient::startObject() {
	Serial.print("In startObject ");
	Serial.print("parent " + parent());
	Serial.println(", current " + current());
}
void WeatherClient::endObject() {
    Serial.print("endObject before pop: ");
	Serial.print("parent " + parent());	Serial.println(", current " + current());
	if (parent() == "alerts") {
		Serial.println("Increase alertIndex");
		alertIndex++;
	} else if (parent() == "daily") {
		Serial.println("Increase dailyIndex");
		dailyIndex++;
	}	

	// Objects in an array don't have a key/name so we can't pop in that case or we lost parentage
	if (!inArray) {
		pop();
	}
}

void WeatherClient::push(String s) {
	if (parentIndex < parentSize - 2) {
		parents[parentIndex++] = s;
	} else {
		Serial.println("parentIndex overrun");
	}
}

String WeatherClient::pop() {
	if (parentIndex > 0) {
		parentIndex--;
		return parents[parentIndex];
	} else {
		return "";
	}
}

String WeatherClient::parent() {
	if (parentIndex > 1) {
		return parents[parentIndex-2];
	 } else {
		 return "";
	 }
}

String WeatherClient::current() {
	if (parentIndex > 0) {
		return parents[parentIndex-1];
	 } else {
		return "";
	 }
}