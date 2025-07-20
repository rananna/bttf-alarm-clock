/**
 * 
 * Back to the Future Alarm Clock
 * =================================  
 * Author: Randall North              // Your name here
 * Version: 1.0.8 (July 7, 2025)  // Version number and release date
 * Board: ESP32 Dev Module        // Target hardware
 * License: MIT                   // Software license
 * 
 * Description:
 * A WiFi-connected alarm clock inspired by Back to the Future time circuits,
 * with multiple displays showing current, destination and last departure times.
 * 
 * Features:
 * - NTP time synchronization with multiple fallback servers    // Reliable time keeping
 * - Configurable alarm with snooze functionality               // Wake-up capability
 * - OTA updates for easy firmware upgrades                     // Remote updating
 * - Display brightness control with day/night settings         // Eye comfort
 * - Full web configuration interface (http://bttf-alarmclock.local) // Easy configuration
 * - Time travel animations and sound effects                   // Movie authenticity
 * - Multiple display themes (Green, Red, Amber, Blue)          // User customization
 * - Power saving mode during specified hours                   // Energy efficiency
 * - Full time zone support including daylight saving times     // Timekeeping flexibility
 */

#include "Arduino.h"                // Core Arduino functions
#include "TM1637Display.h"          // Library for 7-segment LED displays
#include "WiFiManager.h"            // Handles WiFi connection and captive portal
#include <Wire.h>                   // I2C communication protocol library
#include "Adafruit_GFX.h"           // Graphics library for display control
#include "Adafruit_LEDBackpack.h"   // Controls LED matrix and 7-segment displays
#include "ArduinoOTA.h"             // Over-The-Air firmware updates
#include "ESPmDNS.h"                // Multicast DNS for local hostname resolution
#include "DFRobotDFPlayerMini.h"    // Controls MP3 module for sound effects
#include <WiFi.h>                   // ESP32 WiFi functionality
#include <ESPAsyncWebServer.h>      // Non-blocking web server for configuration interface
#include <Preferences.h>            // ESP32 non-volatile storage for persistent settings
#include <ArduinoJson.h>            // JSON parsing for web API communication
#include <time.h>                   // Time functions for clock operations
#include <WiFiUdp.h>                // UDP protocol for NTP time synchronization
//#include <NTPClient.h>              // NTP client for Internet time synchronization

#define green_CLK 13              // Clock pin for all TM1637 displays
#define green1_DIO 18             // Data pin for the first TM1637 display
#define green2_DIO 15             // Data pin for the second TM1637 display
#define green3_DIO 14             // Data pin for the third TM1637 display
#define greenAM 27                // Output pin to control the AM indicator LED
#define greenPM 12                // Output pin to control the PM indicator LED
#define SET_STOP_BUTTON 34        // Input pin for the Set/Stop button (used to stop alarm or enter settings mode)
#define SET_STOP_LED 26           // Output pin for the LED indicating Set/Stop status
#define SET_SOUND_BUTTON 4        // Input pin for the Set/Sound button (likely for testing sounds or toggling sound-related settings)
#define SET_SOUND_LED 2             // Output pin for the LED indicating Set/Sound status
#define HOUR_BUTTON 33            // Input pin for incrementing the hour in settings mode
#define MINUTE_BUTTON 32          // Input pin for incrementing the minute in settings mode
#define FPSerial Serial1            // Serial port used for communication with the DFPlayer Mini MP3 module

static bool nighttimeonlyonce=true;
struct tm timeinfo;

const byte RXD2 = 16;           // RX pin connected to the DFPlayer Mini's TX pin
const byte TXD2 = 17;           // TX pin connected to the DFPlayer Mini's RX pin


DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

float counter = 0;

int sound_minutes = 0;        // Variable to store minutes for sound-related settings (currently unused)
int sound_hours = 0;          // Variable to store hours for sound-related settings (currently unused)

int flag_alarm = 0 ;           // Flag to indicate if the alarm is currently active

int h=0;                      // General purpose variable, often used as a counter in loops or for small calculations
int Play_finished = 0;        // Flag to indicate if the currently playing sound/track has finished
int easter_egg = 0;           // Flag to track if the "easter egg" mode (special animation/sound) is active

int sleeping_time_in_minutes; // Calculated start time of the sleep mode in minutes (from midnight)


int wake_time_in_minutes;     // Calculated end time of the sleep mode in minutes (from midnight)

int currenthour;             // Stores the current hour (0-23)
int currentminute;           // Stores the current minute (0-59)
int current_time_in_minutes;  // Calculated current time in minutes (from midnight)

//int brightness=30;
//int brightness=30;            // Commented-out variable, likely intended for overall brightness control (but 'currentSettings.brightness' is used instead)


uint8_t segments[4];


bool res;
int Hour = 0;
//========================USEFUL VARIABLES=============================
const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT","NOV","DEC","VOL","SND","ALM","EGG","SNZ"};
                                        // Array of month abbreviations and other codes for display on the alphanumeric display
                                        // Includes codes for Volume, Sound, Alarm, Easter Egg, and Snooze
int randomTime = 0;               // Unused variable, possibly intended for generating random times for display
float currentSpeed = 0; // Store the last green1ed speed
float targetSpeed = 88;           // Target speed for the initial "time travel" animation (88 mph)
unsigned long previousMillis = 0;   // General-purpose variable to store the last time an event occurred (in milliseconds)
unsigned long previoussecMillis = 0;// Stores the last time the second-based blink event occurred
unsigned long previoustestMillis = 0;// Another time tracking variable, likely for animations or other periodic events
//const long interval = 3600000; // Interval in milliseconds (1 hour)

const long secinterval = 500; // Interval in milliseconds (0.5 sec ) blink on every second
// unsigned long soundinterval =  60000; // Interval in milliseconds 30 sec )
static int lastexecutedhour=-1;
static int lastexecutedminute=-1;
// --- Animation Control Variables ---
// Prevents animations from triggering too frequently and tracks their timing
unsigned long lastAnimationTriggerTime = 0;  // Timestamp of last animation in milliseconds

int UTC = -4;                      // Unused variable, likely intended to store the UTC offset in hours (but TZ_INFO is used instead)
const long utcOffsetInSeconds = 3600; // Unused constant, likely intended for UTC offset in seconds (but TZ_INFO is used instead)
// Note: Module connection pins defined later in the code

bool changesMade = false;           // Flag to track if settings were modified and need saving

// --- Configuration ---
#define MDNS_HOSTNAME "bttf-alarmclock"   // Hostname for mDNS, allowing access via http://bttf-alarmclock.local/





// Time Zone (for Canada/Eastern - EDT). This string sets the TZ environment variable for `time.h` functions.
// Format: "STDoffsetDSToffset,startdate/time,enddate/time"
// Example: "EST5EDT,M3.2.0,M11.1.0" means:
//   EST = Eastern Standard Time, 5 hours behind UTC (UTC-5)
//   EDT = Eastern Daylight Time (during DST), 4 hours behind UTC (UTC-4)
//   M3.2.0 = Month 3 (March), 2nd Sunday, 00:00:00 (start of DST)
//   M11.1.0 = Month 11 (November), 1st Sunday, 00:00:00 (end of DST)
const char* TZ_INFO = "EST5EDT,M3.2.0,M11.1.0"; 

// --- NTP Server configuration for the custom client ---
// List of NTP server hostnames to try in order.
const char* NTP_SERVERS[] = {
    "pool.ntp.org",
    "time.google.com",
    "time.nist.gov",
    "us.pool.ntp.org" // Another common pool
};
const int NUM_NTP_SERVERS = sizeof(NTP_SERVERS) / sizeof(NTP_SERVERS[0]); // Calculate number of servers
int currentNtpServerIndex = 0; // Index of the server currently being used for retries

const int NTP_PACKET_SIZE = 48; // Standard size of an NTP time stamp message (in bytes)
 byte packetBuffer[NTP_PACKET_SIZE]; // Buffer to hold incoming and outgoing NTP packets

// A UDP instance to let us send and receive packets over UDP for NTP
WiFiUDP Udp;
// Create an NTPClient instance, passing the WiFiUDP object and other parameters

// NTP Sync variables to manage sync status, timing, and retry logic.
unsigned long lastNtpRequestSent = 0; // Stores the millis() when the last NTP request was sent
const long NTP_BASE_INTERVAL_MS = 60000;   // Base interval (60 seconds) to request new time if successful
unsigned long currentNtpInterval = NTP_BASE_INTERVAL_MS; // Current interval, adjusted by exponential backoff on failure
const unsigned long NTP_MAX_INTERVAL_MS = 30 * 60 * 1000; // Max interval (30 minutes) for exponential backoff
unsigned int ntpFailedAttempts = 0; // Counter for consecutive failed NTP sync attempts, used for backoff
                                    // --- Global Objects ---
bool timeSynchronized = false;        // Flag to indicate if time has been successfully synchronized at least once
time_t lastNtpSyncTime = 0;           // Stores the Unix timestamp of the last successful NTP sync
String syncStatusMessage = "Initializing..."; // Detailed message about current sync status for web UI
String currentNtpServerUsed = "None"; // To display which NTP server was last used/attempted for web UI

// --- Global Objects ---
WiFiManager wifiManager;      // WiFiManager object for network setup and provisioning
AsyncWebServer server(80);    // AsyncWebServer object, listening on port 80 (standard HTTP for web UI)
Preferences preferences;      // Preferences object for Non-Volatile Storage (NVS)
// Server-sent events to refresh client browsers
AsyncEventSource events("/events");
// Structure to hold alarm clock settings. This struct's contents are saved/loaded from NVS.
struct ClockSettings {
  int destinationHour;
  int destinationMinute;
  bool alarmOnOff;
  int snoozeMinutes;
  int departureHour;    // Start hour for 'sleep' mode brightness
  int departureMinute;
  int arrivalHour;      // End hour for 'sleep' mode brightness
  int arrivalMinute;
  int brightness;       // Overall display brightness (e.g., for LEDs, 0-7)
  int notificationVolume; // Volume level for alarms/notifications (e.g., 0-30)
  bool timeTravelSoundToggle; // Renamed: Enable/disable time travel sound effects/alarm sounds
  bool powerOfLoveToggle; // New: Toggle for "Power of Love" (e.g., Flux Capacitor animation/light)
  int timeTravelAnimationInterval; // New: Interval in minutes for display animation (0 for disabled)
  bool displayFormat24h; // True for 24-hour format (HH:MM), false for 12-hour (HH:MM AM/PM)
  int theme;            // Theme index (e.g., 0 for Green, 1 for Red, 2 for Amber, 3 for Blue)
  char timezoneString[64];  // String to store the selected timezone
};

// Initialize with default settings - these values are used if NVS read fails or for first boot.
ClockSettings defaultSettings = { 
  .destinationHour = 8,
  .destinationMinute = 30,
  .alarmOnOff = false,
  .snoozeMinutes = 9,
  .departureHour = 23, // 11 PM (Start of sleep mode)
  .departureMinute = 0,
  .arrivalHour = 7,    // 7 AM (End of sleep mode)
  .arrivalMinute = 0,
  .brightness = 5,
  .notificationVolume = 15,
  .timeTravelSoundToggle = true, // Default to true
  .powerOfLoveToggle = false, // Default to false
  .timeTravelAnimationInterval = 10, // Default: Animate every 10 minutes
  .displayFormat24h = true,
  .theme = 0,             // Default to the first theme (Green)
  .timezoneString = "EST5EDT,M3.2.0,M11.1.0"  // Default timezone Canada/Eastern
};

ClockSettings currentSettings; // This variable will hold the actively used settings (loaded from NVS or default)

// --- Function Prototypes ---
// Declaring functions before they are defined allows the compiler to know their signatures.
void configModeCallback(WiFiManager *myWiFiManager); // Callback for WiFiManager config portal entry
void loadSettings();                                 // Load settings from NVS
void saveSettings();                                 // Save settings to NVS
void resetDefaultSettings();                         // Reset settings to factory defaults
void handleApiTime(AsyncWebServerRequest *request);  // Web API handler for current time/sync status
void handleApiSettings(AsyncWebServerRequest *request); // Web API handler for retrieving all settings
void handleApiSaveSettings(AsyncWebServerRequest *request); // Web API handler for saving settings
void handleApiStatus(AsyncWebServerRequest *request); // Web API handler for WiFi/network status
void handleApiResetDefaults(AsyncWebServerRequest *request); // Web API handler for resetting settings
void handleApiResetWifi(AsyncWebServerRequest *request); // Web API handler for resetting WiFi credentials
void notFound(AsyncWebServerRequest *request);       // Web server 404 handler
void triggerBrowserRefresh();






// --- HTML, CSS, JavaScript (Embedded as Raw Strings) ---
// These are served by the web server to provide a configuration interface.
// Using R"raw(...)raw" allows multi-line strings without needing escape characters.

