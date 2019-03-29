//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <SPI.h>
#include <SD.h>

#define MAX_FORECAST_PERIODS 24  // Changed from 7 to 12 to support 6 day / 2 screen forecast (Neptune)

class WeatherClient: public JsonListener {
  private:
    String currentKey;
    String currentParent = "";
    String currentTemp;
    int currentForecastPeriod;
    int forecastIcon [MAX_FORECAST_PERIODS];
    String forecastDayOfWeek [MAX_FORECAST_PERIODS];    // 1 per calendar day, up to 6
    String forecastLowTemp [MAX_FORECAST_PERIODS];      // For high & low WU provides one value per calendar day, currently returns 6
    String forecastHighTemp [MAX_FORECAST_PERIODS];     // For high & low WU provides one value per calendar day, currently returns 6
    String forecastText [MAX_FORECAST_PERIODS];              // WU provides a narrative for a day as well as 12 hour periods. We use Night & Day so 12 entries
    String moonAge[MAX_FORECAST_PERIODS];               // WU provides 1 per calendar day
    String sunriseTime[MAX_FORECAST_PERIODS];           // WU provides 1 per calendar day
    String sunsetTime[MAX_FORECAST_PERIODS];            // WU provides 1 per calendar day
    String moonriseTime[MAX_FORECAST_PERIODS];          // WU provides 1 per calendar day
    String moonsetTime[MAX_FORECAST_PERIODS];           // WU provides 1 per calendar day
    void doUpdate(int port, char server[], String url);

  public:
    WeatherClient(boolean foo);
    void updateConditions(String appKey, String location);
    
    String getMoonAge();
    String getSunriseTime();
    String getSunsetTime();
    String getMoonriseTime();
    String getMoonsetTime();
    String getCurrentTemp();
    String getTodayIcon();
    String getTonightIcon();
    String getTomorrowIcon();
    String getTomorrowNightIcon();
    String getTodayName();
    String getTomorrowName();
    String getTodayForecastHigh();
    String getTodayForecastLow();
    String getTomorrowForecastHigh();
    String getTomorrowForecastLow();
    String getTodayForecastTextAM();
    String getTodayForecastTextPM();
    String getTomorrowForecastTextAM();
    String getTomorrowForecastTextPM();
    String getForecastDayOfWeek(int period);
    String getMeteoconIcon(int iconCode);
    
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
