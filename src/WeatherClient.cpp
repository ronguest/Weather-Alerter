//
//  Copyright (C) 2027-2029 Ronald Guest <http://ronguest.net>
//

#include "WeatherClient.h"
#include <WiFi101.h>

// Stack variables
String parents[parentSize];
uint16_t parentIndex = 0;

boolean inArray = false;
uint16_t dailyIndex;
uint16_t alertIndex;

int cmpfunc(const void * a, const void * b) {
	Alert *alertA = (Alert *)a;
	Alert *alertB = (Alert *)b;
	// Serial.print("alertA.severity "); Serial.println(alertA->severity);
	// Serial.print("alertB.severity "); Serial.println(alertB->severity);
	// Serial.print("b > a? "); Serial.println(alertB->severity - alertA->severity);
	return (alertB->severity - alertA->severity);
}

WeatherClient::WeatherClient(boolean foo) {
}

boolean WeatherClient::updateConditions(String apiKey, String location) {
	boolean result;
	// Reset the alertIndex to 0 here since this is the only JSON document containing alerts
	// Also dailyIndex because that only applies to the Dark Sky reponse as well (not AW)
	alertIndex = 0;
	dailyIndex = 0;
    result = doUpdate(443, "api.darksky.net", "/forecast/" + apiKey + "/" + location + "?exclude=minutely,hourly");
	// Fake alert for text display testing
/* 	alertIndex = 2;
	alerts[0].severity = 3;
	alerts[0].title = "Severity title";
	alerts[0].description = "This is a very long line of text which should get broken up into two lines if I am very very luck";
	alerts[1].severity = 2;
	alerts[1].title = "Severity title2";
	alerts[1].description = "This is ANOTHER very long line of text which should get broken up into two lines if I am very very luck"; */
	// Fake alert data for sort testing
	/* alertIndex = 3;
	for (int i=0; i<alertIndex; i++) {
		alerts[i].severity = i+1;
		alerts[i].title = String(i+1);
		alerts[i].description = String(i+1);
	}
	Serial.println("Before sort");
	for (int i=0; i<alertIndex; i++) {
		Serial.print(i); Serial.print(": "); Serial.println(alerts[i].severity);
	}	 */
	qsort(alerts, alertIndex, sizeof(Alert), cmpfunc);
/* 	Serial.println("After soft");
	for (int i=0; i<alertIndex; i++) {
		Serial.print(i); Serial.print(": "); Serial.println(alerts[i].severity);
	} */
	return result;
}

boolean WeatherClient::updateLocal(String device, String appKey, String apiKey) {
  return doUpdate(443, "api.ambientweather.net", "/v1/devices/" + device + "?applicationKey=" + appKey + "&apiKey=" + apiKey + "&limit=1");
}

boolean WeatherClient::doUpdate(int port, char server[], String url) {
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
            return false;
        }
    } else {
        if (!client.connect(server, port)) {
            Serial.println("connection failed");
            return false;
        }
    }

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait some seconds to start receiving a response
    int retryCounter = 0;
    while (!client.available()) {
        delay(500);
        retryCounter++;
        if (retryCounter > 15) {
            Serial.println(F("Retry timed out"));
            return false;
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
	return true;
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
		} else if (current() == "summary") {  
			// Serial.println("summary " + value);
			summary = value;
		} else if (current() == "icon") { 
			// Serial.println("icon " + value);
			currentIcon = value;
		} else if (current() == "precipProbability") { 
			// Serial.println("precipProbability " + value);
			precipProbability = value.toInt();
		} 
	} else if (parent() == "alerts") {
		if (current() == "description") { 
			// Serial.println("description " + value);
			alerts[alertIndex].description = value;
		} else if (current() == "severity") { 
			Serial.println("severity " + value);
			// Convert severity to int for sorting
			int sev = 1;	// default 1 = advisory/other
			if (value == "warning") {
				sev = 3;
			} else if (value == "watch") {
				sev = 2;
			}
			alerts[alertIndex].severity = sev;
		} else if (current() == "title") {
			Serial.println("title " + value);
			alerts[alertIndex].title = value;
		}
	} else if (parent() == "data") {
/* 		if (current() == "temperatureMax") {
			Serial.print("save tempMax to index " + String(dailyIndex));
			Serial.println(", temperatureMax " + value);
			temperatureMax[dailyIndex] = value.toFloat();
		} */
	} else {
		// Either an unused/unknown parent or it is the Ambient Weather data which has no parent
		if (current() == "hourlyrainin") {
			rainIn = value;
			// Serial.println("rainIn " + rainIn);
		} else if (current() == "dailyrainin") {
			rainDay = value;
			// Serial.println("rainDay " + rainDay);
		} else if (current() == "windspeedmph") {
			windSpeed = (int) round(value.toFloat());
		} else if (current() == "windgustmph") {
			windGust = (int) round(value.toFloat());
		} else if (current() == "tempf") { 
			// Serial.println("temperature " + value);
			temperature = (int) round(value.toFloat());
		} else if (current() == "humidity") { 
			// Serial.println("humidity " + value);
			humidity = value.toInt();
		}
	}
	// Values in "regions" for alerts don't have keys so there is nothing to pop in that case
	if ((parent() == "alerts") && (current() == "regions")) {
		// Serial.println("value: not popping stack since doing alerts regions");
	} else {
		pop();
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
}

// startArray lets us know the key has a set of values
void WeatherClient::startArray() {
	inArray = true;
    // Serial.print("startArray ");
	// Serial.print("parent " + parent());
	// Serial.println(", current " + current());
}
void WeatherClient::endArray() {
	inArray = false;
    // Serial.print("endArray ");
	// Serial.print("parent " + parent());
	// Serial.println(", current " + current());
	// Special case because "regions" values do not have keys
	// So when the "regions" array ends pop it off the stack
	if ((parent() == "alerts") && (current() == "regions")) {
		// Serial.println("endArray: Pop regions off of stack");
		pop();
	}
}

void WeatherClient::startObject() {
	// Serial.print("In startObject ");
	// Serial.print("parent " + parent());
	// Serial.println(", current " + current());
}
void WeatherClient::endObject() {
    // Serial.print("endObject before pop: ");
	// Serial.print("parent " + parent());	Serial.println(", current " + current());
	if (current() == "alerts") {
		if (alertIndex < maxAlerts) {
			alertIndex++;
		}
		// Serial.println("Increase alertIndex, now " + String(alertIndex));
	} else if (parent() == "daily") {
		// Serial.println("Increase dailyIndex");
		if (dailyIndex < maxDaily) {
			dailyIndex++;
		}
	}	

	// Objects in an array don't have a key/name so we can't pop in that case or we lost parentage
	if (!inArray && (current() != "alerts")) {
		pop();
	}
}

String WeatherClient::getSummary() {
	return summary;
}

String WeatherClient::getCurrentIcon() {
	return currentIcon;
}

uint16_t WeatherClient::getTemperature() {
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
	return alerts[i].description;
}

String WeatherClient::getAlertTitle(uint16_t i) {
	return alerts[i].title;
}

uint16_t WeatherClient::getAlertSeverity(uint16_t i) {
	return alerts[i].severity;
}

uint16_t WeatherClient::getNearestStormDistance() {
	return nearestStormDistance;
}

String WeatherClient::getRainIn() {
	return rainIn;
}

String WeatherClient::getRainDay() {
	return rainDay;
}

// Stack implementation for keys
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