// INDEX_HTML - Main web page structure
const char* INDEX_HTML = R"raw(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 BTTF Alarm Clock</title>
    <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&family=Share+Tech+Mono&display=swap" rel="stylesheet">
    <link rel="stylesheet" type="text/css" href="/style.css">
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
    <div id="messageBanner" class="message-banner"></div>

    <div class="container">
        <h1>Back to the Future Alarm Clock</h1>
        
        <div class="time-display">
            <p>Present Time: <span id="currentTime">--:--:--</span></p>
            <p>Current Date: <span id="currentDate">--/--/----</span></p>
            <p>Time Synchronized: <span id="timeSyncStatus">No</span></p>
            <p>Last Sync Time: <span id="lastSyncTime">Never</span></p>
            <p>Sync Status: <span id="syncStatusMessage">Initializing...</span></p>
            <p>Last NTP Server: <span id="lastNtpServer">N/A</span></p>
            <button id="fetchStatusBtn" onclick="fetchStatus()">Refresh time circuits</button>
            <div id="status"></div>
        </div>

        <hr>

        <h2>Time Circuit Settings</h2>
        <div class="setting-group">
            <h3>Destination (Alarm) Time</h3>
            <label for="destinationHour">
                Hour:
                <div class="slider-with-bar">
                    <input type="range" id="destinationHour" min="0" max="23" value="8">
                    <span id="destinationHourValue">8</span>
                </div>
            </label>
            <label for="destinationMinute">
                Minute:
                <div class="slider-with-bar">
                    <input type="range" id="destinationMinute" min="0" max="59" value="30">
                    <span id="destinationMinuteValue">30</span>
                </div>
            </label>
            <label for="alarmOnOff">Alarm On:</label>
            <label class="toggle-switch">
                <input type="checkbox" id="alarmOnOff">
                <span class="slider round"></span>
            </label>
            <br>
            <label for="snoozeMinutes">Snooze Time (min):
                <div class="slider-with-bar">
                    <input type="range" id="snoozeMinutes" min="1" max="59" value="9">
                    <span id="snoozeMinutesValue">9</span>
                </div>
            </label>
        </div>

        <div class="setting-group">
            <h3>Departure/Arrival (Sleep) Times</h3>
            <label for="departureHour">
                Departure Hour:
                <div class="slider-with-bar">
                    <input type="range" id="departureHour" min="0" max="23" value="23">
                    <span id="departureHourValue">23</span>
                </div>
            </label>
            <label for="departureMinute">
                Minute:
                <div class="slider-with-bar">
                    <input type="range" id="departureMinute" min="0" max="59" value="0">
                    <span id="departureMinuteValue">0</span>
                </div>
            </label>
            <br>
            <label for="arrivalHour">
                Arrival Hour:
                <div class="slider-with-bar">
                    <input type="range" id="arrivalHour" min="0" max="23" value="7">
                    <span id="arrivalHourValue">7</span>
                </div>
            </label>
            <label for="arrivalMinute">
                Minute:
                <div class="slider-with-bar">
                    <input type="range" id="arrivalMinute" min="0" max="59" value="0">
                    <span id="arrivalMinuteValue">0</span>
                </div>
            </label>
        </div>

        <div class="setting-group">
            <h3>Console Controls (Display & Sound)</h3>
            <label for="brightness">
                Display Brightness:
                <div class="slider-with-bar">
                    <input type="range" id="brightness" min="0" max="7">
                    <div id="brightnessBar" class="visual-bar brightness-bar"></div>
                    <span id="brightnessValue">5</span>
                </div>
            </label>

            <label for="notificationVolume">
                Notification Volume:
                <div class="slider-with-bar">
                    <input type="range" id="notificationVolume" min="0" max="30">
                    <div id="volumeBar" class="visual-bar volume-bar"></div>
                    <span id="volumeValue">15</span>
                </div>
            </label>
            
            <label for="timeTravelAnimationInterval">
                Time Travel Animation Every (min, 0=Off):
                <div class="slider-with-bar">
                    <input type="range" id="timeTravelAnimationInterval" min="0" max="60">
                    <span id="timeTravelAnimationIntervalValue">30</span>
                </div>
            </label>
            
            <label for="timeTravelSoundToggle">Time Travel Sound FX On:</label>
            <label class="toggle-switch">
                <input type="checkbox" id="timeTravelSoundToggle">
                <span class="slider round"></span>
            </label><br>

            <label for="powerOfLoveToggle">Power of Love (Flux Capacitor):</label>
            <label class="toggle-switch">
                <input type="checkbox" id="powerOfLoveToggle">
                <span class="slider round"></span>
            </label><br>



            <label for="themeSelect">Theme:</label>
            <select id="themeSelect">
                <option value="0">Time Circuits (Green)</option>
                <option value="1">Biff Tannen (Red)</option>
                <option value="2">1955 (Amber)</option>
                <option value="3">DeLorean (Blue)</option>
            </select>
        </div>

        <div class="setting-group">
            <h3>Temporal Controls</h3>
            <label for="timezoneSelect">Timezone:</label>
            <select id="timezoneSelect">
                <option value="EST5EDT,M3.2.0,M11.1.0" selected>Canada/Eastern</option>
                <option value="PST8PDT,M3.2.0,M11.1.0">USA/Pacific</option>
                <option value="MST7MDT,M3.2.0,M11.1.0">USA/Mountain</option>
                <option value="CST6CDT,M3.2.0,M11.1.0">USA/Central</option>
                <option value="GMT0">Europe/London</option>
                <option value="CET-1CEST,M3.5.0,M10.5.0">Europe/Berlin</option>
                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Athens</option>
                <option value="WST-8">Australia/Perth</option>
                <option value="CET-1CEST,M3.5.0,M10.5.0">Europe/Berlin</option>
                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Athens</option>
                <option value="WST-8">Australia/Perth</option>
                <option value="ACT-9:30AEDT-10:30,M10.1.0,M4.1.0">Australia/Adelaide</option>
                <option value="AEST-10">Australia/Brisbane</option>
                <option value="GST-10">Australia/Sydney</option>
                <option value="NZST-12NZDT-13,M9.5.0,M4.1.0/3">New Zealand/Auckland</option>
                <option value="HST10">US/Hawaii</option>
                <option value="AKST9AKDT8,M3.2.0,M11.1.0">US/Alaska</option>
                <option value="BRT3">America/Sao_Paulo</option>
                <option value="GST-4">Asia/Dubai</option>
                <option value="IST-5:30">Asia/Kolkata</option>
                <option value="EET-2EEST,M4.5.5/0,M10.5.4/0">Africa/Cairo</option>
                <option value="JST-9">Japan/Tokyo</option>
            </select> 


            <label for="displayFormat24h">24 Hour Format:</label>
            <label class="toggle-switch">
                <input type="checkbox" id="displayFormat24h">
                <span class="slider round"></span>
            </label>
            

        </div>

        <button id="saveSettingsBtn" onclick="saveSettings()">Engage! (Save Settings)</button>
        <button id="resetDefaultsBtn" onclick="resetToDefaults()" class="reset-button">Return to 1985 (Reset Settings)</button>
        <button id="resetWifiBtn" onclick="resetWifi()" class="reset-button">Forget Destination (Reset WiFi)</button>
    </div>

    <script src="/script.js"></script>
</body>
</html>
)raw";

// STYLE_CSS - Web page styling
const char* STYLE_CSS = R"raw(
/* Define base CSS variables for colors */
:root {
    --bg-color: #1a1a1a;
    --container-bg-color: #2a2a2a;
    --border-color: #00ff00;
    --shadow-color: rgba(0, 255, 0, 0.7);
    --heading-color: #00ffff;
    --heading-shadow-color: rgba(0, 255, 255, 0.7);
    --text-color: #00ff00;
    --highlight-text-color: #ffcc00;
    --main-time-color: #ff00ff;
    --main-time-shadow-color: rgba(255, 0, 255, 0.7);
    --setting-group-bg: #3a3a3a;
    --setting-group-border: #008800;
    --setting-group-shadow: rgba(0, 255, 0, 0.3);
    --input-bg: #111;
    --input-color: #ffcc00;
    --input-border: #008800;
    --invalid-input-border: #ff0000;
    --invalid-input-shadow: rgba(255, 0, 0, 0.7);
    --slider-thumb-color: #00ffff;
    --slider-thumb-shadow: rgba(0, 255, 255, 0.8);
    --slider-track-color: #005500;
    --slider-track-border: #00ff00;
    --visual-bar-color-1: #ffcc00; /* Brightness */
    --visual-bar-shadow-1: rgba(255, 204, 0, 0.7);
    --visual-bar-color-2: #ff00ff; /* Volume */
    --visual-bar-shadow-2: rgba(255, 0, 255, 0.7);
    --toggle-slider-bg: #444;
    --toggle-slider-border: #00ff00;
    --toggle-slider-before: #00ffff;
    --toggle-slider-checked-bg: #00ff00;
    --button-bg: #006600;
    --button-color: #00ff00;
    --button-border: #00ff00;
    --button-hover-bg: #009900;
    --button-hover-shadow: rgba(0, 255, 0, 0.9);
    --reset-button-bg: #660000;
    --reset-button-border: #ff0000;
    --reset-button-color: #ffcc00;
    --reset-button-shadow: rgba(255, 0, 0, 0.5);
    --reset-button-hover-bg: #990000;
    --reset-button-hover-shadow: rgba(255, 0, 0, 0.9);
    --message-banner-bg: #333;
    --message-banner-color: white;
    --success-banner-bg: #008800;
    --success-banner-color: #e0ffe0;
    --success-banner-border: #00ff00;
    --error-banner-bg: #880000;
    --error-banner-color: #ffe0e0;
    --error-banner-border: #ff0000;
}

body {
    font-family: 'Share Tech Mono', monospace; /* Modern techy font */
    background-color: var(--bg-color);
    color: var(--text-color); /* Green LED-like color */
    margin: 0;
    padding: 20px;
    display: flex;
    justify-content: center;
    align-items: flex-start;
    min-height: 100vh;
    box-sizing: border-box;
}

.container {
    background-color: var(--container-bg-color);
    border: 2px solid var(--border-color);
    box-shadow: 0 0 15px var(--shadow-color);
    padding: 30px;
    border-radius: 10px;
    max-width: 600px;
    width: 100%;
    box-sizing: border-box;
}

h1, h2 {
    font-family: 'Orbitron', sans-serif; /* Sci-fi display font */
    color: var(--heading-color);
    text-align: center;
    text-shadow: 0 0 8px var(--heading-shadow-color);
    margin-bottom: 20px;
}

hr {
    border: none;
    border-top: 1px dashed var(--border-color);
    margin: 30px 0;
}

p {
    margin: 8px 0;
    line-height: 1.5;
}

span {
    font-weight: bold;
    color: var(--highlight-text-color);
}

.time-display p {
    font-size: 1.2em;
    text-align: center;
    margin-bottom: 5px;
}

.time-display span {
    font-size: 1.5em;
    font-family: 'Orbitron', sans-serif;
    color: var(--main-time-color);
    text-shadow: 0 0 10px var(--main-time-shadow-color);
}

.setting-group {
    background-color: var(--setting-group-bg);
    border: 1px solid var(--setting-group-border);
    padding: 20px;
    border-radius: 8px;
    margin-bottom: 25px;
    box-shadow: inset 0 0 5px var(--setting-group-shadow);
}

.setting-group h3 {
    color: var(--text-color);
    text-shadow: 0 0 5px var(--shadow-color);
    margin-top: 0;
    margin-bottom: 15px;
    text-align: center;
}

label {
    display: block;
    margin-bottom: 10px;
    color: var(--text-color);
}

input[type="number"],
input[type="range"],
select { /* Added select element to style inputs */
    width: calc(100% - 20px);
    padding: 8px;
    margin-top: 5px;
    margin-bottom: 10px;
    border: 1px solid var(--input-border);
    background-color: var(--input-bg);
    color: var(--input-color);
    border-radius: 4px;
    box-sizing: border-box;
    -webkit-appearance: none; /* Remove default styling for range and select */
    -moz-appearance: none;
    appearance: none;
}
select {
    padding-right: 30px; /* Make space for custom arrow */
    background-image: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="%2300ff00" d="M7 10l5 5 5-5z"/></svg>');
    background-repeat: no-repeat;
    background-position: right 8px center;
    background-size: 24px;
    cursor: pointer;
}
select::-ms-expand {
    display: none; /* Hide default arrow in IE/Edge */
}


input[type="number"].invalid {
    border-color: var(--invalid-input-border);
    box-shadow: 0 0 5px var(--invalid-input-shadow);
}

.validation-message {
    color: var(--invalid-input-border);
    font-size: 0.8em;
    margin-top: -8px;
    margin-bottom: 10px;
    display: inline-block;
}

/* Custom range slider styling */
input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    height: 18px;
    width: 18px;
    border-radius: 50%;
    background: var(--slider-thumb-color);
    cursor: pointer;
    box-shadow: 0 0 5px var(--slider-thumb-shadow);
    margin-top: -5px; /* Adjust this to center the thumb vertically */
}

input[type="range"]::-moz-range-thumb {
    height: 18px;
    width: 18px;
    border-radius: 50%;
    background: var(--slider-thumb-color);
    cursor: pointer;
    box-shadow: 0 0 5px var(--slider-thumb-shadow);
}

input[type="range"]::-webkit-slider-runnable-track {
    width: 100%;
    height: 8px;
    cursor: pointer;
    background: var(--slider-track-color);
    border-radius: 4px;
    border: 0.5px solid var(--slider-track-border);
}

input[type="range"]::-moz-range-track {
    width: 100%;
    height: 8px;
    cursor: pointer;
    background: var(--slider-track-color);
    border-radius: 4px;
    border: 0.5px solid var(--slider-track-border);
}

.slider-with-bar {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 10px;
        width: 100%;         /* Ensure it takes full width for consistency */
}

.slider-with-bar input[type="range"] {
    flex-grow: 1;
    width: auto; /* Override 100% width for flex container */
    margin-bottom: 0;
}

.visual-bar {
    height: 8px;
    border-radius: 4px;
    background-color: var(--slider-track-color);
    border: 0.5px solid var(--slider-track-border);
    width: 80px; /* Max width of the visual bar */
    flex-shrink: 0; /* Don't let it shrink */
}

.brightness-bar {
    background-color: var(--visual-bar-color-1);
    box-shadow: 0 0 5px var(--visual-bar-shadow-1);
}

.volume-bar {
    background-color: var(--visual-bar-color-2);
    box-shadow: 0 0 5px var(--visual-bar-shadow-2);
}


/* Toggle Switch Styling */
.toggle-switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
    margin-left: 10px;
    vertical-align: middle;
}

.toggle-switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: var(--toggle-slider-bg);
    -webkit-transition: .4s;
    transition: .4s;
    border: 1px solid var(--toggle-slider-border);
}

.slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: var(--toggle-slider-before);
    -webkit-transition: .4s;
    transition: .4s;
}

input:checked + .slider {
    background-color: var(--toggle-slider-checked-bg);
}

input:focus + .slider {
    box-shadow: 0 0 1px var(--toggle-slider-checked-bg);
}

input:checked + .slider:before {
    -webkit-transform: translateX(26px);
    -ms-transform: translateX(26px);
    transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
    border-radius: 34px;
}

.slider.round:before {
    border-radius: 50%;
}


button {
    background-color: var(--button-bg);
    color: var(--button-color);
    border: 1px solid var(--button-border);
    padding: 10px 15px;
    border-radius: 5px;
    cursor: pointer;
    font-size: 1em;
    margin-top: 20px;
    width: 100%;
    box-sizing: border-box;
    transition: background-color 0.3s ease, box-shadow 0.3s ease;
    text-shadow: 0 0 3px var(--shadow-color);
}

button:hover {
    background-color: var(--button-hover-bg);
    box-shadow: 0 0 10px var(--button-hover-shadow);
}

button:disabled {
    background-color: #555;
    color: #aaa;
    border-color: #777;
    cursor: not-allowed;
    box-shadow: none;
}

.reset-button {
    background-color: var(--reset-button-bg);
    border-color: var(--reset-button-border);
    color: var(--reset-button-color);
    text-shadow: 0 0 3px var(--reset-button-shadow);
}

.reset-button:hover {
    background-color: #990000;
    box-shadow: 0 0 10px rgba(255, 0, 0, 0.9);
}

.message-banner {
    position: fixed;
    top: 20px;
    left: 50%;
    transform: translateX(-50%);
    background-color: var(--message-banner-bg);
    color: var(--message-banner-color);
    padding: 10px 20px;
    border-radius: 5px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.5);
    z-index: 1000;
    opacity: 0;
    visibility: hidden;
    transition: opacity 0.5s ease, visibility 0.5s ease;
    white-space: nowrap;
}

.message-banner.success {
    background-color: var(--success-banner-bg);
    color: var(--success-banner-color);
    border: 1px solid var(--success-banner-border);
}

.message-banner.error {
    background-color: var(--error-banner-bg);
    color: #ffe0e0;
    border: 1px solid var(--error-banner-border);
}

/* NEW: Highlight animation for saved fields */
@keyframes highlight-glow {
    0% { box-shadow: 0 0 0px var(--success-banner-border); }
    50% { box-shadow: 0 0 10px var(--success-banner-border), 0 0 20px var(--success-banner-border); }
    100% { box-shadow: 0 0 0px var(--success-banner-border); }
}

.highlight-saved {
    animation: highlight-glow 1.5s ease-out; /* Adjust duration as needed */
}

/* --- Theme Definitions --- */

/* Note: The default theme (Green) is defined in :root. */

