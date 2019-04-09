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
	// Reset the alertIndex to 0 here since this is the only JSON document containing alerts
	// Also dailyIndex because that only applies to the Dark Sky reponse as well (not AW)
	alertIndex = 0;
	dailyIndex = 0;
    doUpdate(443, "api.darksky.net", "/forecast/" + apiKey + "/" + location + "?exclude=minutely,hourly");
}

void WeatherClient::updateLocal(String device, String appKey, String apiKey) {
  doUpdate(443, "api.ambientweather.net", "/v1/devices/" + device + "?applicationKey=" + appKey + "&apiKey=" + apiKey + "&limit=1");
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

String WeatherClient::getSummary() {
	return summary;
}

String WeatherClient::getCurrentIcon() {
	return currentIcon;
}

double WeatherClient::getTemperature() {
	return temperature;
}

uint16_t WeatherClient::getWindSpeed() {
	return windSpeed;
}

uint16_t WeatherClient::getWindGust() {
	return windGust;
}

uint16_t WeatherClient::getHumidity() {
	return humidity;
}

uint16_t WeatherClient::getAlertCount() {
	return alertIndex;
}

String WeatherClient::getAlertDescription(uint16_t i) {
	return description[i];
}

String WeatherClient::getAlertTitle(uint16_t i) {
	return title[i];
}

String WeatherClient::getAlertSeverity(uint16_t i) {
	return severity[i];
}

String WeatherClient::getRainIn() {
	return rainIn;
}

String WeatherClient::getRainDay() {
	return rainDay;
}

// The key basically tells us which set of data from the JSON is coming
void WeatherClient::key(String key) {
	// Serial.println("Push key " + key);
	push(key);
}

// Time to use the parent() to figure out which section of JSON we are in
// and current() to know which value we got
void WeatherClient::value(String value) {
	if (parent() == "currently") {
		if (current() == "nearestStormDistance") { 
			// Serial.println("nearestStormDistance " + value);
			nearestStormDistance = value.toInt();
		}
		if (current() == "summary") {  
			Serial.println("summary " + value);
			summary = value;
		}
		if (current() == "icon") { 
			Serial.println("icon " + value);
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
		if (current() == "humidity") { 
			Serial.println("humidity " + value);
			double humid_float = value.toFloat()*100;
			humidity = (int) humid_float;
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
		Serial.println("Got alerts");
		if (current() == "description") { 
			// Serial.println("description " + value);
			description[alertIndex] = value;
		} else if (current() == "severity") { 
			Serial.println("severity " + value);
			severity[alertIndex] = value;
		} else if (current() == "title") {
			Serial.println("title " + value);
			title[alertIndex] = value;
		}
	} else if (parent() == "data") {
/* 		if (current() == "temperatureMax") {
			Serial.print("save tempMax to index " + String(dailyIndex));
			Serial.println(", temperatureMax " + value);
			temperatureMax[dailyIndex] = value.toFloat();
		} */
	} else {
		//Serial.println("unused parent " + parent());
		if (current() == "hourlyrainin") {
			rainIn = value;
			Serial.println("rainIn " + rainIn);
		}
		if (current() == "dailyrainin") {
			rainDay = value;
			Serial.println("rainDay " + rainDay);
		}
	}
	if (current() != "regions") {
		pop();
	} else {
		Serial.println("value: not popping stack since doing regions");
	}
}

void WeatherClient::whitespace(char c) {
    //Serial.println(F("whitespace"));
}

void WeatherClient::startDocument() {
    // Serial.println(F("start document"));
	parentIndex = 0;		// Empty the stack for each document we process
}
void WeatherClient::endDocument() {
    // Serial.println(F("end document"));
	// for (int i=0; i<dailyIndex; i++) {
	// 	Serial.println(temperatureMax[i]);
	// }
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
	if (current() == "regions") {
		Serial.println("endArray: Pop regions off of stack");
		pop();
	}
}

void WeatherClient::startObject() {
	Serial.print("In startObject ");
	Serial.print("parent " + parent());
	Serial.println(", current " + current());
}
void WeatherClient::endObject() {
    Serial.print("endObject before pop: ");
	Serial.print("parent " + parent());	Serial.println(", current " + current());
	if (current() == "alerts") {
		alertIndex++;
		Serial.println("Increase alertIndex, now " + String(alertIndex));
	} else if (parent() == "daily") {
		// Serial.println("Increase dailyIndex");
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
		// Serial.println("Pop " + parents[parentIndex]);
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