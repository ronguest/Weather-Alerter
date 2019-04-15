#include <Arduino.h>
#include "settings.h"
#include "display.h"

#define TFT_RST -1
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

//void sendNTPpacket(IPAddress&);
long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

WeatherClient weather(1);
Adafruit_ImageReader reader;     // For icon display
ImageReturnCode imageStatus; 	// Status from image-reading functions
DisplayMode displayMode;
boolean updateSuccess;
int pageNumber = 0;			// Which alert text to display

void drawUpdate();
void drawAlert(int index);
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
    tft.setCursor(50, 160);
    tft.println(F("Connecting to WiFi"));

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
        // wait for connection
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
    // Check if time to update weather information
    if ((millis() - lastDownloadUpdate) > (1000 * UPDATE_INTERVAL_SECS)) {
		tft.fillCircle(470, 10, 5, HX8357_BLUE);		// Blue status dot means update in progress
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
	int y = 20;
	int textLength;
	int finalSpace;
	int maxLines = 20;
	int maxPerLine = 50;
	int lineSize = 20;
	int startPoint = 0;   // Position in text of next character to print
	int16_t x1, y1;
	uint16_t w, h;

	tft.fillScreen(WX_BLACK);
	tft.setFont(&smallFont);
	tft.setTextColor(WX_CYAN, WX_BLACK);
	if (updateSuccess) {
		tft.fillCircle(470, 10, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(470, 10, 5, HX8357_RED);
	}	

	tft.setCursor(10,y);
	tft.print(weather.getAlertTitle(index));
	y += lineSize + 10; 	// Add a little extra space after title

	textLength = weather.getAlertDescription(index).length();
	while ((startPoint < textLength) && (maxLines > 0)) {
		// Take initial cut at finding the last space in the next string we will print
		finalSpace = weather.getAlertDescription(index).lastIndexOf(' ', startPoint + maxPerLine);
		if (finalSpace == -1 ) {
			// It's possible the final substring doesn't have a space
			finalSpace = textLength;
		}

		// The NWS uses ALL CAPS in some text and lines full of all caps take up a lot more width
		// So we check the text bounds width and if it is wider than we want we reduce character count
		// 430 seems the practical max, out of 480
		tft.getTextBounds(weather.getAlertDescription(index).substring(startPoint, finalSpace), 10, y, &x1, &y1, &w, &h);
		int mult = 0;
		while (w > 430) {
			// The initial cut will be too wide. Shorten line until it fits by backing up the count of max characters per line
			mult++;
			finalSpace = weather.getAlertDescription(index).lastIndexOf(' ', startPoint + maxPerLine - (mult*5));
			tft.getTextBounds(weather.getAlertDescription(index).substring(startPoint, finalSpace), 10, y, &x1, &y1, &w, &h);			
		}
		// Serial.print("Final space: ");Serial.println(finalSpace);
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
		tft.fillCircle(470, 10, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(470, 10, 5, HX8357_RED);
	}	
	tft.setFont(&largeFont);
	tft.setCursor(50,75);
	tft.print((int)weather.getTemperature());
	tft.setFont(&smallFont);
	tft.setCursor(tft.getCursorX(),tft.getCursorY()-17);
	tft.print("o");			// ˚
	char file[64] = "/Icons/";
	strcat(file, mapIcon(weather.getCurrentIcon()).c_str());
	strcat(file, ".bmp");
	imageStatus = reader.drawBMP(file, tft, 200, 5);
	tft.setFont(&largeFont);
	tft.setCursor(350,75);
	tft.print(weather.getHumidity());
	tft.setFont(&smallFont);
	tft.setCursor(tft.getCursorX(),tft.getCursorY()-14);
	tft.print("\045");			// % 

	int y = 125;
	tft.setFont(&smallFont);
	if (weather.getAlertCount() > 0 ) {
		Serial.println("Alert count: " + String(weather.getAlertCount()));
		for (int i=0; i < min(weather.getAlertCount(), 4); i++) {
			tft.setCursor(20,y);
			if (weather.getAlertSeverity(i) == 3) {
				tft.setTextColor(WX_RED);
			} else if (weather.getAlertSeverity(i) == 2) {
				tft.setTextColor(WX_YELLOW);
			} else {
				tft.setTextColor(WX_WHITE);
			}
			tft.print(weather.getAlertTitle(i));
			y += 25;
		}
	} else {
		tft.setFont(&largeFont);
		y += 10;
		tft.setCursor(20, y);
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
	tft.print(weather.getWindGust());
	tft.print(" mph");

	tft.setCursor(280, 300);
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