body.theme-biff-tannen {
    --border-color: #ff0000;
    --shadow-color: rgba(255, 0, 0, 0.7);
    --heading-color: #ff9900; /* Orange-red */
    --heading-shadow-color: rgba(255, 153, 0, 0.7);
    --text-color: #ff0000;
    --highlight-text-color: #ffddaa; /* Lighter amber */
    --main-time-color: #ff0077; /* Deep pink */
    --main-time-shadow-color: rgba(255, 0, 119, 0.7);
    --setting-group-border: #aa0000;
    --setting-group-shadow: rgba(255, 0, 0, 0.3);
    --input-border: #aa0000;
    --slider-thumb-color: #ff9900;
    --slider-thumb-shadow: rgba(255, 153, 0, 0.8);
    --slider-track-color: #550000;
    --slider-track-border: #ff0000;
    --visual-bar-color-1: #ff6600; /* Orange */
    --visual-bar-shadow-1: rgba(255, 102, 0, 0.7);
    --visual-bar-color-2: #cc00cc; /* Purple */
    --visual-bar-shadow-2: rgba(204, 0, 204, 0.7);
    --toggle-slider-border: #ff0000;
    --toggle-slider-before: #ff9900;
    --toggle-slider-checked-bg: #ff0000;
    --button-bg: #aa0000;
    --button-color: #ff0000;
    --button-border: #ff0000;
    --button-hover-bg: #cc0000;
    --button-hover-shadow: rgba(255, 0, 0, 0.9);
    --success-banner-bg: #aa0000;
    --success-banner-color: #ffe0e0;
    --success-banner-border: #ff0000;
}

body.theme-1955 {
    --border-color: #ffaa00;
    --shadow-color: rgba(255, 170, 0, 0.7);
    --heading-color: #ffddaa;
    --heading-shadow-color: rgba(255, 221, 170, 0.7);
    --text-color: #ffaa00;
    --highlight-text-color: #ffffff;
    --main-time-color: #ff8800;
    --main-time-shadow-color: rgba(255, 136, 0, 0.7);
    --setting-group-border: #aa6600;
    --setting-group-shadow: rgba(255, 170, 0, 0.3);
    --input-border: #aa6600;
    --slider-thumb-color: #ffddaa;
    --slider-thumb-shadow: rgba(255, 221, 170, 0.8);
    --slider-track-color: #553300;
    --slider-track-border: #ffaa00;
    --visual-bar-color-1: #ffffff;
    --visual-bar-shadow-1: rgba(255, 255, 255, 0.7);
    --visual-bar-color-2: #ff4400;
    --visual-bar-shadow-2: rgba(255, 68, 0, 0.7);
    --toggle-slider-border: #ffaa00;
    --toggle-slider-before: #ffddaa;
    --toggle-slider-checked-bg: #ffaa00;
    --button-bg: #aa6600;
    --button-color: #ffaa00;
    --button-border: #ffaa00;
    --button-hover-bg: #cc8800;
    --button-hover-shadow: rgba(255, 170, 0, 0.9);
    --success-banner-bg: #aa6600;
    --success-banner-color: #ffeedd;
    --success-banner-border: #ffaa00;
}

body.theme-delorean {
    --border-color: #00ccff;
    --shadow-color: rgba(0, 204, 255, 0.7);
    --heading-color: #66eeff;
    --heading-shadow-color: rgba(102, 238, 255, 0.7);
    --text-color: #00ccff;
    --highlight-text-color: #ccddff;
    --main-time-color: #33bbff;
    --main-time-shadow-color: rgba(51, 187, 255, 0.7);
    --setting-group-border: #0088bb;
    --setting-group-shadow: rgba(0, 204, 255, 0.3);
    --input-border: #0088bb;
    --slider-thumb-color: #66eeff;
    --slider-thumb-shadow: rgba(102, 238, 255, 0.8);
    --slider-track-color: #003355;
    --slider-track-border: #00ccff;
    --visual-bar-color-1: #66bbff;
    --visual-bar-shadow-1: rgba(102, 187, 255, 0.7);
    --visual-bar-color-2: #9966ff;
    --visual-bar-shadow-2: rgba(153, 102, 255, 0.7);
    --toggle-slider-border: #00ccff;
    --toggle-slider-before: #66eeff;
    --toggle-slider-checked-bg: #00ccff;
    --button-bg: #0088bb;
    --button-color: #00ccff;
    --button-border: #00ccff;
    --button-hover-bg: #00aadd;
    --button-hover-shadow: rgba(0, 204, 255, 0.9);
    --success-banner-bg: #0088bb;
    --success-banner-color: #e0f0ff;
    --success-banner-border: #00ccff;
}
body.theme-doc-brown {
    --bg-color: #332200; /* Dark brown */
    --container-bg-color: #554422; /* Medium brown */
    --border-color: #D4A017; /* Gold */
    --shadow-color: rgba(212, 160, 23, 0.7); /* Gold with transparency */
    --heading-color: #FFD700; /* Gold */
    --heading-shadow-color: rgba(255, 215, 0, 0.7); /* Gold with transparency */
    --text-color: #FFFFE0; /* Light yellow */
    --highlight-text-color: #FFFFFF; /* White */
    --main-time-color: #FFFF00; /* Yellow */
    --main-time-shadow-color: rgba(255, 255, 0, 0.7); /* Yellow with transparency */
    --setting-group-bg: #664400; /* Darker brown */
    --setting-group-border: #B8860B; /* Dark goldenrod */
    --setting-group-shadow: rgba(212, 175, 55, 0.3); /* Dark goldenrod with transparency */
    --input-bg: #443311; /* Very dark brown */
    --input-color: #FFFFFF; /* White */
    --input-border: #B8860B; /* Dark goldenrod */
    --slider-thumb-color: #FFD700; /* Gold */
    --slider-thumb-shadow: rgba(255, 215, 0, 0.8); /* Gold with transparency */
    --slider-track-color: #775500; /* Darker brown */
    --slider-track-border: #D4A017; /* Gold */
    --visual-bar-color-1: #FFD700; /* Gold */
    --visual-bar-shadow-1: rgba(255, 215, 0, 0.7); /* Gold with transparency */
    --visual-bar-color-2: #FFFF00; /* Yellow */
    --visual-bar-shadow-2: rgba(255, 255, 0, 0.7); /* Yellow with transparency */
    --toggle-slider-bg: #554422; /* Medium brown */
    --toggle-slider-border: #B8860B; /* Dark goldenrod */
    --toggle-slider-before: #FFD700; /* Gold */
    --toggle-slider-checked-bg: #D4A017; /* Gold */
    --button-bg: #664400; /* Darker brown */
    --button-color: #FFFFE0; /* Light yellow */
    --button-border: #B8860B; /* Dark goldenrod */
    --button-hover-bg: #775500; /* Slightly lighter brown */
    --button-hover-shadow: rgba(212, 160, 23, 0.9); /* Gold with transparency */
    --reset-button-bg: #8B4513; /* Saddle brown */
    --reset-button-border: #A0522D; /* Sienna */
    --reset-button-color: #FFFAF0; /* Floral white */
    --reset-button-shadow: rgba(139, 69, 19, 0.5); /* Saddle brown with transparency */
    --reset-button-hover-bg: #A0522D; /* Sienna */
    --reset-button-hover-shadow: rgba(160, 82, 45, 0.9); /* Sienna with transparency */
    --message-banner-bg: #554422; /* Medium brown */
    --message-banner-color: #FFFFE0; /* Light yellow */
    --success-banner-bg: #B8860B; /* Dark goldenrod */
    --success-banner-color: #FFFAF0; /* Floral white */
    --success-banner-border: #D4A017; /* Gold */
    --error-banner-bg: #8B4513; /* Saddle brown */
    --error-banner-color: #FFFAF0; /* Floral white */
    --error-banner-border: #A0522D; /* Sienna */
}
body.theme-purple {
    --border-color: #9966ff;
    --shadow-color: rgba(153, 102, 255, 0.7);
}

/* Ensure reset buttons maintain consistent look */
.reset-button {
    background-color: #660000; /* Still red */
    border-color: #ff0000;
    color: #ffcc00;
    text-shadow: 0 0 3px rgba(255, 0, 0, 0.5);
}

.reset-button:hover {
    background-color: #990000;
    box-shadow: 0 0 10px rgba(255, 0, 0, 0.9);
}
.message-banner.error {
    background-color: #880000;
    color: #ffe0e0;
    border: 1px solid #ff0000;
}

/* --- Responsive Design Media Queries --- */

/* For tablets and smaller screens where the viewport is less than our container's max-width.
   This reduces padding to give the content more room. */
@media (max-width: 660px) {
    body {
        padding: 15px; /* Reduce outer padding */
    }

    .container {
        padding: 20px; /* Reduce container internal padding */
    }
}

/* For mobile phones in portrait view.
   This makes the layout more compact and readable on small screens. */
@media (max-width: 480px) {
    body {
        padding: 10px; /* Further reduce padding */
        font-size: 15px; /* Adjust base font for better readability */
    }

    .container {
        padding: 15px; /* Make internal padding more compact */
        border-width: 1px;
    }

    h1 {
        font-size: 1.6em; /* Scale down main heading */
    }

    h2 {
        font-size: 1.3em; /* Scale down sub-headings */
    }
}
)raw";

// SCRIPT_JS - Web page JavaScript logic
const char* SCRIPT_JS = R"raw(
// Global variable to track if any input is invalid (less relevant now with sliders, but keeping for structure)
let anyInputInvalid = false;

document.addEventListener('DOMContentLoaded', (event) => {
    fetchTime();
    fetchSettings();
    setInterval(fetchTime, 1000); // Update time every second

    // Event listeners for ALL range sliders to update their displayed values and visual bars
    const sliders = [
        { id: 'destinationHour', valueSpanId: 'destinationHourValue' },
        { id: 'destinationMinute', valueSpanId: 'destinationMinuteValue' },
        { id: 'snoozeMinutes', valueSpanId: 'snoozeMinutesValue' },
        { id: 'departureHour', valueSpanId: 'departureHourValue' },
        { id: 'departureMinute', valueSpanId: 'departureMinuteValue' },
        { id: 'arrivalHour', valueSpanId: 'arrivalHourValue' },
       
        { id: 'brightness', valueSpanId: 'brightnessValue', hasBar: true },
        { id: 'notificationVolume', valueSpanId: 'volumeValue', hasBar: true },
        { id: 'timeTravelAnimationInterval', valueSpanId: 'timeTravelAnimationIntervalValue' }
    ];

    sliders.forEach(sliderInfo => {
        const sliderElement = document.getElementById(sliderInfo.id);
        const valueSpanElement = document.getElementById(sliderInfo.valueSpanId);
        if (sliderElement && valueSpanElement) {
            sliderElement.addEventListener('input', (e) => {
                valueSpanElement.textContent = e.target.value;
                if (sliderInfo.hasBar) {
                    updateVisualBar(e.target.id, e.target.value, parseInt(e.target.max));
                }
            });
        }
    });

    // New: Event listener for theme select to apply the theme immediately on change
    const themeSelect = document.getElementById('themeSelect');
    if (themeSelect) {
        themeSelect.addEventListener('change', (e) => {
            applyTheme(parseInt(e.target.value));
        });
    }

    // Initial validation (primarily for the timeTravelAnimationInterval if it was a number input,
    // but now sliders handle min/max inherently, so this might become less critical
    // or need adjustment for other non-slider inputs if added later).
    // With all hour/minute inputs now sliders, anyInputInvalid will likely always be false.
    validateAllNumberInputs(); 
});

// Function to update the width of the visual bar based on slider value
function updateVisualBar(sliderId, currentValue, maxValue) {
    let barId = '';
    switch (sliderId) {
        case 'brightness':
            barId = 'brightnessBar';
            break;
        case 'notificationVolume':
            barId = 'volumeBar';
            break;
        default:
            return; // No bar for this slider
    }

    const barElement = document.getElementById(barId);
    if (barElement) {
        const percentage = (currentValue / maxValue) * 100;
        const maxWidthPx = 80; // Max width defined in CSS for visual-bar
        barElement.style.width = `${(percentage / 100) * maxWidthPx}px`;
    }
}

// Function to show transient messages (success/error banners) at the top of the page
function showMessage(message, type = 'success', duration = 3000) {
    const banner = document.getElementById('messageBanner');
    if (!banner) return;

    banner.textContent = message;
    banner.className = 'message-banner ' + type; // Apply type class for styling (success/error)
    banner.style.visibility = 'visible';
    banner.style.opacity = '1';

    // Set a timeout to fade out and hide the banner after 'duration' milliseconds
    setTimeout(() => {
        banner.style.opacity = '0'; // Start fade out
        setTimeout(() => {
            banner.style.visibility = 'hidden'; // Hide completely after fade
            banner.textContent = ''; // Clear text
            banner.className = 'message-banner'; // Reset class
        }, 500); // Wait for opacity transition to finish (0.5s CSS transition)
    }, duration);
}

// Function to display a loading indicator on buttons during asynchronous operations
function showLoading(buttonId, isLoading) {
    const button = document.getElementById(buttonId);
    if (!button) return;

    if (isLoading) {
        button.dataset.originalText = button.textContent; // Store original text before changing
        button.textContent = 'Loading...';
        button.disabled = true; // Disable button to prevent multiple clicks
        button.classList.add('loading'); // Add a class for potential loading styles
    } else {
        button.textContent = button.dataset.originalText || 'Submit'; // Restore original text
        button.disabled = false; // Re-enable button
        button.classList.remove('loading'); // Remove loading class
    }
}

// Frontend validation function for individual number inputs.
// NOTE: This function is now less critical as hour/minute inputs are sliders
// and inherently enforce min/max. It will only validate remaining number inputs (e.g., if you add one back).
function validateInput(inputElement, min, max) {
    const value = parseInt(inputElement.value);
    const validationMessageSpan = document.getElementById(inputElement.id + 'Validation');
    
    if (isNaN(value) || value < min || value > max) {
        inputElement.classList.add('invalid');
        if (validationMessageSpan) validationMessageSpan.textContent = `(${min}-${max})`;
    } else {
        inputElement.classList.remove('invalid');
        if (validationMessageSpan) validationMessageSpan.textContent = '';
    }
    validateAllNumberInputs(); // Re-validate all to update save button state
}

// Function to validate all "number" inputs (now mostly sliders) and update the save button state.
// Sliders inherently enforce min/max, so this will mostly ensure the button is enabled.
function validateAllNumberInputs() {
    anyInputInvalid = false;
    // Query for any remaining actual 'number' inputs, if any.
    // As of this update, timeTravelAnimationInterval is now a range input, so this might be empty.
    const numberInputs = document.querySelectorAll('input[type="number"]'); 

    numberInputs.forEach(input => {
        const min = parseInt(input.min);
        const max = parseInt(input.max);
        const value = parseInt(input.value);

        if (isNaN(value) || value < min || value > max) {
            input.classList.add('invalid');
            const validationMessageSpan = document.getElementById(input.id + 'Validation');
            if (validationMessageSpan) {
                validationMessageSpan.textContent = `(${min}-${max})`;
            }
            anyInputInvalid = true;
        } else {
            input.classList.remove('invalid');
            const validationMessageSpan = document.getElementById(input.id + 'Validation');
            if (validationMessageSpan) {
                validationMessageSpan.textContent = '';
            }
        }
    });
    // The save button will be enabled unless 'anyInputInvalid' is explicitly true from other sources.
    // With all number inputs converted to range sliders, this will likely always be false.
    document.getElementById('saveSettingsBtn').disabled = anyInputInvalid;
}

