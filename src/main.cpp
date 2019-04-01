#include <Arduino.h>
#include "settings.h"
#include "display.h"

#define TFT_RST -1
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

//void sendNTPpacket(IPAddress&);
long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

WeatherClient weather(1);
Adafruit_ImageReader reader;     // Class w/image-reading functions
ImageReturnCode imageStatus; 	// Status from image-reading functions

void drawUpdate();
String mapIcon(String s);

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
    tft.setCursor(50, 160);
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

// Display is 480x320
void drawUpdate() {
	tft.fillScreen(WX_BLACK);
	tft.setFont(&largeFont);
	// tft.setTextSize(2);
	tft.setCursor(50,75);
	tft.print((int)weather.getTemperature());
	tft.print("F");
	// tft.setCursor(150,50);
	// tft.print(weather.getCurrentIcon());
	char file[64] = "/Icons/";
	strcat(file, mapIcon(weather.getCurrentIcon()).c_str());
	strcat(file, ".bmp");
// String icon = "/icons/" + mapIcon(weather.getCurrentIcon()) + ".bmp";
	imageStatus = reader.drawBMP(file, tft, 200, 5);
	reader.printStatus(imageStatus);   // How'd we do?  	
	tft.setCursor(350,75);
	tft.print(weather.getHumidity());
	tft.print("\045");
	tft.setFont(&smallFont);
	tft.setCursor(220,240);
	tft.print(weather.getWindSpeed());
	tft.print("mph");
	tft.setCursor(220,300);
	tft.print(weather.getWindGust());
	tft.print("mph");
}

// Map from Dark Sky's icon names to ones we know on SD card
String mapIcon(String s) {
	if ((s == "clear-day") || (s == "clear-night")) {
		return "clear";
	}
	if (s == "rain") {
		return "rain";
	}
	if (s == "snow") {
		return "snow";
	}
	if (s == "sleet") {
		return "sleet";
	}
	if (s== "wind") {
		return "hazy";
	}
	if (s == "fog") {
		return "fog";
	}
	if (s == "cloudy") {
		return "cloudy";
	}
	if ((s == "partly-cloudy-day") || (s == "partly-cloudy-night")) {
		return "pcloudy";
	}
	return "unknown";
}