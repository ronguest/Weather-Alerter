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

class WeatherClient : public JsonListener {
   private:
    void doUpdate(int port, char server[], String url);
    String currentKey;
    String currentParent = "";
    // "currently" condition values
    String currentIcon;
    uint16_t nearestStormDistance;
    uint16_t precipProbability;
    String summary;
    double temperature;
    double windSpeed;
    double windGust;
    // "alerts" values
	uint16_t alertIndex;
    String description[maxAlerts];
    uint16_t expires[maxAlerts];
    String severity[maxAlerts];
    String title[maxAlerts];
	// "daily" values
	uint16_t dailyIndex;
	double temperatureMax[maxDaily];

   public:
    WeatherClient(boolean foo);
    void updateConditions(String appKey, String location);
    uint16_t getNearestStormDistance();
    uint16_t getPrecipProbability();
    String getSummary();

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