// Function to apply the selected theme by adding a CSS class to the <body> element.
function applyTheme(themeIndex) {
    const body = document.body;
    // Remove all possible theme classes to ensure a clean slate
    body.classList.remove('theme-biff-tannen', 'theme-1955', 'theme-delorean');

    switch(parseInt(themeIndex)) { // Use parseInt for safety
        // case 0 is the default (Green) and requires no class.
        case 1: body.classList.add('theme-biff-tannen'); break;
        case 2: body.classList.add('theme-1955'); break;
        case 3: body.classList.add('theme-delorean'); break;
    }
}

// Fetches current time, date, and NTP synchronization status from the ESP32's /api/time endpoint.
function fetchTime() {
    fetch('/api/time')
        .then(response => response.json())
        .then(data => {
            document.getElementById('currentTime').textContent = data.currentTime;
            document.getElementById('currentDate').textContent = data.currentDate;
            document.getElementById('timeSyncStatus').textContent = data.timeSynchronized ? 'Yes' : 'No';
            document.getElementById('lastSyncTime').textContent = data.lastSyncTime;
            document.getElementById('syncStatusMessage').textContent = data.syncStatusMessage;
            document.getElementById('lastNtpServer').textContent = data.lastNtpServer;
        })
        .catch(error => {
            console.error('Error fetching time:', error);
            document.getElementById('currentTime').textContent = '--:--:--';
            document.getElementById('currentDate').textContent = '--/--/----';
            document.getElementById('timeSyncStatus').textContent = 'Error';
            document.getElementById('lastSyncTime').textContent = 'Error';
            document.getElementById('syncStatusMessage').textContent = 'Error fetching status';
            document.getElementById('lastNtpServer').textContent = 'Error';
            showMessage('Error fetching time status!', 'error', 5000);
        });
}

// Fetches all current settings from the ESP32's /api/settings endpoint.
// Populates the form fields with the loaded values, including new sliders.
function fetchSettings() {
    fetch('/api/settings')
        .then(response => response.json())
        .then(data => {
            // Define all slider IDs and their corresponding value span IDs for easier iteration
            const sliderUpdateMap = [
                { id: 'destinationHour', valueSpanId: 'destinationHourValue', prop: 'destinationHour' },
                { id: 'destinationMinute', valueSpanId: 'destinationMinuteValue', prop: 'destinationMinute' },
                { id: 'snoozeMinutes', valueSpanId: 'snoozeMinutesValue', prop: 'snoozeMinutes' },
                { id: 'departureHour', valueSpanId: 'departureHourValue', prop: 'departureHour' },
                { id: 'departureMinute', valueSpanId: 'departureMinuteValue', prop: 'departureMinute' },
               { id: 'arrivalHour', valueSpanId: 'arrivalHourValue', prop: 'arrivalHour' },
                { id: 'arrivalMinute', valueSpanId: 'arrivalMinuteValue', prop: 'arrivalMinute' },
                { id: 'brightness', valueSpanId: 'brightnessValue', prop: 'brightness', hasBar: true },
                { id: 'notificationVolume', valueSpanId: 'volumeValue', prop: 'notificationVolume', hasBar: true },
                { id: 'timeTravelAnimationInterval', valueSpanId: 'timeTravelAnimationIntervalValue', prop: 'timeTravelAnimationInterval' }
            ];

            // Loop through map to update sliders and their values
            sliderUpdateMap.forEach(item => {
                const slider = document.getElementById(item.id);
                const valueSpan = document.getElementById(item.valueSpanId);
                if (slider && valueSpan && data.hasOwnProperty(item.prop)) {
                    slider.value = data[item.prop];
                    valueSpan.textContent = data[item.prop];
                    if (item.hasBar) {
                        updateVisualBar(item.id, data[item.prop], parseInt(slider.max));
                    }
                }
            });
            
            // Populate checkbox (toggle) fields
            document.getElementById('alarmOnOff').checked = data.alarmOnOff;
            document.getElementById('timeTravelSoundToggle').checked = data.timeTravelSoundToggle;
            document.getElementById('powerOfLoveToggle').checked = data.powerOfLoveToggle;
            document.getElementById('displayFormat24h').checked = data.displayFormat24h;

            // Set the selected theme in the dropdown and apply it to the body
            const themeSelect = document.getElementById('themeSelect');
            if (themeSelect) {
                themeSelect.value = data.theme;
                applyTheme(data.theme);
            }
      // Set the selected theme in the dropdown
            const timezoneSelect = document.getElementById('timezoneSelect');
            if (timezoneSelect) {
            // Check if data.timezoneString is undefined or null, and if so, set it to the default timezone string
                if (!data.timezoneString) {
                  data.timezoneString = "EST5EDT,M3.2.0,M11.1.0"; // Set to default timezone
               }

                timezoneSelect.value = data.timezoneString;
           }
              // Re-validate all inputs after populating, though for sliders this mainly ensures button is enabled.
            validateAllNumberInputs(); 
        })
        .catch(error => {
            console.error('Error fetching settings:', error);
            showMessage('Error loading settings!', 'error', 5000);
        });
}

// Saves all current settings from the web form to the ESP32 via the /api/saveSettings endpoint.
function saveSettings() {
    if (anyInputInvalid) { // This check remains, but with sliders it's less likely to be true
        showMessage('Please fix invalid input values before saving!', 'error', 5000);
        return;
    }

    showLoading('saveSettingsBtn', true);

    const settings = {
        destinationHour: document.getElementById('destinationHour').value,
        destinationMinute: document.getElementById('destinationMinute').value,
        alarmOnOff: document.getElementById('alarmOnOff').checked,
        snoozeMinutes: document.getElementById('snoozeMinutes').value,
        departureHour: document.getElementById('departureHour').value,
        departureMinute: document.getElementById('departureMinute').value,
        arrivalHour: document.getElementById('arrivalHour').value,
        arrivalMinute: document.getElementById('arrivalMinute').value,
        notificationVolume: document.getElementById('notificationVolume').value,
        timeTravelSoundToggle: document.getElementById('timeTravelSoundToggle').checked,
        powerOfLoveToggle: document.getElementById('powerOfLoveToggle').checked,
        timeTravelAnimationInterval: document.getElementById('timeTravelAnimationInterval').value,
        displayFormat24h: document.getElementById('displayFormat24h').checked,
        theme: document.getElementById('themeSelect').value,
        timezoneString: document.getElementById('timezoneSelect').value
        
   };

    fetch('/api/saveSettings', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: new URLSearchParams(settings).toString()
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.text();
    })
    .then(data => {
        console.log(data);
        showMessage('Settings saved successfully!', 'success');
        
        // --- Highlight Saved Fields ---
        for (const key in settings) {
            const element = document.getElementById(key);
            if (element) {
                element.classList.remove('highlight-saved'); 
                void element.offsetWidth;
                element.classList.add('highlight-saved'); 
            }
        }
        fetchSettings();
    })
    .catch(error => {
        console.error('Error saving settings:', error);
        showMessage('Error saving settings!', 'error', 5000);
    })
    .finally(() => {
        showLoading('saveSettingsBtn', false);
    });
}

// Fetches general status (WiFi connection, IP, RSSI) from /api/status.
function fetchStatus() {
    showLoading('fetchStatusBtn', true);
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('status').innerHTML = 
                `<p>WiFi Status: ${data.wifiStatus}</p>` +
                `<p>IP Address: ${data.ipAddress}</p>` +
                `<p>RSSI: ${data.rssi} dBm</p>`;
            showMessage('Status updated!', 'success');
        })
        .catch(error => {
            console.error('Error fetching status:', error);
            document.getElementById('status').innerHTML = '<p>Error fetching status.</p>';
            showMessage('Error fetching status!', 'error', 5000);
        })
        .finally(() => {
            showLoading('fetchStatusBtn', false);
        });
}

// Resets all alarm clock settings to their default values.
function resetToDefaults() {
    if (!confirm('Are you sure you want to reset all settings to default values? This cannot be undone.')) {
        return;
    }
    showLoading('resetDefaultsBtn', true);
    fetch('/api/resetDefaults', { method: 'POST' })
        .then(response => {
            if (!response.ok) throw new Error('Network response was not ok');
            return response.text();
        })
        .then(data => {
            console.log(data);
            showMessage('Settings reset. Device rebooting in a few seconds...', 'success', 10000);
        })
        .catch(error => {
            console.error('Error resetting defaults:', error);
            showMessage('Error resetting defaults!', 'error', 5000);
        });
}

// Clears saved WiFi credentials.
function resetWifi() {
    if (!confirm('Are! you sure you want to clear WiFi credentials? You will need to reconnect the device via its AP mode.')) {
        return;
    }
    showLoading('resetWifiBtn', true);
    fetch('/api/resetWifi', { method: 'POST' })
        .then(response => {
            if (!response.ok) throw new Error('Network response was not ok');
            return response.text();
        })
        .then(data => {
            console.log(data);
            showMessage('WiFi credentials cleared. Device rebooting to AP mode in a few seconds...', 'success', 10000);
        })
        .catch(error => {
            console.error('Error resetting WiFi:', error);
            showMessage('Error resetting WiFi credentials!', 'error', 5000);
        });
}
// SSE connection for server-pushed browser refresh
const evtSource = new EventSource('/events');
evtSource.addEventListener('command', (e) => {
  if (e.data === 'refresh') {
    console.log("Server requested page refresh");
    location.reload();
  }
});
)raw";


// --- Custom NTP Client Functions ---
// These functions implement a basic NTP client using UDP.
// The standard `sntp_set_time_sync_notification_cb` and `configTime` functions
// are often sufficient, but a custom client provides more control over server selection,
// retry logic, and direct UDP interaction for specific needs.

/**
 * @brief Sends an NTP request packet to the specified NTP server IP address.
 * This function constructs a standard NTP client request packet (48 bytes) and sends it via UDP.
 * * @param ntpServerIp The IP address of the NTP server.
 * @param serverHostname The hostname string of the NTP server being used (for logging/UI).
 */
void sendNTPpacket(IPAddress& ntpServerIp, const char* serverHostname) {
  // Clear the packet buffer by setting all bytes to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Set the first byte of the NTP packet header:
  // - LI (Leap Indicator): 0 (No leap second adjustment)
  // - VN (Version Number): 3 (NTPv3, commonly supported)
  // - Mode: 3 (Client mode, indicating a request for time)
  packetBuffer[0] = 0b11100011; 
  
  // All other fields (e.g., Stratum, Poll Interval, Precision, etc.) are optional for a basic
  // client request and can be left as 0s. The NTP server will fill in relevant timestamps
  // in its response.

  Serial.print("Sending NTP request to: ");
  Serial.print(ntpServerIp);
  Serial.print(" (");
  Serial.print(serverHostname);
  Serial.println(")");
  
  // Begin sending a UDP packet to the NTP server on standard NTP port 123.
  Udp.beginPacket(ntpServerIp, 123); 
  Udp.write(packetBuffer, NTP_PACKET_SIZE); // Write the 48-byte NTP request packet
  Udp.endPacket(); // Send the packet
  
  lastNtpRequestSent = millis(); // Record the time the request was sent for timeout/retry logic
  syncStatusMessage = "Request sent to " + String(serverHostname) + ". Waiting for response..."; // Update status for web UI
  currentNtpServerUsed = serverHostname; // Store the hostname for web UI
}

/**
 * @brief Attempts to get the current time from the NTP server.
 * This function handles DNS resolution, sends the NTP request, and waits for a response.
 * It parses the response to extract the Unix timestamp.
 * * @return time_t (Unix timestamp) if successful, 0 on failure (e.g., DNS lookup failure, no response).
 */
time_t getNTPtime() {
    IPAddress ntpServerIP;
    
    // Get the hostname of the current NTP server from the predefined array
    const char* serverToUse = NTP_SERVERS[currentNtpServerIndex];

    Serial.print("Resolving NTP server hostname: ");
    Serial.println(serverToUse);
    
    // Perform DNS lookup to resolve the NTP server hostname to an IP address.
    // WiFi.hostByName() returns 1 on success, 0 on failure.
    int dnsResolved = WiFi.hostByName(serverToUse, ntpServerIP);
    
    if (dnsResolved == 0) {
        // DNS lookup failed
        Serial.printf("DNS lookup failed for NTP server '%s'!\n", serverToUse);
        syncStatusMessage = "DNS failed for " + String(serverToUse) + ".";
        return 0; // Return 0 to indicate failure
    }

    sendNTPpacket(ntpServerIP, serverToUse); // Send the NTP request packet using the resolved IP and hostname

    // Wait for a UDP response packet for up to 1.5 seconds.
    unsigned long startWaiting = millis();
    while (millis() - startWaiting < 1500) { 
        int packetSize = Udp.parsePacket(); // Check if a UDP packet has arrived and return its size
        if (packetSize) {
            // A packet was received
            Serial.print("Received ");
            Serial.print(packetSize);
            Serial.print(" bytes from NTP server (");
            Serial.print(serverToUse);
            Serial.println(").");
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // Read the contents of the packet into the buffer

            // The NTP timestamp is a 64-bit unsigned fixed-point number.
            // The relevant part for a client (Transmit Timestamp) starts at byte 40 of the received packet.
            // We need to extract the 32-bit integer part (seconds).
            // It's structured as two 16-bit words: high word (bytes 40, 41) and low word (bytes 42, 43).
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            
            // Combine the two words into a single 32-bit unsigned long.
            // This 'secsSince1900' represents seconds since Jan 1, 1900 UTC (NTP epoch).
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            Serial.print("Seconds since Jan 1 1900 = ");
            Serial.println(secsSince1900);

            // Convert NTP time (seconds since Jan 1, 1900) to Unix time (seconds since Jan 1, 1970).
            // There are 2208988800 seconds between Jan 1, 1900 and Jan 1, 1970.
            const unsigned long seventyYears = 2208988800UL;
            unsigned long epoch = secsSince1900 - seventyYears;
            
            return epoch; // Return the calculated Unix timestamp
        }
        delay(10); // Small delay to yield CPU to other tasks while waiting for UDP packet
    }
    Serial.printf("No NTP response received from '%s' within timeout.\n", serverToUse);
    syncStatusMessage = "No response from " + String(serverToUse) + "."; // Update status for web UI
    return 0; // No response received within the timeout, return 0 for failure
}

/**
 * @brief Manages the NTP synchronization process.
 * This function calls `getNTPtime()` and, if successful, sets the system time.
 * On failure, it implements exponential backoff and cycles through the list of NTP servers
 * to improve reliability. It also updates status messages for the web UI.
 */
