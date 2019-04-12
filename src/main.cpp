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

void drawUpdate();
void drawAlert(int index);
String mapIcon(String s);
int pageNumber = 0;			// Which alert text to display

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
			drawAlert(0);
		}
    }
	  // If user touches screen, toggle between weather overview and each alert
	if (ts.touched()) {
		Serial.println("Touched");
		if ((displayMode == standard) && (weather.getAlertCount() > 0) ){
			// On first touch switch to alert mode and display first alert
			pageNumber = 0;
			displayMode = alert;
			drawAlert(pageNumber);
		} else if ((displayMode == alert) && (pageNumber < weather.getAlertCount()-1) ){
			// Go to next alert in list
			pageNumber++;
			drawAlert(pageNumber);
		} else {
			// Switch back to the front page if done showing alerts
			displayMode = standard;
			drawUpdate();
		}
		delay(400);		// "debounce"
	}
    delay(100);
}

// Draw a page of alert details
void drawAlert(int index) {
	tft.fillScreen(WX_BLACK);
	tft.setFont(&smallFont);
	tft.setTextColor(WX_CYAN, WX_BLACK);
	if (updateSuccess) {
		tft.fillCircle(450, 10, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(450, 10, 5, HX8357_RED);
	}	
	int y = 20;
	int textLength;
	int finalSpace;
	int maxLines = 12;
	int maxPerLine = 50;
	int lineSize = 20;
	int startPoint = 0;   // Position in text of next character to print

	textLength = weather.getAlertDescription(index).length();
	while ((startPoint < textLength) && (maxLines > 0)) {
	// Find the last space in the next string we will print
	finalSpace = weather.getAlertDescription(index).lastIndexOf(' ', startPoint + maxPerLine);
	if (finalSpace == -1 ) {
		// It's possible the final substring doesn't have a space
		finalSpace = textLength;
	}
	//Serial.print("Final space: ");Serial.println(finalSpace);
	// If the first character is a space, skip it (happens due to line wrapping)
	if (weather.getAlertDescription(index).indexOf(' ', startPoint) == startPoint) {
		startPoint++;
	}
	tft.setCursor(10,y);
	tft.print(weather.getAlertDescription(index).substring(startPoint, finalSpace));
	y += lineSize;
	startPoint = finalSpace;
	//Serial.print("Start point: ");Serial.println(startPoint);
	maxLines--;
	}	
}

// Display is 480x320
// Draws the "front page"
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
	tft.setFont(&largeFont);
	if (weather.getAlertCount() > 0 ) {
		Serial.println("Alert count: " + String(weather.getAlertCount()));
		for (int i=0; i < max(weather.getAlertCount(), 4); i++) {
			tft.setCursor(20,y);
			// tft.print(weather.getAlertSeverity(0));
			// tft.print(": ");
			if (weather.getAlertSeverity(i) == 3) {
				tft.setTextColor(WX_RED);
			} else if (weather.getAlertSeverity(i) == 2) {
				tft.setTextColor(WX_YELLOW);
			} else {
				tft.setTextColor(WX_WHITE);
			}
			tft.print(weather.getAlertTitle(i));
			y += 35;
		}
		// tft.setCursor(20,y);
		// tft.print("Alert list done");
	} else {
		tft.setCursor(20, y);
		//tft.print("No alerts");
		tft.print(weather.getSummary());
	}
	tft.setFont(&smallFont);
	tft.setTextColor(WX_CYAN);
	if (weather.getNearestStormDistance() < 75) {
		tft.setCursor(20, 275);
		tft.print("Nearest storm: "); tft.print(weather.getNearestStormDistance()); tft.print(" miles");
	}
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
