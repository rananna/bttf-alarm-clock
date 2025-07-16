# Back to the Future - ESP32 Alarm Clock

 <!-- Replace with a GIF or image of your clock in action! -->

A WiFi-connected, triple-display alarm clock inspired by the iconic Back to the Future time circuits. Built using an ESP32, this clock automatically synchronizes time over the internet and offers full configuration through a web interface.

This project uses three 7-segment displays to show the "Present Time" (current date and time), "Destination Time" (alarm time), and "Last Time Departed" (user-configurable sleep/wake times). It features authentic sound effects and time travel animations, making it a perfect project for any BTTF fan.

---

## Features

*   **Triple Time Circuit Displays**: Shows current, destination (alarm), and departure/arrival times.
*   **NTP Time Synchronization**: Automatically fetches and syncs time from multiple fallback NTP servers for high reliability.
*   **Full Time Zone Support**: Includes automatic Daylight Saving Time adjustments.
*   **Web Configuration Portal**: Configure all settings easily from any device on your network via a web browser at `http://bttf-clock.local`.
*   **WiFi Manager**: Simple initial WiFi setup using a captive portal. No need to hardcode credentials.
*   **Configurable Alarm**: Set alarm time, toggle on/off, and configure snooze duration.
*   **Sound Effects**: Plays iconic sounds from the movie for alarms and animations using a DFPlayer Mini MP3 module.
*   **Time Travel Animations**: Watch the displays go haywire and the speedometer hit 88 MPH at configurable intervals!
*   **Power Saving Mode**: Displays automatically turn off during user-defined "sleep" hours.
*   **Over-The-Air (OTA) Updates**: Update the firmware wirelessly without needing a physical connection.
*   **Customizable Themes**: Change the color of the displays (Green, Red, Amber, Blue) from the web UI.

---

## Hardware Requirements

| Component                 | Quantity | Notes                                                              |
| ------------------------- | :------: | ------------------------------------------------------------------ |
| **ESP32 Dev Module**      |    1     | The core of the project.                                           |
| TM1637 7-Segment Display  |    3     | For the three main time circuit readouts.                          |
| Adafruit AlphaNum4 Display|    1     | 14-segment I2C display for showing the month.                      |
| DFPlayer Mini MP3 Module  |    1     | For playing sound effects.                                         |
| MicroSD Card              |    1     | For storing MP3 sound files for the DFPlayer.                      |
| Speaker                   |    1     | 8 Ohm, small speaker for sound output.                             |
| Push Buttons              |    4     | For physical interaction (Set/Stop, Sound/Toggle, Hour, Minute).   |
| LEDs                      |    4     | 2 for AM/PM, 2 for status indicators.                              |
| Resistors                 |  ~4-8    | Pull-down for buttons, current-limiting for LEDs. (e.g., 10kΩ, 220Ω) |
| Breadboard & Jumper Wires |          | For prototyping and connections.                                   |
| 5V Power Supply           |    1     | To power the ESP32 and components (e.g., USB adapter).             |

## Software & Libraries

This project is built using the Arduino framework for the ESP32. You will need to install the following libraries through the Arduino IDE Library Manager or PlatformIO:

*   `WiFiManager` by tzapu
*   `Adafruit GFX Library`
*   `Adafruit LED Backpack Library`
*   `DFRobotDFPlayerMini` by DFRobot
*   `ESPAsyncWebServer`
*   `AsyncTCP`
*   `ArduinoJson`
*   `TM1637` by Avishay Orpaz

The following libraries are typically included with the ESP32 core installation:

*   `WiFi.h`
*   `ArduinoOTA.h`
*   `ESPmDNS.h`
*   `Wire.h`
*   `Preferences.h`

---

## Wiring Guide

Connect the components to the ESP32 according to the pin definitions in the code.

### Displays
| Component                 | Pin | ESP32 GPIO |
| ------------------------- |:---:| :--------: |
| **TM1637 Displays (Shared)** | CLK |    13    |
| TM1637 Display 1 (Month/Day) | DIO |    18    |
| TM1637 Display 2 (Year)     | DIO |    15    |
| TM1637 Display 3 (Time)     | DIO |    14    |
| **Adafruit AlphaNum4 (I2C)** | SDA |    21    |
|                           | SCL |    22    |

### Audio (DFPlayer Mini)
| Component        | Pin | ESP32 GPIO |
| ---------------- |:---:| :--------: |
| **DFPlayer Mini**| RX  |  17 (TXD2) |
|                  | TX  |  16 (RXD2) |