void processNTPresponse() {
    time_t epochTime = getNTPtime(); // Attempt to get time from the current NTP server
    if (epochTime > 0) {
        // If a valid Unix epoch time is received:
        // 1. Set the system's real-time clock. `settimeofday` expects a `timeval` struct (seconds and microseconds).
        struct timeval tv;
        tv.tv_sec = epochTime;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL); 
        
        // 2. Apply the timezone information. This is crucial for `getLocalTime()` and `strftime()`
        // to correctly convert UTC time to your local time (including Daylight Saving Time).
        setenv("TZ", TZ_INFO, 1); // Set the TZ environment variable. '1' means overwrite if it exists.
        tzset();                  // Update time conversion information based on TZ environment variabletimeSynchronizedudp


        timeSynchronized = true;    // Set flag to true indicating successful sync
        lastNtpSyncTime = epochTime; // Store the Unix timestamp of this successful sync
        syncStatusMessage = "Synchronized"; // Update detailed status message for UI
        Serial.println("NTP synchronization successful with custom client!");
        
        // Reset backoff parameters on successful sync to go back to base interval for next sync
        ntpFailedAttempts = 0;
        currentNtpInterval = NTP_BASE_INTERVAL_MS;
        Serial.printf("NTP sync successful. Next sync in %lu seconds.\n", currentNtpInterval / 1000);

    } else {
        // If getting time failed:
        timeSynchronized = false; // Mark time as not synchronized
        Serial.println("NTP synchronization failed with custom client.");
        
        // Increment failed attempts counter and cycle to the next NTP server in the list.
        ntpFailedAttempts++;
        currentNtpServerIndex = (currentNtpServerIndex + 1) % NUM_NTP_SERVERS; // Modulo operator ensures cycling back to 0

        // Implement exponential backoff for the next retry interval:
        // Interval doubles with each failed attempt, up to a maximum.
        // currentNtpInterval = BASE_INTERVAL * 2^failedAttempts, capped at MAX_INTERVAL.
        unsigned long nextAttemptFactor = (1UL << ntpFailedAttempts); // Calculate 2 to the power of ntpFailedAttempts
                                                                    // Using 1UL ensures unsigned long math to prevent overflow with large shifts.
        // Prevent potential overflow if ntpFailedAttempts gets too high (e.g., beyond 30 for unsigned long)
        if (nextAttemptFactor == 0 || ntpFailedAttempts > 30) { 
            nextAttemptFactor = NTP_MAX_INTERVAL_MS / NTP_BASE_INTERVAL_MS; // Cap the factor to prevent excessive values
        }

        currentNtpInterval = NTP_BASE_INTERVAL_MS * nextAttemptFactor;
        // Ensure the interval doesn't exceed the maximum allowed.
        if (currentNtpInterval > NTP_MAX_INTERVAL_MS || currentNtpInterval < NTP_BASE_INTERVAL_MS) { 
            currentNtpInterval = NTP_MAX_INTERVAL_MS;
        }
        
        Serial.printf("NTP sync failed (attempt %u). Trying next server: %s. Next retry in %lu seconds.\n", 
                      ntpFailedAttempts, NTP_SERVERS[currentNtpServerIndex], currentNtpInterval / 1000);
        // Append retry information to the status message for the web UI.
        syncStatusMessage += " Retrying with " + String(NTP_SERVERS[currentNtpServerIndex]);
    }
}

// --- WiFiManager Callback ---
/**
 * @brief This function is called by WiFiManager when it enters configuration mode.
 * This typically happens if the ESP32 cannot connect to a previously saved WiFi network
 * or if WiFi credentials have been explicitly reset.
 * It provides debugging information about the AP created by WiFiManager.
 * * @param myWiFiManager Pointer to the WiFiManager instance.
 */
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFiManager config mode");
  Serial.print("Config Portal AP IP: ");
  Serial.println(WiFi.softAPIP()); // Print the IP address of the Access Point created by WiFiManager
  Serial.print("Config Portal SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID()); // Print the SSID of the config portal
}

// --- Settings Management Functions ---

/**
 * @brief Loads application settings from NVS (Non-Volatile Storage) using the Preferences library.
 * If no settings are found in NVS (e.g., first boot) or if the stored data size
 * does not match the current `ClockSettings` struct size (indicating a schema change or corruption),
 * default settings are loaded and then immediately saved to NVS.
 */
void loadSettings() {
  // Attempt to read the entire ClockSettings struct from NVS under the key "settings".
  // `getBytes` returns the number of bytes read.
  size_t bytesRead = preferences.getBytes("settings", &currentSettings, sizeof(currentSettings));
  
  if (bytesRead != sizeof(currentSettings)) {
    // This condition means either:
    // 1. NVS is empty (first boot).
    // 2. The size of the `ClockSettings` struct has changed between firmware versions,
    //    making existing stored data incompatible.
    // 3. The NVS data for "settings" is corrupted or incomplete.
    Serial.println("No settings found in NVS or partial read. Resetting to defaults.");
    resetDefaultSettings(); // This function will copy `defaultSettings` to `currentSettings` and save them.
  } else {
    Serial.println("Settings loaded successfully from NVS. Validating...");
    // Perform basic validation of loaded settings to ensure they are within expected/safe ranges.
    // This is crucial to prevent potential crashes or erratic behavior if corrupted or out-of-range
    // data was loaded from NVS. If a value is out of bounds, it's reset to its default.
    if (currentSettings.destinationHour < 0 || currentSettings.destinationHour > 23) currentSettings.destinationHour = defaultSettings.destinationHour;
    if (currentSettings.destinationMinute < 0 || currentSettings.destinationMinute > 59) currentSettings.destinationMinute = defaultSettings.destinationMinute;
    currentSettings.alarmOnOff = (currentSettings.alarmOnOff != 0); // Ensure boolean is strictly true/false
    if (currentSettings.snoozeMinutes < 1 || currentSettings.snoozeMinutes > 59) currentSettings.snoozeMinutes = defaultSettings.snoozeMinutes;
    if (currentSettings.departureHour < 0 || currentSettings.departureHour > 23) currentSettings.departureHour = defaultSettings.departureHour;
    if (currentSettings.departureMinute < 0 || currentSettings.departureMinute > 59) currentSettings.departureMinute = defaultSettings.departureMinute;
    if (currentSettings.arrivalHour < 0 || currentSettings.arrivalHour > 23) currentSettings.arrivalHour = defaultSettings.arrivalHour;
    if (currentSettings.arrivalMinute < 0 || currentSettings.arrivalMinute > 59) currentSettings.arrivalMinute = defaultSettings.arrivalMinute;
    if (currentSettings.brightness < 0 || currentSettings.brightness > 7) currentSettings.brightness = defaultSettings.brightness;
   if (currentSettings.notificationVolume < 0 || currentSettings.notificationVolume > 30) currentSettings.notificationVolume = defaultSettings.notificationVolume;
    currentSettings.timeTravelSoundToggle = (currentSettings.timeTravelSoundToggle != 0); // Ensure boolean is strictly true/false (Renamed)
    currentSettings.powerOfLoveToggle = (currentSettings.powerOfLoveToggle != 0); // New: Ensure boolean is strictly true/false
    if (currentSettings.timeTravelAnimationInterval < 0 || currentSettings.timeTravelAnimationInterval > 60) currentSettings.timeTravelAnimationInterval = defaultSettings.timeTravelAnimationInterval; // New: Validate interval
    currentSettings.displayFormat24h = (currentSettings.displayFormat24h != 0); // Ensure boolean is strictly true/false
            // No validation needed for timezoneString as it's a string, but ensure it's not too long to avoid buffer overflows
    if (currentSettings.theme < 0 || currentSettings.theme > 3) currentSettings.theme = defaultSettings.theme; // Assuming 4 themes (0-3)
  }
  // Print the active settings to Serial for debugging purposes, regardless of whether they were loaded or defaulted.
    Serial.println("Active Settings:");
  Serial.printf("Destination: %02d:%02d, Alarm On: %d, Snooze: %d min\n",
    currentSettings.destinationHour, currentSettings.destinationMinute,
    currentSettings.alarmOnOff, currentSettings.snoozeMinutes);
  Serial.printf("Departure: %02d:%02d, Arrival: %02d:%02d\n",
    currentSettings.departureHour, currentSettings.departureMinute,
    currentSettings.arrivalHour, currentSettings.arrivalMinute);
  Serial.println("---------------------------------\n");
Serial.printf("Brightness: %d, Volume: %d, Time Travel Sound: %d, Power of Love: %d, Animation Interval: %d min, 24h Format: %d, Theme: %d\n",
     currentSettings.brightness,
    currentSettings.notificationVolume, currentSettings.timeTravelSoundToggle,
    currentSettings.powerOfLoveToggle, currentSettings.timeTravelAnimationInterval,
    currentSettings.displayFormat24h, currentSettings.theme);


 Serial.print("Timezone: ");
}
/**
 * @brief Saves the current `currentSettings` structure to NVS.
 * This should be called whenever settings are modified (e.g., after receiving updates from the web UI).
 */
void saveSettings() {
  preferences.putBytes("settings", &currentSettings, sizeof(currentSettings));
  Serial.println("Settings saved.");
}

/**
 * @brief Resets current settings to their predefined default values and saves them to NVS.
 * This is typically triggered by a "Factory Reset" command from the web UI.
 */
void resetDefaultSettings() {
  currentSettings = defaultSettings; // Copy all values from `defaultSettings` to `currentSettings`
 strncpy(currentSettings.timezoneString, defaultSettings.timezoneString, sizeof(currentSettings.timezoneString) - 1); // Copy the string
  saveSettings(); // Immediately save these new default settings to NVS
  Serial.println("Settings reset to defaults and saved.");
}

// --- API Handler Functions for Web Server ---
char timezone[64];
/**
 * @brief Handles GET requests to the `/api/time` endpoint.
 * Returns the current time, date, NTP synchronization status, last sync time, and sync message as a JSON object.
 * This is used by the web UI to display real-time clock information.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiTime(AsyncWebServerRequest *request) {
    StaticJsonDocument<350> doc; // ArduinoJson document to efficiently build the JSON response
    
    struct tm timeinfo; // Structure to hold broken-down time (year, month, day, hour, etc.)
    
    // Attempt to get the local time. `getLocalTime()` converts the system's UTC time
    // (set by NTP) into local time based on the `TZ_INFO` environment variable.
    // It returns true on success, false if time isn't synchronized or if conversion fails.
    if (!timeSynchronized || !getLocalTime(&timeinfo)){ 
        // If time is not synchronized or `getLocalTime()` fails, provide placeholder values.
        doc["currentTime"] = "--:--:--";
        doc["currentDate"] = "--/--/----";
        doc["timeSynchronized"] = false;
        doc["lastSyncTime"] = "N/A"; 
        doc["syncStatusMessage"] = syncStatusMessage; // Report detailed custom sync status
        doc["lastNtpServer"] = currentNtpServerUsed;  // Show which NTP server was last used/attempted
    } else {
        char timeBuffer[12]; // Buffer for time string (e.g., "HH:MM:SS" or "HH:MM:SS AM")
        char dateBuffer[11]; // Buffer for date string (e.g., "DD/MM/YYYY")

        // Format time based on user's preference (24-hour or 12-hour AM/PM)
        if (currentSettings.displayFormat24h) {
            strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo); // 24-hour format (e.g., 14:35:01)
        } else {
            strftime(timeBuffer, sizeof(timeBuffer), "%I:%M:%S %p", &timeinfo); // 12-hour format with AM/PM (e.g., 02:35:01 PM)
        }
        strftime(dateBuffer, sizeof(dateBuffer), "%m/%d/%Y", &timeinfo); // Month/Day/Year (e.g., 07/04/2025)

        doc["currentTime"] = timeBuffer;
        doc["currentDate"] = dateBuffer;
        doc["timeSynchronized"] = timeSynchronized;

        // Format and include the last successful NTP synchronization time (time part only).
        if (lastNtpSyncTime > 0) {
            struct tm *lastSyncInfo = localtime(&lastNtpSyncTime); // Convert Unix timestamp to local time struct
            char lastSyncBuffer[12]; // Buffer for time string only
            if (currentSettings.displayFormat24h) {
                strftime(lastSyncBuffer, sizeof(lastSyncBuffer), "%H:%M:%S", lastSyncInfo);
            } else {
                strftime(lastSyncBuffer, sizeof(lastSyncBuffer), "%I:%M:%S %p", lastSyncInfo);
            }
            doc["lastSyncTime"] = lastSyncBuffer;
        } else {
            doc["lastSyncTime"] = "Never"; // If no sync has occurred yet
        }
        doc["syncStatusMessage"] = syncStatusMessage; // Report current sync status
        doc["lastNtpServer"] = currentNtpServerUsed;  // Add this field for UI
    }

    String jsonString;
    serializeJson(doc, jsonString); // Convert the JSON document into a String for transmission
    request->send(200, "application/json", jsonString); // Send the JSON string as the HTTP response
}

/**
 * @brief Handles GET requests to the `/api/settings` endpoint.
 * Returns all current `ClockSettings` as a JSON object.
 * This is used by the web UI to populate the settings form when the page loads.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiSettings(AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc; // ArduinoJson document to hold settings

    // Populate the JSON document with values from the `currentSettings` struct.
    doc["destinationHour"] = currentSettings.destinationHour;
    doc["destinationMinute"] = currentSettings.destinationMinute;
    doc["alarmOnOff"] = currentSettings.alarmOnOff;
    doc["snoozeMinutes"] = currentSettings.snoozeMinutes;
    doc["departureHour"] = currentSettings.departureHour;
    doc["departureMinute"] = currentSettings.departureMinute;
    doc["arrivalHour"] = currentSettings.arrivalHour;
    doc["arrivalMinute"] = currentSettings.arrivalMinute;
    doc["brightness"] = currentSettings.brightness;
    doc["notificationVolume"] = currentSettings.notificationVolume;
    doc["timeTravelSoundToggle"] = currentSettings.timeTravelSoundToggle; // Renamed
    doc["powerOfLoveToggle"] = currentSettings.powerOfLoveToggle; // New
    doc["timeTravelAnimationInterval"] = currentSettings.timeTravelAnimationInterval; // New
    doc["displayFormat24h"] = currentSettings.displayFormat24h;
    doc["timezoneString"] = currentSettings.timezoneString;
    doc["theme"] = currentSettings.theme; // Existing

    String jsonString;
    serializeJson(doc, jsonString); // Convert JSON document to string
    request->send(200, "application/json", jsonString); // Send the JSON response
}

/**
 * @brief Handles POST requests to the `/api/saveSettings` endpoint.
 * Receives new settings from the web form, updates the `currentSettings` struct,
 * prints changes to Serial Monitor, and saves the updated settings to NVS.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiSaveSettings(AsyncWebServerRequest *request) {
    // 1. Create a temporary copy of `currentSettings` BEFORE applying changes.
    // This allows us to compare and report which settings have actually changed.
    ClockSettings oldSettings = currentSettings; 

    // Retrieve each setting from the POST request parameters and update `currentSettings`.
    // The `getParam(name, true)` method retrieves POST parameters.
    // `.value().toInt()` converts string values to integers.
    // For boolean values (checkboxes), `.value() == "true"` correctly converts the string "true" to boolean true.
    if (request->hasParam("destinationHour", true)) currentSettings.destinationHour = request->getParam("destinationHour", true)->value().toInt();
    if (request->hasParam("destinationMinute", true)) currentSettings.destinationMinute = request->getParam("destinationMinute", true)->value().toInt();
    if (request->hasParam("alarmOnOff", true)) currentSettings.alarmOnOff = (request->getParam("alarmOnOff", true)->value() == "true");
    if (request->hasParam("snoozeMinutes", true)) currentSettings.snoozeMinutes = request->getParam("snoozeMinutes", true)->value().toInt();
    if (request->hasParam("departureHour", true)) currentSettings.departureHour = request->getParam("departureHour", true)->value().toInt();
    if (request->hasParam("departureMinute", true)) currentSettings.departureMinute = request->getParam("departureMinute", true)->value().toInt();
    if (request->hasParam("arrivalHour", true)) currentSettings.arrivalHour = request->getParam("arrivalHour", true)->value().toInt();
    if (request->hasParam("arrivalMinute", true)) currentSettings.arrivalMinute = request->getParam("arrivalMinute", true)->value().toInt();
    if (request->hasParam("brightness", true)) currentSettings.brightness = request->getParam("brightness", true)->value().toInt();
    if (request->hasParam("notificationVolume", true)) currentSettings.notificationVolume = request->getParam("notificationVolume", true)->value().toInt();
    if (request->hasParam("timeTravelSoundToggle", true)) currentSettings.timeTravelSoundToggle = (request->getParam("timeTravelSoundToggle", true)->value() == "true"); // Renamed
    if (request->hasParam("powerOfLoveToggle", true)) currentSettings.powerOfLoveToggle = (request->getParam("powerOfLoveToggle", true)->value() == "true"); // New
    if (request->hasParam("timeTravelAnimationInterval", true)) currentSettings.timeTravelAnimationInterval = request->getParam("timeTravelAnimationInterval", true)->value().toInt(); // New
    if (request->hasParam("displayFormat24h", true)) currentSettings.displayFormat24h = (request->getParam("displayFormat24h", true)->value() == "true");
    if (request->hasParam("timezoneString", true)) strncpy(currentSettings.timezoneString, request->getParam("timezoneString", true)->value().c_str(), sizeof(currentSettings.timezoneString) - 1);
    if (request->hasParam("theme", true)) currentSettings.theme = request->getParam("theme", true)->value().toInt(); // Existing
    nighttimeonlyonce = true;
    // 2. Compare `oldSettings` with `currentSettings` and print any changed values to Serial Monitor.
    Serial.println("\n--- Settings Changes Detected ---");
    

    // Macro for cleaner comparison and printing of integer settings that have changed.
    // #settingName converts the variable name to a string.
    // (long) cast is used for printf's %d to avoid potential warnings with int types, especially on different architectures.
    #define PRINT_IF_CHANGED(settingName, format) \
        if (currentSettings.settingName != oldSettings.settingName) { \
            Serial.printf("  " #settingName ": " format " -> " format "\n", \
                    (long)oldSettings.settingName, (long)currentSettings.settingName); \
            changesMade = true; \
        }
     
    // Macro for cleaner comparison and printing of boolean settings that have changed.
    // Uses ternary operator to print "On" or "Off".
    #define PRINT_BOOL_IF_CHANGED(settingName) \
        if (currentSettings.settingName != oldSettings.settingName) { \
            Serial.printf("  " #settingName ": %s -> %s\n", \
                         (oldSettings.settingName ? "On" : "Off"), \
                         (currentSettings.settingName ? "On" : "Off")); \
           changesMade = true; \
         }
    // Macro for cleaner comparison and printing of string settings that have changed.
    #define PRINT_STRING_IF_CHANGED(settingName) \
        if (strcmp(currentSettings.settingName, oldSettings.settingName) != 0) { \
            Serial.printf("  " #settingName ": %s -> %s\n", oldSettings.settingName, currentSettings.settingName); \
          changesMade = true; \
        }
    // Apply the macros to check and print each setting:
    // Time-related settings
    PRINT_IF_CHANGED(destinationHour, "%d");
    PRINT_IF_CHANGED(destinationMinute, "%d");
    PRINT_BOOL_IF_CHANGED(alarmOnOff);
    PRINT_IF_CHANGED(snoozeMinutes, "%d");
    PRINT_IF_CHANGED(departureHour, "%d");
    PRINT_IF_CHANGED(departureMinute, "%d");
    PRINT_IF_CHANGED(arrivalHour, "%d");
    PRINT_IF_CHANGED(arrivalMinute, "%d");

    // Display & Sound settings
   
    
    PRINT_IF_CHANGED(notificationVolume, "%d");
    PRINT_BOOL_IF_CHANGED(timeTravelSoundToggle); // Renamed
    PRINT_BOOL_IF_CHANGED(powerOfLoveToggle); // New
    PRINT_IF_CHANGED(timeTravelAnimationInterval, "%d"); // New
    PRINT_BOOL_IF_CHANGED(displayFormat24h);
    PRINT_STRING_IF_CHANGED(timezoneString);
    PRINT_IF_CHANGED(theme, "%d"); // Existing
       if (strcmp(currentSettings.timezoneString, oldSettings.timezoneString) != 0) {
              Serial.print("New Timezone String: ");
          Serial.println(currentSettings.timezoneString);
          changesMade = true;
        }
        if (currentSettings.theme != oldSettings.theme) {
          changesMade = true;
        }

      if(currentSettings.timezoneString != oldSettings.timezoneString) {
             nighttimeonlyonce = true;
             processNTPresponse();
             Serial.println("reset nighttimeonlyonce");
              changesMade = true;
        }
    if (!changesMade) {
        Serial.println("  No observable changes in settings.");
    }
    Serial.println("---------------------------------\n");

    saveSettings(); // Save the updated `currentSettings` struct to NVS
    request->send(200, "text/plain", "Settings saved!"); // Send a simple success response to the client
}

/**
 *  @brief Handles GET requests to the `/api/status` endpoint
 * Returns general WiFi and network status information (connected status, IP, RSSI) as JSON.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiStatus(AsyncWebServerRequest *request) {
    StaticJsonDocument<128> doc; // Small JSON document for status info
    doc["wifiStatus"] = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
    doc["ipAddress"] = WiFi.localIP().toString(); // Convert IPAddress object to string
    doc["rssi"] = WiFi.RSSI(); // Get Received Signal Strength Indicator in dBm

    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, "application/json", jsonString);
}

/**
 * @brief Handles POST requests to the `/api/resetDefaults` endpoint.
 * Triggers a full reset of application settings to their factory defaults.
 * This also initiates a device reboot to ensure a clean state with new settings.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiResetDefaults(AsyncWebServerRequest *request) {
    Serial.println("Received request to reset settings to defaults.");
    resetDefaultSettings(); // Call the function to reset settings and save them to NVS
    request->send(200, "text/plain", "Settings reset to defaults. Device rebooting in 3 seconds...");
    // A small delay is added to allow the web client to receive the response
    // before the ESP32 restarts and closes the connection.
    delay(3000); 
    ESP.restart(); // Force a reboot of the ESP32 to apply default settings cleanly
}

/**
 * @brief Handles POST requests to the `/api/resetWifi` endpoint.
 * Clears any saved WiFi credentials, forcing the ESP32 to enter Access Point (config) mode on next boot.
 * This also initiates a device reboot.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void handleApiResetWifi(AsyncWebServerRequest *request) {
    Serial.println("Received request to reset WiFi credentials.");
    wifiManager.resetSettings(); // Instruct WiFiManager to clear any saved WiFi network credentials
    request->send(200, "text/plain", "WiFi credentials cleared. Device rebooting to AP mode in 3 seconds to enter config mode...");
    // A small delay is added to allow the web client to receive the response.
    delay(3000); 
    ESP.restart(); // Force a reboot of the ESP32. It will now start in WiFiManager's AP (config) mode.
}

/**
 * @brief Handles requests for paths that are not defined by any other server.on() route.
 * Sends a 404 Not Found response.
 * * @param request Pointer to the AsyncWebServerRequest object.
 */
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}







