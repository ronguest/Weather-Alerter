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
DisplayMode displayMode;
boolean updateSuccess;

int freeMemory();

void drawUpdate();
void drawAlert();
String mapIcon(String s);

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

	displayMode = standard;
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(WX_BLACK);
    tft.setFont(&largeFont);
    tft.setTextColor(WX_CYAN, WX_BLACK);
    //   ui.setTextAlignment(CENTER);
    tft.setCursor(50, 160);
    tft.println(F("Connecting to WiFi"));
    //   tft.drawString(120, 160, F("Connecting to WiFi"));

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
	boolean success1, success2;
    // Check if we should update weather information
    if ((millis() - lastDownloadUpdate) > (1000 * UPDATE_INTERVAL_SECS)) {
		tft.fillCircle(450, 10, 5, HX8357_BLUE);
		success1 = weather.updateConditions(DS_KEY, DS_location);
		success2 = weather.updateLocal(AW_DEVICE, AW_APP_KEY, AW_API_KEY);
		updateSuccess = success1 & success2;
		lastDownloadUpdate = millis();
		if (displayMode == standard) {
			drawUpdate();
		} else {
			drawAlert();
		}
    }
	  // If user touches screen, toggle between weather overview and the detailed forecast text
	if (ts.touched()) {
		Serial.println("Touched");
		if (displayMode == standard) {
			displayMode = alert;
			drawAlert();
		} else {
			displayMode = standard;
			drawUpdate();
		}
		delay(400);		// "debounce"
	}
    delay(100);
}

void drawAlert() {
	tft.fillScreen(WX_BLACK);
	if (updateSuccess) {
		tft.fillCircle(450, 10, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(450, 10, 5, HX8357_RED);
	}	
	tft.setFont(&smallFont);
	// tft.setTextSize(2);
	tft.setCursor(20,50);
	tft.print(weather.getAlertDescription(0));
}

// Display is 480x320
void drawUpdate() {
	tft.fillScreen(WX_BLACK);
	if (updateSuccess) {
		tft.fillCircle(450, 10, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(450, 10, 5, HX8357_RED);
	}	
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
	// reader.printStatus(imageStatus);   // How'd we do?  	
	tft.setCursor(350,75);
	tft.print(weather.getHumidity());
	tft.print("\045");

	int y = 115;
	tft.setFont(&smallFont);
	if (weather.getAlertCount() > 0 ) {
		Serial.println("Alert count: " + String(weather.getAlertCount()));
		for (int i=0; i < weather.getAlertCount(); i++) {
			tft.setCursor(20,y);
			// tft.print(weather.getAlertSeverity(0));
			// tft.print(": ");
			if (weather.getAlertSeverity(0) == "warning") {
				tft.setTextColor(WX_RED);
			} else if (weather.getAlertSeverity(0) == "watch") {
				tft.setTextColor(WX_YELLOW);
			} else {
				tft.setTextColor(WX_WHITE);
			}
			tft.print(weather.getAlertTitle(0));
			y += 20;
		}
		// tft.setCursor(20,y);
		// tft.print("Alert list done");
	} else {
		tft.setCursor(20, y);
		tft.print("No alerts");
	}
	tft.setTextColor(WX_CYAN);
	tft.setCursor(20,300);
	tft.print(weather.getWindSpeed());
	tft.print(" mph || gusting ");
	// tft.setCursor(220,300);
	tft.print(weather.getWindGust());
	tft.print(" mph");

	tft.setCursor(300, 300);
	tft.print(weather.getRainIn());
	tft.print("\"/hr || ");
	tft.print(weather.getRainDay());
	tft.print("\" today");
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

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}