### Buttons & LEDs
| Component          | ESP32 GPIO | Notes                                  |
| ------------------ | :--------: | -------------------------------------- |
| Set/Stop Button    |     34     | Connect to GND with a pull-down resistor. |
| Set/Sound Button   |     4      | Connect to GND with a pull-down resistor. |
| Hour Button        |     33     | Connect to GND with a pull-down resistor. |
| Minute Button      |     32     | Connect to GND with a pull-down resistor. |
| **AM LED**         |     27     | Connect to GND with a resistor.        |
| **PM LED**         |     12     | Connect to GND with a resistor.        |
| Set/Stop LED       |     26     | Connect to GND with a resistor.        |
| Set/Sound LED      |     2      | Connect to GND with a resistor.        |

---

## Installation & Setup

### 1. Hardware Assembly

Wire all the components together as shown in the Wiring Guide. It's recommended to start on a breadboard before soldering a permanent circuit.

### 2. Prepare the SD Card

1.  Format a MicroSD card to FAT16 or FAT32.
2.  Create a folder named `mp3` in the root of the SD card.
3.  Place your sound effect files inside the `mp3` folder. The files must be named in a specific way: `0001.mp3`, `0002.mp3`, etc.
    *   `0002.mp3`: Time travel / alarm sound
    *   `0008.mp3`: Power of Love easter egg
    *   `0010.mp3`: Sleep mode activation sound
    *   `0013.mp3`: "On" or positive confirmation sound
    *   `0014.mp3`: "Off" or negative confirmation sound
    *   ... and other random sounds for the alarm (`0001.mp3` - `0009.mp3`)
4.  Insert the SD card into the DFPlayer Mini module.

### 3. Flash the Firmware

1.  **Install Arduino IDE/PlatformIO**: Set up your preferred development environment and install the ESP32 board definitions.
2.  **Install Libraries**: Using the Library Manager, install all the libraries listed in the Software & Libraries section.
3.  **Configure Time Zone**: Open the `bttf_alarm_clock_copy.ino` file. Find the `TZ_INFO` constant and change it to match your local time zone. You can find the correct format string from this list.
    ```cpp
    // Example for New York (Eastern Time)
    const char* TZ_INFO = "EST5EDT,M3.2.0,M11.1.0"; 
    ```
4.  **Upload Code**: Select your ESP32 board and the correct COM port, then upload the sketch.

### 4. WiFi Configuration

1.  On the first boot (or after resetting WiFi credentials), the clock will create its own WiFi network with the SSID `bttf-clock`.
2.  Connect to this network with your phone or computer.
3.  A captive portal page should automatically open in your browser. If it doesn't, navigate to `192.168.4.1`.
4.  Click "Configure WiFi", select your home WiFi network from the list, and enter your password.
5.  Click "Save". The clock will restart and automatically connect to your network.

---

## Usage

### Web Interface

Once connected to your WiFi, you can access the full configuration panel by navigating to **http://bttf-clock.local/** in your web browser.

 <!-- Replace with a screenshot of your web UI -->

From here, you can control all aspects of the clock:
*   **Destination Time**: Set the alarm time.
*   **Alarm On/Off**: Enable or disable the alarm.
*   **Snooze Time**: Set the duration for the snooze function.
*   **Departure/Arrival Times**: Define the "sleep" period where the displays will turn off to save power and prevent light disturbance.
*   **Display Brightness & Volume**: Adjust the brightness of the LEDs and the volume of the sound effects.
*   **Time Travel Animation**: Set how often (in minutes) the animation plays, or turn it off (0).
*   **Sound FX & Power of Love**: Toggle sound effects and trigger the "Power of Love" easter egg sound.
*   **24 Hour Format & Theme**: Customize the time format and display color.
*   **Device Management**: Refresh status, reset settings to default, or reset WiFi credentials.

### Physical Buttons

The clock can also be operated using the four physical buttons.

*   **SET/STOP Button**:
    *   **Single Press**: Toggles the alarm On/Off.
    *   **Press and Hold**: Enters alarm setting mode. Use the HOUR/MINUTE buttons to adjust the alarm time. Release to save.
    *   **During Alarm**: Stops the alarm.

*   **SET/SOUND Button**:
    *   **Single Press**: Toggles the sound effects On/Off.

*   **HOUR Button**:
    *   **When Idle**: Decreases the volume by one step.
    *   **In Setup Mode**: Increments the hour.
    *   **During Alarm**: Activates Snooze.

*   **MINUTE Button**:
    *   **When Idle**: Increases the volume by one step.
    *   **In Setup Mode**: Increments the minute.
    *   **During Alarm**: Activates Snooze.

*   **Easter Egg**:
    *   Press the **HOUR** and **MINUTE** buttons simultaneously to trigger the "Power of Love" sound effect!

---

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

---

## Acknowledgements

This project wouldn't be possible without the hard work of the open-source community and the creators of the fantastic libraries used. Special thanks to the teams behind the ESP32 Arduino core, WiFiManager, and all the Adafruit and DFRobot libraries.