// --- Helper function to constrain values within a range ---
// Ensures that a given value stays within specified minimum and maximum bounds.
int getConstrainedValue(int val, int minVal, int maxVal) {
  return constrain(val, minVal, maxVal);
}





TM1637Display green1(green_CLK, green1_DIO);
TM1637Display green2(green_CLK, green2_DIO);
TM1637Display green3(green_CLK, green3_DIO);

Adafruit_AlphaNum4 ht16k33_display = Adafruit_AlphaNum4();




//-- Function Prototypes (Forward Declarations) ---
// These tell the compiler that these functions exist and what their signatures are.

void green1Speed(float);
void showMonth(const char*);
void monthdayupdate(int);
void Setup_sound();      // Toggle sound effects on/off or set a timer (function name might be misleading)
void printDetail(uint8_t, int );
void waitMilliseconds(uint16_t);
void Snooze();
void show_hour();
void Setup_alarm();
void alarm();










void setup()
{ 


bool changesMade = false;

Serial.begin(115200); // Initialize serial communication for debugging output
    Serial.println("\nStarting ESP32 Back to the Future Alarm Clock...");

// Setup SSE server for browser refresh
events.onConnect([](AsyncEventSourceClient *client){
  if(client->lastId()) {
    Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
  }
  client->send("connected", NULL, millis(), 1000);
});
server.addHandler(&events);

    // Initialize NVS (Non-Volatile Storage) using the Preferences library.
    // A "namespace" called "alarm-settings" is opened. `false` indicates read-write mode.
    // NVS is used to persistently store our `ClockSettings` struct across reboots.
    if (!preferences.begin("alarm-settings", false)) {
        Serial.println("ERROR: Failed to open Preferences! Settings may not be saved/loaded.");
        // This is a non-critical error for the program to continue, but data persistence will be affected.
    }
    loadSettings(); // Load previously saved settings from NVS, or apply defaults if NVS is empty/corrupted.

    // --- WiFi Management with WiFiManager ---
    // WiFiManager simplifies WiFi provisioning. It will try to connect to a known network.
    // If no network is saved or connection fails, it creates an Access Point (AP)
    // with a captive portal for the user to configure WiFi credentials.
    wifiManager.setAPCallback(configModeCallback); // Set a callback function to run when WiFiManager enters AP mode.
    wifiManager.setMinimumSignalQuality(20);      // Only connect to APs with RSSI >= -20 dBm (higher RSSI is better, e.g., -50 is good, -80 is bad)

    Serial.println("Attempting to auto-connect to WiFi...");
    // autoConnect() attempts to connect to a previously saved WiFi network.
    // If unsuccessful (e.g., no saved credentials, or connection times out after 120 seconds by default),
    // it will create an Access Point (AP) named MDNS_HOSTNAME (e.g., "bttf-clock")
    // allowing a user to connect to it and configure WiFi credentials via a web portal.
    if (!wifiManager.autoConnect(MDNS_HOSTNAME)) {
        Serial.println("Failed to connect to WiFi and timed out. ESP will restart to try config mode.");
        delay(3000); // Small delay before reboot
        ESP.restart(); // Restart the ESP to retry auto-connect or ensure a clean entry into config mode.
    }
    
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());     // Print the IP address assigned to the ESP32
    Serial.print("Gateway IP: ");
    Serial.println(WiFi.gatewayIP());   // Print the network gateway IP
    Serial.print("DNS IP: ");
    Serial.println(WiFi.dnsIP(0));      // Print the primary DNS server IP

    // --- mDNS (Multicast DNS) Responder ---
    // mDNS allows you to access the ESP32 by a friendly hostname (e.g., http://bttf-alarmclock.local/)
    // in your web browser, instead of requiring its dynamic IP address.
    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("mDNS responder started: http://%s.local/\n", MDNS_HOSTNAME);
    } else {
        Serial.println("ERROR: Failed to set up mDNS. You might need to use the IP address directly.");
    }

    // --- Custom NTP Client Initialization ---
    // Initialize the UDP listener for our custom NTP client.
    // The ESP32 will bind to this local port (2390) to receive incoming UDP packets,
    // specifically NTP responses from the servers it queries.
    Udp.begin(2390); 
    Serial.printf("UDP listener started on port %d for NTP client.\n", 2390); // Corrected: Print the known port directly

    // Perform an initial NTP synchronization attempt immediately after connecting to WiFi.
    // This is crucial to set the clock as early as possible.
    processNTPresponse(); 

    // --- Web Server Route Definitions ---
     // Function to set the timezone
       setenv("TZ", currentSettings.timezoneString, 1);
    tzset();
    // Define how the AsyncWebServer responds to different URL paths (endpoints).

    // Root URL: Serve the main HTML page.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(200, "text/html", INDEX_HTML); 
    });
    // Stylesheet URL: Serve the CSS file.
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(200, "text/css", STYLE_CSS); 
    });
    // JavaScript URL: Serve the JavaScript file.
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(200, "application/javascript", SCRIPT_JS); 
    });
    
    // API endpoints for fetching/saving data via AJAX from the web UI.
    server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest *request){ handleApiTime(request); }); 
    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request){ handleApiSettings(request); });
    server.on("/api/saveSettings", HTTP_POST, [](AsyncWebServerRequest *request){ handleApiSaveSettings(request); });
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){ handleApiStatus(request); });
    server.on("/api/resetDefaults", HTTP_POST, [](AsyncWebServerRequest *request){ handleApiResetDefaults(request); });
    server.on("/api/resetWifi", HTTP_POST, [](AsyncWebServerRequest *request){ handleApiResetWifi(request); });
    
    // Fallback handler for any requests to paths not explicitly defined above (404 Not Found).
    server.onNotFound(notFound);

    server.begin(); // Start the HTTP server. It will now listen for incoming client connections.
    Serial.println("HTTP server started");





  pinMode(greenAM, OUTPUT);
  pinMode(greenPM, OUTPUT);
  pinMode(SET_STOP_BUTTON, INPUT);
  pinMode(SET_SOUND_BUTTON, INPUT);
  pinMode(HOUR_BUTTON, INPUT);
  pinMode(MINUTE_BUTTON, INPUT);
  pinMode(SET_STOP_LED, OUTPUT);  //SETUP/STOP BUTTON
  pinMode(2, OUTPUT);  //SETUP/SOUND BUTTON
 
 
  digitalWrite(greenAM, HIGH);
 
  // Initialize NVS Preferences with a namespace "alarm-settings".digitalWriteloop

  // `false` means read/write mode.
  preferences.begin("alarm-settings", false);
  Serial.println("Preferences initialized.");
  loadSettings(); // Load previously saved settings or initialize default
   Serial.print("Loaded timezone string: ");
    Serial.println(currentSettings.timezoneString);
 
 
 
 

if (currentSettings.timeTravelSoundToggle==0) {
   digitalWrite(SET_SOUND_LED,LOW); // added to turn on led on button press
  }
 else {
  digitalWrite(SET_SOUND_LED,HIGH); // added to turn on led on button press
  }


 //soundinterval =(sound_minutes*60000)+(sound_hours*60*60000);
  Wire.begin();
  ht16k33_display.begin(0x70);
  ht16k33_display.setBrightness(currentSettings.brightness*2);
    //update the green1s to blank them
  
    green1.showNumberDecEx(0,0b01000000,true,2,0);
    green1.showNumberDecEx(0,0b01000000,true,2,2);
    green2.showNumberDecEx(0,0b00000000,true);
    green3.showNumberDecEx(0,0b01000000,true,2,0);
    green3.showNumberDecEx(0,0b01000000,true,2,2);
        digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 0);
  Serial.begin(115200);
  //Serial.begin(9600);
  
   

  
    


     ArduinoOTA.setHostname("bttf_clock");
   MDNS.begin("bttf_clock");
  
 

 
   ArduinoOTA.begin();
//setup the web pagew
   // Static IP Address configuration
