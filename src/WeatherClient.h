//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <SD.h>
#include <SPI.h>

const int maxAlerts = 32;
const uint16_t maxDaily = 32;
const uint16_t parentSize = 10;

class WeatherClient : public JsonListener {
   private:
    void doUpdate(int port, char server[], String url);
	String currentKey;
    // "currently" condition values
    String currentIcon;
    uint16_t nearestStormDistance;
    uint16_t precipProbability;
    String summary;
    double temperature;
    uint16_t windSpeed;
    uint16_t windGust;
	uint16_t humidity;
	String rainIn;
	String rainDay;

    // "alerts" values
	uint16_t alertIndex;
    String description[maxAlerts];
    uint16_t expires[maxAlerts];
    String severity[maxAlerts];
    String title[maxAlerts];
	// "daily" values
	// double temperatureMax[maxDaily];
	// Stack functions to keep track of keyword hierarchy/context
	void push(String s);
	String pop();
	String parent();
	String current();

   public:
    WeatherClient(boolean foo);
    void updateConditions(String appKey, String location);
    void updateLocal(String device, String appKey, String apiKey);
    uint16_t getNearestStormDistance();
    uint16_t getPrecipProbability();
    String getSummary();
	uint16_t getHumidity();
	uint16_t getWindSpeed();
	uint16_t getWindGust();
	uint16_t getAlertCount();
	String getAlertDescription(uint16_t ind);
	String getAlertTitle(uint16_t ind);
	String getAlertSeverity(uint16_t ind);
	double getTemperature();
	String getCurrentIcon();
	String getRainIn();
	String getRainDay();

    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject();
};
