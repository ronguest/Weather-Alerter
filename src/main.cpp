#include <Arduino.h>
#include "settings.h"
#include "display.h"

#define TFT_RST -1
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

//void sendNTPpacket(IPAddress&);
long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

WeatherClient weather(1);
void drawUpdate();

// Red LED output on the M0 Feather
const int ledPin = 13;

void setup() {
    //   time_t ntpTime;
    Serial.begin(115200);
    delay(2000);

    //Configure pins for Adafruit M0 ATWINC1500 Feather
    WiFi.setPins(8, 7, 4, 2);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    Serial.println("FeatherWing TFT");
    if (!ts.begin()) {
        Serial.println("Couldn't start touchscreen controller");
        while (1)
            delay(500);
    }
    Serial.println("Touchscreen started");

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(WX_BLACK);
    tft.setFont(&largeFont);
    tft.setTextColor(WX_CYAN, WX_BLACK);
    //   ui.setTextAlignment(CENTER);
    tft.setCursor(120, 160);
    tft.println(F("Connecting to WiFi"));
    //   tft.drawString(120, 160, F("Connecting to WiFi"));

    yield();

    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println(F("No Wifi"));
        // don't continue
        while (true) delay(1000);
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print(F("Wifi connect to: "));
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection
        delay(5000);
    }

    // Set up SD card to read icons/moon files
    Serial.print("Initializing SD card...");
    if (!SD.begin(SD_CS)) {
        Serial.println("SD failed!");
    } else {
        Serial.println("SD OK!");
    }
}

void loop() {
    // Check if we should update weather information
    if ((millis() - lastDownloadUpdate) > (1000 * UPDATE_INTERVAL_SECS)) {
		weather.updateConditions(DS_KEY, DS_location);
		lastDownloadUpdate = millis();
		drawUpdate();
    }
    delay(1000);
    yield();
}

void drawUpdate() {
	tft.fillScreen(WX_BLACK);
	tft.setCursor(10,30);
	tft.print(weather.getTemperature());
	tft.setCursor(150,20);
	tft.print(weather.getCurrentIcon());
	tft.setCursor(300,20);
	tft.print(weather.getHumidity());
	tft.setCursor(100,300);
	tft.print(weather.getWindSpeed());
	tft.setCursor(300,300);
	tft.print(weather.getWindGust());
}