//IPAddress local_IP(192, 168, 1, 180); // Set your desired static IP (e.g., 192.168.1.180)
//IPAddress gateway(192, 168, 1, 1);    // Your router's gateway IP
//IPAddress subnet(255, 255, 255, 0);   // Your network's subnet mask
//IPAddress primaryDNS(8, 8, 8, 8);     // Google DNS
//IPAddress secondaryDNS(8, 8, 4, 4);   // Google DNS


 



  //SETUP DF_PLAYER
  FPSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }




  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.volume(currentSettings.notificationVolume);  
 // myDFPlayer.play(10);  //Play the first mp3
  delay(100);
  Play_finished = 0; 

  Serial.println("\n Starting");
 myDFPlayer.play(2);
  

    digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 0);
  // count to 88
  currentSpeed=0;
  while (currentSpeed != 88) {
       green1.setBrightness(currentSettings.brightness,1);
                  // Adjust smoothing factor dynamically
                float smoothingFactor = (targetSpeed > currentSpeed) ? 0.02 : 1;
                // Smooth transition
                currentSpeed += (targetSpeed - currentSpeed) * smoothingFactor;
            //    Serial.print("Target Speed: ");
            //    Serial.print(targetSpeed);
            //    Serial.print(" | Smoothed Speed: ");
                
                if (currentSpeed > 87.85) currentSpeed = 88;
               green1Speed(currentSpeed);
              //  Serial.println(currentSpeed);
               //delay (100);
           
  }
   if (currentSpeed==88)   {
   //met the target
                   Serial.print(" | target met ");
              //  Serial.println(currentSpeed);
   delay (1000); //delay 8 seconds
    green1.setBrightness(currentSettings.brightness, true);
    green2.setBrightness(currentSettings.brightness, true);
    green3.setBrightness(currentSettings.brightness, true);
  
  // Initial green1 and LED update
  for (int i = 0; i < 50; i++) {
      showMonth(months[random(0, 12)]);
  //  green1.showNumberDecEx(random(10, 300), 0b00000000, false);
    monthdayupdate(random(1, 30));
    green2.showNumberDecEx(random(1000, 8000), 0b00000000, false);
    green3.showNumberDecEx(random(1000, 1900), 0b00000000, false);
   delay(10);
  }

     // Show months from JAN to OCT with synchronized animations
  for (int i = 0; i < 12; i++) {
      showMonth(months[i]);
    //    green1.showNumberDecEx(random(10, 300), 0b00000000, false);
      monthdayupdate(random(1, 30));
    green2.showNumberDecEx(random(500, 3000), 0b00000000, false);
    green3.showNumberDecEx(random(1000, 1900), 0b00000000, false);
   delay(500);
  }

   }

   ht16k33_display.clear();
    ht16k33_display.writeDisplay();
  green1.setBrightness(currentSettings.brightness, true);
  green1.showNumberDecEx(0, 0b00000000, true);
  green2.setBrightness(0,false);
  green2.showNumberDecEx(0, 0b00000000, true);
  green3.setBrightness(0, false);
  green3.showNumberDecEx(0, 0b00000000, true);
 
  // Store your values in an array
int updateValues[] = {88, 75, 63, 51, 45, 35, 30, 25, 20, 11, 8, 5, 3, 0};

// The loop automatically goes through each 'value' in the 'updateValues' array
for (int value : updateValues) {
  monthdayupdate(value);
  delay(300);
}

  
                 delay(1000);
                 green1.setBrightness(0,0);
                   monthdayupdate(0);
         

               delay(1000);
    currentSpeed=0;
    if (Hour > 11) {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
  } else {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
  }
Serial.printf("Timezone: %s\n", currentSettings.timezoneString);
lastexecutedhour=timeinfo.tm_hour;
lastexecutedminute=timeinfo.tm_min;
 previoustestMillis = millis();

}
void green1Speed(float speed) {
    int whole = (int)speed*10;
   // int decimal = (int)((speed - whole) * 100);
  // int green1Value = (whole * 100) + decimal;
  int green1Value = whole;
     int thousands= (whole)/1000;
   int hundreds=((whole)/100) %10;
    int tens= ((whole)/10) %10;
    int units= (whole) %10;
    segments[0]=0x00;
    segments[3]=0x00;
    segments[1]=green1.encodeDigit(hundreds);
    segments[2]=green1.encodeDigit(tens);
     green1.setSegments(segments);

    //green1.showNumberDecEx(green1Value, 0b00000000, true);
    //    green1.setSegments(new uint8_t[] {0x00},1,0);
  //  green1.setSegments(new uint8_t[] {0x00},1,3);
 
}

/**
 * Main program loop - runs continuously after setup()
 * Performs the following tasks in order:
 * 1. Check and process OTA updates
 * 2. Update time from NTP when needed
 * 3. Check for sleep mode time range
 * 4. Process time-based animations
 * 5. Update display values and brightness
 * 6. Check alarm conditions
 */
void loop()

{

    unsigned long currentMillis = millis();
    unsigned long currentsecMillis = millis();
    unsigned long currenttestMillis = millis();
    unsigned long lastYieldTime = millis();
    static bool colonState = false;
    
  // Handle watchdog
  if (millis() - lastYieldTime > 50) {
    yield();
    lastYieldTime = millis();
  }
  // - Maintaining WiFi connection.
    // - Detecting disconnects and re-attempting connection.
    // - Running the captive portal if WiFi is lost or reset.
    wifiManager.process(); 

    // --- Custom NTP Sync Retry Logic ---
    // Only attempt NTP synchronization if WiFi is currently connected.
    if (WiFi.status() == WL_CONNECTED) {
        // Condition for attempting NTP sync:
        // - `!timeSynchronized`: If time has never been successfully synchronized yet.
        // - `(millis() - lastNtpRequestSent >= currentNtpInterval)`: If the calculated interval
        //   since the last NTP request was sent has elapsed. This handles both normal polling
        //   and exponential backoff for failed attempts.
        if (!timeSynchronized || (millis() - lastNtpRequestSent >= currentNtpInterval)) {
            Serial.printf("Time to attempt NTP sync. Current interval: %lu ms.\n", currentNtpInterval);
            processNTPresponse(); // Call the custom NTP function to send a request and process the response
        }
    } else {
        // If WiFi is disconnected, log this periodically to the serial monitor.
        // This helps in diagnosing network issues and prevents continuous logging.
        if (millis() - lastNtpRequestSent >= 5000) { // Log every 5 seconds if disconnected
            Serial.println("WiFi is disconnected. Cannot attempt NTP sync.");
            lastNtpRequestSent = millis(); // Reset this to prevent spamming serial too fast
        }
    }
    
    //    Periodically retrieve the current time from the ESP32's system clock.
    //    `getLocalTime()` converts the UTC time (set by NTP) to local time
    //    based on the `TZ_INFO` configured earlier.
       if (timeSynchronized)  getLocalTime(&timeinfo);
    //       `// Time is available and current
    //    Use the time components (hour, minute, second) to update the  display.
     //    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, currentSettings.displayFormat24h);
     //date components:
    //    timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);




    
    h=1;
    Play_finished = 0;


    ArduinoOTA.handle();

       //CHECK IF sleep time
  
 
   // set time variables
    bool sleeptimeactive = false;

    if (timeSynchronized) {
        // Create a time_t value for the current time
        time_t current_time = mktime(&timeinfo);

        // Create time_t values for departure and arrival times on the current day
        struct tm departure_tm = timeinfo;
        departure_tm.tm_hour = currentSettings.departureHour;
        departure_tm.tm_min = currentSettings.departureMinute;
        departure_tm.tm_sec = 0;
        time_t departure_time = mktime(&departure_tm);

        struct tm arrival_tm = timeinfo;
        arrival_tm.tm_hour = currentSettings.arrivalHour;
        arrival_tm.tm_min = currentSettings.arrivalMinute;
        arrival_tm.tm_sec = 0;
        time_t arrival_time = mktime(&arrival_tm);

        // Adjust for crossing midnight
        if (departure_time > arrival_time) {
            // If departure time is later than arrival time, it means the sleep period crosses midnight.
            // We check if the current time is either after the departure time OR before the arrival time.
            sleeptimeactive = (current_time >= departure_time || current_time < arrival_time);
        } else {
            // If departure time is earlier than arrival time, the sleep period is within the same day.
            // We check if the current time is between the departure and arrival times.
            sleeptimeactive = (current_time >= departure_time && current_time < arrival_time);
        }

       // Serial.print("Current time: ");
       // Serial.println(current_time);
       // Serial.print("Departure time: ");
       // Serial.println(departure_time);
       // Serial.print("Arrival time: ");
       // Serial.println(arrival_time);
       // Serial.print("Sleep time active: ");
        Serial.println(sleeptimeactive);

    }
    
    //CHECK IF sleep time
  
  if ( sleeptimeactive) {  //greater than sleep start or less than sleep stop am
    //  do nothing during sleep time
  }
  else {
//process the loop normally
  // Check if settings were updated via web interface
  if (changesMade) {
    // Apply new settings
           Serial.println("Web page settings were updated! Performing actions...");
              
               setenv("TZ", currentSettings.timezoneString, 1);
            tzset();
   //change the player volume
    myDFPlayer.volume(currentSettings.notificationVolume);  
    //CHECK TO SEE IF alarmOnOff HAS CHANGED, IF SO CHANGE THE LED STATUS
  if(currentSettings.alarmOnOff == 1){
    digitalWrite(SET_STOP_LED,HIGH);
     }
  else {
       digitalWrite(SET_STOP_LED,LOW);
  }

  //CHECK TO SEE IF timeTravelSoundToggle HAS CHANGED, IF SO CHANGE THE LED STATUS
  if (currentSettings.timeTravelSoundToggle) {
    digitalWrite(SET_SOUND_LED, HIGH);
     }
  else {
       digitalWrite(SET_SOUND_LED,LOW);
  }
    //set the display brightness 
     green1.setBrightness(currentSettings.brightness,1);
     green2.setBrightness(currentSettings.brightness,1);
     green3.setBrightness(currentSettings.brightness,1);
     ht16k33_display.setBrightness(currentSettings.brightness*2);

                 // If time travel sound effects are enabled, trigger them:
            if (currentSettings.timeTravelSoundToggle) {
                Serial.println("Playing Time Travel Sound FX!");
                // Example: `playTimeTravelSound(currentSettings.notificationVolume);`
                // You'd need an MP3 module or a buzzer to play sounds.
            }

            // If "Power of Love" (Flux Capacitor) is toggled on:
            if (currentSettings.powerOfLoveToggle) {
                Serial.println("Activating Power of Love (Flux Capacitor)!");
                // Example: `activateFluxCapacitorLEDs();`
                // This would control specific LEDs or other visual elements.
               
                 myDFPlayer.play(8);
                 currentSettings.powerOfLoveToggle=false;
                saveSettings();
                triggerBrowserRefresh();
                

            }  
 
    changesMade = false;  // Reset flag after processing
  }

    // --- Time Travel Animation Trigger (NEW) ---
    // Only trigger if time is synchronized and animation interval is set (not 0)
    if (timeSynchronized && currentSettings.timeTravelAnimationInterval > 0) {
        unsigned long currentMillis = millis();
        // Convert the animation interval from minutes to milliseconds
        unsigned long intervalMs = (unsigned long)currentSettings.timeTravelAnimationInterval * 60 * 1000; 

        if (currentMillis - lastAnimationTriggerTime >= intervalMs) {
            Serial.printf("Triggering Time Travel Animation (every %d min)!\n", currentSettings.timeTravelAnimationInterval);


  previoustestMillis = currenttestMillis;
        //run the animation
        if (currentSettings.timeTravelSoundToggle==1) myDFPlayer.play(2);
      //  if (currentSettings.timeTravelSoundToggle==1)  myDFPlayer.play(random(10,14)); //Playing the alarm sound
//BLANK THE DISPLAYS
 ht16k33_display.clear();
    ht16k33_display.writeDisplay();
  green1.setBrightness(currentSettings.brightness,1);
  green1.showNumberDecEx(0, 0b00000000, true);
  green2.setBrightness(0,0);
  green2.showNumberDecEx(0, 0b00000000, true);
  green3.setBrightness(0,0);
  green3.showNumberDecEx(0, 0b00000000, true);
      digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 0);
         // count to 88
  currentSpeed=0;
  while (currentSpeed != 88) {
       green1.setBrightness(currentSettings.brightness,1);
                  // Adjust smoothing factor dynamically
                float smoothingFactor = (targetSpeed > currentSpeed) ? 0.02 : 1;
                // Smooth transition
                currentSpeed += (targetSpeed - currentSpeed) * smoothingFactor;
              //  Serial.print("Target Speed: ");
              //  Serial.print(targetSpeed);
            //    Serial.print(" | Smoothed Speed: ");
                
                if (currentSpeed > 87.85) currentSpeed = 88;
               green1Speed(currentSpeed);
             //   Serial.println(currentSpeed);
               //delay (100);
           
  }
   if (currentSpeed==88)   {
   //met the target
                   Serial.print(" | target met ");
                Serial.println(currentSpeed);
   delay (1000); //delay 8 seconds
    green1.setBrightness(currentSettings.brightness,1);
    green2.setBrightness(currentSettings.brightness,2);
  green2.showNumberDecEx(0, 0b00000000, true);
  green3.setBrightness(currentSettings.brightness,1);
  green3.showNumberDecEx(0, 0b00000000, true);
  // Initial green1 and LED update
  for (int i = 0; i < 50; i++) {
      showMonth(months[random(0, 12)]);

    //green1.showNumberDecEx(random(10, 300), 0b00000000, false);
    monthdayupdate(random(1,30));
    green2.showNumberDecEx(random(1000, 8000), 0b00000000, false);
    green3.showNumberDecEx(random(1000, 1900), 0b00000000, false);
   delay(10);
  }

  

   }

   ht16k33_display.clear();
    ht16k33_display.writeDisplay();
  green1.setBrightness(currentSettings.brightness,1);
  green1.showNumberDecEx(0, 0b00000000, false);
      uint8_t seg_zero[] = {0x00};
      green1.setSegments(seg_zero, 1, 0);
  green2.setBrightness(0,0);
  green2.showNumberDecEx(0, 0b00000000, false);
  green3.setBrightness(0,0);
  green3.showNumberDecEx(0, 0b00000000, true);
 // Store your values in an array
int updateValues[] = {88, 75, 63, 51, 45, 35, 30, 25, 20, 11, 8, 5, 3, 0};

// The loop automatically goes through each 'value' in the 'updateValues' array
for (int value : updateValues) {
  monthdayupdate(value);
  delay(300);
}
  
                 delay(1000);
                 green1.setBrightness(0,0);
                   monthdayupdate(0);
         

               delay(1000);
    currentSpeed=0;
    if (Hour > 11) {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
  } else {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
  }

        lastexecutedhour=currenthour;
        lastexecutedminute=currentminute;
  


   



            // Reset the timer for the next animation trigger
            lastAnimationTriggerTime = currentMillis; 
        }

    }

    if (myDFPlayer.available())
   {
      printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
     }
   if (digitalRead(SET_STOP_BUTTON) == true){
    if(currentSettings.alarmOnOff == 1){
    currentSettings.alarmOnOff = 0;
    saveSettings();
    triggerBrowserRefresh();
      myDFPlayer.stop();
     myDFPlayer.play(14);
    }
  else {
    currentSettings.alarmOnOff = 1;
    saveSettings();
    triggerBrowserRefresh();
    myDFPlayer.stop();
  myDFPlayer.play(13);
  }
  monthdayupdate(currentSettings.alarmOnOff);  
  showMonth(months[14]);
 // delay(1000); 
   }
   

   if (digitalRead(SET_SOUND_BUTTON) == true)
    { 
     Setup_sound() ;
     
     myDFPlayer.stop();
     }
    if (digitalRead(SET_STOP_BUTTON) == true)
    { 
     Setup_alarm() ;
     myDFPlayer.stop();
     digitalWrite(SET_STOP_LED,LOW);
    }
if (currentSettings.destinationHour == timeinfo.tm_hour && flag_alarm == 0 && currentSettings.alarmOnOff == 1)
{
  if (currentSettings.destinationMinute == timeinfo.tm_min) {
    flag_alarm=1;
    alarm();
    currentSettings.alarmOnOff = 1;
    delay(2000);
    Serial.println("Exit alarm");
    }
} 

if(currentSettings.alarmOnOff == 1){digitalWrite(SET_STOP_LED,HIGH);}
else digitalWrite(SET_STOP_LED,LOW);

if(currentSettings.destinationMinute != timeinfo.tm_min )
{flag_alarm=0;}


if(digitalRead(MINUTE_BUTTON) == true && digitalRead(HOUR_BUTTON) == true ) // Push Min and Hour simultanetly to PLAY THE EASTER EGG
 { 
      if(easter_egg == 0) {
        easter_egg=1;
         if (currentSettings.timeTravelSoundToggle==1) {
          myDFPlayer.stop();
          myDFPlayer.play(8);
         }

      }
      else {easter_egg = 0;
        //pixels.setBrightness(0);
         if (currentSettings.timeTravelSoundToggle==1) { 
          myDFPlayer.stop();
          
         }
      
      }
       showMonth(months[15]);
       delay(1000);
    }

  
 // green3.showNumberDecEx(00,0b01000000,true,2,0);
//  green3.showNumberDecEx(alarmOnOff,0b01000000,true,2,2);



  
else {
if (digitalRead(HOUR_BUTTON) == true ) // Push hour butoon for volume down
{ 

  currentSettings.notificationVolume =currentSettings.notificationVolume-1;
  if (currentSettings.notificationVolume <=0) currentSettings.notificationVolume=0;
 saveSettings();
  triggerBrowserRefresh();
  myDFPlayer.volume(currentSettings.notificationVolume);  
  myDFPlayer.play(14);

 monthdayupdate(currentSettings.notificationVolume);
 showMonth(months[12]);
  //green3.showNumberDecEx(00,0b01000000,true,2,0);
 // green3.showNumberDecEx(notificationVolume',0b01000000,true,2,2);
  delay(1000);


  }

    if (digitalRead(MINUTE_BUTTON) == true ) // Push minute butoon for volume up
{ 

  currentSettings.notificationVolume =currentSettings.notificationVolume+1;
  if (currentSettings.notificationVolume >=30) currentSettings.notificationVolume=30;
  myDFPlayer.volume(currentSettings.notificationVolume);  
                  saveSettings();
                triggerBrowserRefresh();
  myDFPlayer.play(13);

monthdayupdate(currentSettings.notificationVolume);
showMonth(months[12]);
// green3.showNumberDecEx(00,0b01000000,true,2,0);
 // green3.showNumberDecEx(notificationVolume',0b01000000,true,2,2);
  delay(1000);


  }
  }



    }


  int currentYear = timeinfo.tm_year+1900;
 // Serial.print("Year: ");
 //Serial.println(currentYear);
// dates: timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  int monthDay = timeinfo.tm_mday;
// Serial.print("Month day: ");
 // Serial.println(monthDay);

  int currentMonth = timeinfo.tm_mon+1;
 // Serial.print("Month: ");
 //Serial.println(currentMonth);

      if (timeinfo.tm_hour == 0)
  { 
    Hour = 12;
   if (!sleeptimeactive) {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
   }

    //Serial.write(" =0");
  }

  else if (timeinfo.tm_hour == 12)
  { 
    Hour = timeinfo.tm_hour;
    if (!sleeptimeactive) {
    digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 1);
    }

  //  Serial.write(" =12");
  }
  
  else if (timeinfo.tm_hour >= 13) {
    if (currentSettings.displayFormat24h) {
        Hour = timeinfo.tm_hour;
    }
    else {
         Hour = timeinfo.tm_hour - 12;
    }
    if (!sleeptimeactive) {
    digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 1);
    }

  //  Serial.write(" >=13");
  }

  else {
    Hour = timeinfo.tm_hour;
   if (!sleeptimeactive) {
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);
   }


   // Serial.write("else");
  }


//update the display during sleep and wake times
  

  if (sleeptimeactive) {

    if (nighttimeonlyonce) {

      if (currentSettings.timeTravelSoundToggle==1) myDFPlayer.play(10); //play only once
    digitalWrite(greenAM, 0);
   // Serial.println("27");

    digitalWrite(greenPM, 0);
   // Serial.println("12"); 
    digitalWrite(SET_STOP_LED, 0);
    //Serial.println("26");
    digitalWrite(SET_SOUND_LED, 0);
   // Serial.println("2");

    green1.setBrightness(0,0);
     green2.setBrightness(0,0);
    green3.setBrightness(0,0);
 //ht16k33_green1.setBrightness(1);
 Serial.println("else not sleeptimeactive and set nighttimeonlyonce=true");
   ht16k33_display.clear();
    ht16k33_display.writeDisplay();

    //update the green1s to blank them
  
    green1.showNumberDecEx(0,0b01000000,true,2,0);
    green1.showNumberDecEx(0,0b01000000,true,2,2);
    green2.showNumberDecEx(0,0b00000000,true);
    green3.showNumberDecEx(0,0b01000000,true,2,0);
    green3.showNumberDecEx(0,0b01000000,true,2,2);
    
    nighttimeonlyonce=false;
     }
  } else {
    nighttimeonlyonce=true;

   //ht16k33_green1.setBrightness(1);
  green1.setBrightness(currentSettings.brightness);
  green2.setBrightness(currentSettings.brightness);
  green3.setBrightness(currentSettings.brightness);



  

 



  //update green green1
    // Blink colon every second for green3 only
  if (currentMillis - previoussecMillis >= secinterval) {
    previoussecMillis = currentsecMillis;
    colonState = !colonState; // Toggle colon state
    //green1.showNumberDecEx(monthDay*10,0b00000000,false);
   
   
  //update green1 display with monthday
  monthdayupdate(monthDay);

   // green1.setSegments(new uint8_t[] {0x00},1,0);
  //green1.setSegments(new uint8_t[] {0x00},1,3);
    

    green2.showNumberDecEx(currentYear,0b00000000,true);
    green3.showNumberDecEx(Hour, colonState ? 0b01000000 : 0b00000000,true,2,0);
    green3.showNumberDecEx(timeinfo.tm_min, colonState ? 0b01000000 : 0b00000000,true,2,2);
    showMonth(months[currentMonth-1]);
  } 







if(timeinfo.tm_hour>=13){
    digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 1);}

  
else if(timeinfo.tm_hour==12){
    digitalWrite(greenAM, 0);
    digitalWrite(greenPM, 1);}


else{
    digitalWrite(greenAM, 1);
    digitalWrite(greenPM, 0);}

  

  }  
}


void monthdayupdate(int monthDay) {

    int thousands= (monthDay*10)/1000;
   int hundreds=((monthDay*10)/100) %10;
    int tens= ((monthDay*10)/10) %10;
    int units= (monthDay*10) %10;
    segments[0]=0x00;
    segments[3]=0x00;
    segments[1]=green1.encodeDigit(hundreds);
    segments[2]=green1.encodeDigit(tens);
     green1.setSegments(segments);


}

void showMonth(const char* month) {
  ht16k33_display.clear();
  for (int i = 0; i < 3; i++) {
    ht16k33_display.writeDigitAscii(i+1, month[i]);
  }
  ht16k33_display.writeDisplay();
}


// A helper function to check all relevant buttons at once
// Returns: 0 = no button, 1 = stop button, 2 = snooze button
int checkButtons() {
  if (digitalRead(SET_STOP_BUTTON) == true) {
    return 1; // Stop
  }
  if (digitalRead(MINUTE_BUTTON) == true || digitalRead(HOUR_BUTTON) == true) {
    return 2; // Snooze
  }
  return 0; // No button pressed
}

/**
 * @brief Waits for a specific duration without blocking the CPU.
 * * This function continuously checks for button presses during the wait period.
 * It's a non-blocking alternative to delay().
 *
 * @param ms The number of milliseconds to wait.
 * @return int The result of checkButtons() if a button is pressed (1 for STOP, 2 for SNOOZE), 
 * or 0 if the delay completes without any button press.
 */
int responsiveDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    int buttonState = checkButtons();
    if (buttonState != 0) {
      return buttonState; // A button was pressed, so return immediately.
    }
    // You can add other repeating tasks here if needed, like updating a display.
    delay(1); // A tiny delay to prevent the loop from overwhelming the CPU.
  }
  return 0; // The full delay time has passed without interruption.
}

void alarm() {
  bool alarmFullyStopped = false;

  // This outer loop allows the alarm to restart after a snooze.
  while (!alarmFullyStopped) {
    int buttonState = 0;

    // --- Start of Alarm Sequence ---
    showMonth(months[14]); // Display "alm"
    myDFPlayer.play(2);    // Play the initial alarm sound

    // --- 88 MPH Acceleration Sequence (Now fully responsive) ---
    for (int u = 0; u < 89; u++) {
      monthdayupdate(u);
      buttonState = responsiveDelay(20);
      if (buttonState != 0) break; // Exit loop if a button is pressed
    }

    // Handle button press from the sequence above
    if (buttonState == 1) { alarmFullyStopped = true; break; }
    if (buttonState == 2) { Snooze(); continue; }

    // --- Countdown Sequence (Now fully responsive) ---
    int countdownValues[] = {88, 75, 63, 51, 45, 35, 30, 25, 20, 11, 8, 5, 3, 0};

    buttonState = responsiveDelay(3000);
    if (buttonState == 1) { alarmFullyStopped = true; break; }
    if (buttonState == 2) { Snooze(); continue; }

    for (int val : countdownValues) {
      monthdayupdate(val);
      buttonState = responsiveDelay(300);
      if (buttonState != 0) break; // Exit loop if a button is pressed
    }
    if (buttonState == 1) { alarmFullyStopped = true; break; }
    if (buttonState == 2) { Snooze(); continue; }

    buttonState = responsiveDelay(1000);
    if (buttonState == 1) { alarmFullyStopped = true; break; }
    if (buttonState == 2) { Snooze(); continue; }
    
    green1.setBrightness(0, 0);
    monthdayupdate(0);
    
    buttonState = responsiveDelay(1000);
    if (buttonState == 1) { alarmFullyStopped = true; break; }
    if (buttonState == 2) { Snooze(); continue; }

    // --- Main Alarm Loop (already quite responsive, no change needed here) ---
    digitalWrite(SET_STOP_LED, HIGH);
    myDFPlayer.play(random(1, 9));

    while (true) {
      show_hour();
      if (myDFPlayer.available() && myDFPlayer.readType() == DFPlayerPlayFinished) {
        myDFPlayer.play(random(1, 9));
      }

      buttonState = checkButtons();
      if (buttonState == 1) { // STOP
        alarmFullyStopped = true;
        break;
      }
      if (buttonState == 2) { // SNOOZE
        Snooze();
        break; // Exits this inner loop, outer loop will restart alarm
      }
      delay(50); // Small delay here is fine for loop pacing
    }
  }

  // --- Cleanup after alarm is fully stopped ---
  myDFPlayer.stop();
  currentSettings.alarmOnOff = false; // Disarm the alarm
  saveSettings();                     // Save the new state
  triggerBrowserRefresh();            // Update the web UI
  digitalWrite(SET_STOP_LED, LOW);
  Serial.println("Alarm stopped.");
  while (digitalRead(SET_STOP_BUTTON) == true) { // Wait for button release
    delay(20);
  }
}
// Snooze if you'd like to sleep a few minutes more 
void Snooze() { // This function is called from within the blocking alarm() function
  myDFPlayer.stop();
  showMonth(months[16]); // Display "snz"
  Serial.println("SNOOZING... time will update on display.");

  // Turn off other displays during snooze
  green1.setBrightness(0, 0);
  green2.setBrightness(0, 0);
  green1.showNumberDecEx(0,0b01000000,true,2,0);
  green1.showNumberDecEx(0,0b01000000,true,2,2);
  green2.showNumberDecEx(0,0b00000000,true);

  unsigned long snoozeStartTime = millis();
  unsigned long snoozeDurationMs = (unsigned long)currentSettings.snoozeMinutes * 60 * 1000;

  // Non-blocking snooze loop
  while (millis() - snoozeStartTime < snoozeDurationMs) {
    // Check if the user wants to cancel the snooze and stop the alarm
    if (checkButtons() == 1) { // 1 is the STOP button
      break; // Exit the snooze loop early
    }

    // **THE FIX:** Get fresh time data and recalculate Hour INSIDE the loop
    getLocalTime(&timeinfo);

    if (currentSettings.displayFormat24h) {
      Hour = timeinfo.tm_hour;
    } else {
      Hour = timeinfo.tm_hour % 12;
      if (Hour == 0) {
        Hour = 12; // Handle 12 AM/PM
      }
    }

    show_hour(); // Now displays the updated time

    // Wait for 1 second before the next update, while still checking buttons
    if (responsiveDelay(1000) == 1) {
      break;
    }
  }
}
void show_hour(){
  green3.setBrightness(currentSettings.brightness,1);
green3.showNumberDecEx(Hour,0b01000000,true,2,0);
green3.showNumberDecEx(timeinfo.tm_min,0b01000000,true,2,2);

}


void Setup_sound() {
//toggle the sound
if (currentSettings.timeTravelSoundToggle==0) {
  currentSettings.timeTravelSoundToggle=1;
  digitalWrite(SET_SOUND_LED,HIGH); // added to turn on led on button press
 saveSettings();
 triggerBrowserRefresh();
  monthdayupdate(currentSettings.timeTravelSoundToggle);  
  showMonth(months[13]);
  myDFPlayer.play(13);
 }
else {
  currentSettings.timeTravelSoundToggle=0;
  saveSettings();
  triggerBrowserRefresh();
  digitalWrite(SET_SOUND_LED,LOW); // added to turn on led on button press
  myDFPlayer.play(14);
  monthdayupdate(currentSettings.timeTravelSoundToggle); 
  showMonth(months[13]);
 }
while (digitalRead(SET_SOUND_BUTTON) == true)
{ 

     
  if (digitalRead(MINUTE_BUTTON) == true){
    sound_minutes = sound_minutes + 1;
  }

  if (digitalRead(HOUR_BUTTON) == true){
    sound_hours = sound_hours + 1;
  }
  if (sound_minutes > 59){sound_minutes=0;}
  if (sound_hours > 23){sound_hours=0;}
 
 delay(100);

  green3.showNumberDecEx(sound_hours,0b01000000,true,2,0);
  green3.showNumberDecEx(sound_minutes,0b01000000,true,2,2);


}
//the sound interval to set the animation in ms
//soundinterval =(sound_minutes*60000)+(sound_hours*60*60000);

}

void Setup_alarm() {


while (digitalRead(SET_STOP_BUTTON) == true)
{ 
  if (digitalRead(MINUTE_BUTTON) == true){
    currentSettings.destinationMinute = currentSettings.destinationMinute + 1;
  }

  if (digitalRead(HOUR_BUTTON) == true){
    currentSettings.destinationHour = currentSettings.destinationHour + 1;
  }
  if (currentSettings.destinationMinute > 59){currentSettings.destinationMinute=0;}
  if (currentSettings.destinationHour > 23){currentSettings.destinationHour=0;}
 
 delay(100);

  green3.showNumberDecEx(currentSettings.destinationHour,0b01000000,true,2,0);
  green3.showNumberDecEx(currentSettings.destinationMinute,0b01000000,true,2,2);


}

}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      Play_finished = 1;
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();

  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications
    // to be handled without interrupts
    delay(1);
  }
}

void triggerBrowserRefresh() {
  events.send("refresh", "command", millis());
}