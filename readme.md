# Back to the Future - ESP32 Alarm Clock

<p align="center">
  <!-- Replace with a GIF or image of your clock in action! -->
  <img src="https://via.placeholder.com/600x300.png?text=BTTF+Clock+in+Action" alt="BTTF Alarm Clock">
</p>

> **Great Scott!** It appears you've stumbled upon the schematics for a temporal displacement alarm clock. While this device can't actually travel through time (flux capacitor technology is still a bit tricky), it brings the iconic look and feel of the DeLorean's time circuits right to your nightstand. Using the power of an ESP32 and a little bit of 1.21-gigawatt... I mean, 5-volt... ingenuity, this clock connects to your local WiFi network to display the precise date and time. You can set a "Destination Time" alarm and a "Last Time Departed" sleep schedule, all configurable from a web interface. So, fire it up, but be warned: once this baby hits 88 miles per hour on the display, you're going to see some serious stuff!

---

### Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Bill of Materials (BOM)](#bill-of-materials-bom)
- [Software & Libraries](#software--libraries)
- [Wiring Guide](#wiring-guide)
- [Installation & Setup](#installation--setup)
- [Usage](#usage)
  - [Web Interface](#web-interface)
  - [Physical Buttons](#physical-buttons)
- [License](#license)
- [Acknowledgements](#acknowledgements)
## Features

This project is packed with features to create an authentic and highly functional alarm clock:

*   **BTTF-Themed Display Layout**: Four displays work together to show the current date and time.
    *   **Month**: An alphanumeric display shows the three-letter month abbreviation (e.g., `JUL`).
    *   **Day**: A 7-segment display shows the day of the month.
    *   **Year**: A 7-segment display shows the full year.
    *   **Time**: A 7-segment display shows the current hour and minute, with a blinking colon.
*   **Accurate & Automatic Time**:
    *   **NTP Time Synchronization**: Automatically fetches and syncs time from multiple fallback NTP servers for high reliability.
    *   **Full Time Zone Support**: Includes automatic Daylight Saving Time adjustments, configurable via the web UI.
*   **Complete Web Interface**:
    *   **Web Configuration Portal**: Configure all settings easily from any device on your network via a web browser at `http://bttf-clock.local`.
    *   **WiFi Manager**: Simple initial WiFi setup using a captive portal. No need to hardcode credentials.
*   **Advanced Alarm System**:
    *   **Configurable Alarm**: Set alarm time, toggle on/off, and configure snooze duration from the web UI or physical buttons.
    *   **Snooze Functionality**: Snooze the alarm with a press of a button.
*   **Authentic Experience**:
    *   **Sound Effects**: Plays iconic sounds from the movie for alarms and animations using a DFPlayer Mini MP3 module.
    *   **Time Travel Animations**: Watch the displays go haywire and the speedometer hit 88 MPH at configurable intervals!
*   **Customization & Convenience**:
    *   **Power Saving Mode**: Displays automatically turn off during user-defined "sleep" hours to save power and avoid nighttime glare.
    *   **Over-The-Air (OTA) Updates**: Update the firmware wirelessly without needing a physical connection.
    *   **Web UI Themes**: Change the color scheme of the web interface to match your favorite BTTF aesthetic.
    *   **Physical Button Controls**: Full control over core functions without needing the web UI.
    ## Hardware Requirements

This project requires a handful of common electronics components. Below is a detailed Bill of Materials (BOM) for everything you'll need to assemble the clock.

## Bill of Materials (BOM)

| Component                    | Quantity | Notes                                                                                               |
| ---------------------------- | :------: | --------------------------------------------------------------------------------------------------- |
| **[ESP32 Dev Module](https://www.aliexpress.com/item/1005004571486357.html?spm=a2g0o.order_list.order_list_main.105.77ff1802BW3kIe)**         |    1     | The core of the project. A 30-pin or 38-pin version will work.                                      |
| [TM1637 7-Segment Display](https://www.aliexpress.com/item/1005001582129952.html)     |    3     | 4-digit displays for Day, Year, and Time.                                                           |
| [Adafruit AlphaNum4 Display](https://www.aliexpress.com/item/1005001593666162.html)   |    1     | 14-segment I2C display for showing the month. Any HT16K33-based alphanumeric display should work.   |
| [DFPlayer Mini MP3 Module](https://www.aliexpress.com/item/1005008228039985.html)     |    1     | For playing sound effects.                                                                          |
| [MicroSD Card](https://www.aliexpress.com/item/1005008978876553.html)                 |    1     | 10MB or larger, formatted to FAT16 or FAT32. For storing MP3 files.                                  |
| [20mm diameter Speaker](https://www.aliexpress.com/item/1005006682079525.html)                      |    1     | A small 8 Ohm speaker (e.g., 0.5W or 1W) for sound output.                                          |
| [Lighted Tactile Push Buttons](https://www.aliexpress.com/item/1005007163972572.html) |    2     | For the Set/Stop and Sound/Toggle functions. The built-in LED replaces a separate status LED.       |
| [Tactile Push Buttons](https://www.aliexpress.com/item/1005002622143638.html)         |    2     | For the Hour and Minute functions.                                                                  |
| [5mm LEDs](https://www.aliexpress.com/item/1005003912454852.html)                     |    2     | For the AM/PM indicators.                                                                           |
| [10kΩ Resistors](https://www.aliexpress.com/item/1005007447212056.html)               |    4     | Used as pull-down resistors for the push buttons to ensure stable readings.                         |
| [220Ω Resistors](https://www.aliexpress.com/item/1005007447212056.html)               |    2     | Current-limiting for the AM/PM LEDs. Lighted buttons may have built-in resistors; check specs.      |
| [Breadboard](https://www.aliexpress.com/item/1005007085965483.html) & [Jumper Wires](https://www.aliexpress.com/item/1005007298861842.html)    |    1     | For prototyping and making connections. A half-size or full-size breadboard is recommended.         |
| 5V Power Supply              |    1     | A reliable power supply capable of at least 1A (e.g., a standard USB phone charger and cable).      |

## Software & Libraries

This project is built using the Arduino framework for the ESP32. You will need to install the following libraries through the Arduino IDE Library Manager or PlatformIO:

| Library                       | Author              | Purpose                               |
| ----------------------------- | ------------------- | ------------------------------------- |
| `WiFiManager`                 | tzapu               | For the WiFi connection portal.       |
| `Adafruit GFX Library`        | Adafruit            | Core graphics library.                |
| `Adafruit LED Backpack Library` | Adafruit            | Drives the AlphaNum4 display.         |
| `DFRobotDFPlayerMini`         | DFRobot             | Controls the MP3 player module.       |
| `ESPAsyncWebServer`           | me-no-dev           | Hosts the web configuration interface.|
| `AsyncTCP`                    | me-no-dev           | Required by ESPAsyncWebServer.        |
| `ArduinoJson`                 | Benoit Blanchon     | Handles data for the web API.         |
| `TM1637`                      | Avishay Orpaz       | Drives the 7-segment displays.        |
## Wiring Guide

This guide provides a detailed overview of how to connect all components to the ESP32. It's highly recommended to assemble the circuit on a breadboard first to test all connections before soldering.

<p align="center">
  <img src="circuit.jpg" alt="Wiring Schematic" width="800">
</p>

### Power Distribution

All components require a connection to power (VCC) and ground (GND).
*   **5V Power**: Connect the **VIN** pin of the ESP32 to the positive (5V) rail of your power source. Connect the VCC pins of all three TM1637 displays, the Adafruit AlphaNum4 display, and the DFPlayer Mini to this 5V rail.
*   **Ground**: Connect a **GND** pin from the ESP32 to the common ground rail. Connect the GND pins of all displays, the DFPlayer Mini, all buttons, and the negative (cathode) side of all LEDs (via their current-limiting resistors) to this ground rail.
### Display Connections

The clock uses four separate displays that are controlled differently.

#### TM1637 7-Segment Displays
These three displays share a common clock (CLK) line to save pins, but each has a unique data (DIO) line.

| Display Function            | Pin | ESP32 GPIO |
| --------------------------- |:---:| :--------: |
| **All TM1637s (Shared)**    | CLK |    13    |
| Day of Month                | DIO |    18    |
| Year                        | DIO |    15    |
| Time (HH:MM)                | DIO |    14    |

#### Adafruit AlphaNum4 Alphanumeric Display
This display uses the I2C communication protocol.

| Component                 | Pin | ESP32 GPIO |
| ------------------------- |:---:| :--------: |
| **Adafruit AlphaNum4 (I2C)** | SDA |    21    |
|                           | SCL |    22    |
### Audio Connections

The DFPlayer Mini module communicates with the ESP32 using a serial (UART) connection.

| Component        | Pin | ESP32 GPIO | Notes                                  |
| ---------------- |:---:| :--------: | -------------------------------------- |
| **DFPlayer Mini**| RX  |  17 (TXD2) | Connects to the ESP32's **TX** pin.    |
|                  | TX  |  16 (RXD2) | Connects to the ESP32's **RX** pin.    |
|                  | SPK_1 | Speaker +  | Connect one speaker wire here.         |
|                  | SPK_2 | Speaker -  | Connect the other speaker wire here.   |

### Button & LED Connections

The buttons require pull-down resistors (e.g., 10kΩ) to prevent floating inputs. The LEDs require current-limiting resistors (e.g., 220-330Ω) to prevent them from burning out.

| Component          | ESP32 GPIO | Notes                                  |
| ------------------ | :--------: | -------------------------------------- |
| Set/Stop Button    |     34     | Connect one side to 3.3V and the other to the GPIO pin. Add a 10kΩ pull-down resistor from the GPIO pin to GND. |
| Set/Sound Button   |     4      | Connect one side to 3.3V and the other to the GPIO pin. Add a 10kΩ pull-down resistor from the GPIO pin to GND. |
| Hour Button        |     33     | Connect one side to 3.3V and the other to the GPIO pin. Add a 10kΩ pull-down resistor from the GPIO pin to GND. |
| Minute Button      |     32     | Connect one side to 3.3V and the other to the GPIO pin. Add a 10kΩ pull-down resistor from the GPIO pin to GND. |
| **AM LED**         |     27     | Connect the anode (+) to this pin. Connect cathode (-) to GND via a 220-330Ω resistor. |
| **PM LED**         |     12     | Connect the anode (+) to this pin. Connect cathode (-) to GND via a 220-330Ω resistor. |
| Set/Stop LED       |     26     | Connect the anode (+) to this pin. Connect cathode (-) to GND via a 220-330Ω resistor. |
| Set/Sound LED      |     2      | Connect the anode (+) to this pin. Connect cathode (-) to GND via a 220-330Ω resistor. |
## Installation & Setup

### 1. 3D Print the Housing

A custom-designed housing is available to give your clock an authentic, finished look. The 3D model is provided as a Bambu Studio project file (`bambu studio clock.3mf`).

This project's housing is based on the incredible "Back to the Future Single Time Circuit" model by TerryB on MakerWorld. A huge thank you to to him for his original work and the mods he made at my request to the files! You can find the original model [here](https://makerworld.com/en/models/1154106-back-to-the-future-single-time-circuit#profileId-1537667).

*   **File**: `bambu studio clock.3mf`
*   **Recommended Filament**: For a realistic metallic look, use a metallic grey or silver PLA/PETG filament.
*   **Build Plate**: Printing on a patterned or textured PEI build plate can enhance the metallic effect and give the surface a unique finish.
*   **Infill**: Use an infill of **50% or higher** to ensure the housing is sturdy and durable.

### 2. Hardware Assembly

Wire all the components together as described in the **Wiring Guide**. Double-check all your connections, especially power and ground, before applying power.

### 3. Prepare the SD Card

1.  Format a MicroSD card to **FAT16** or **FAT32**.
2.  Create a folder named `mp3` in the root of the SD card.
3.  Copy your sound effect files into the `mp3` folder. The files must be named in a specific four-digit format: `0001.mp3`, `0002.mp3`, etc.
    *   **Important**: Due to how the DFPlayer Mini module indexes files, you must copy the files to the SD card **one by one, in numerical order**. Start with `0001.mp3`, then `0002.mp3`, and so on. Do not copy them all at once, as this can cause the wrong sounds to play.
    *   `0002.mp3`: Time travel / primary alarm sound
    *   `0008.mp3`: "Power of Love" easter egg sound
    *   `0010.mp3`: Sleep mode activation sound
    *   `0013.mp3`: "On" or positive confirmation sound
    *   `0014.mp3`: "Off" or negative confirmation sound
    *   ... and other random sounds for the alarm (`0001.mp3` - `0009.mp3`)
4.  Insert the SD card into the DFPlayer Mini module.

### 4. Flash the Firmware

1.  **Install Your Development Environment**: You can compile and upload this sketch using either the traditional **Arduino IDE** or **Visual Studio Code** with the [Arduino Maker Workshop](https://marketplace.visualstudio.com/items?itemName=TheLastOutpostWorkshop.arduino-maker-workshop) extension. After setting up your chosen IDE, make sure to install the ESP32 board definitions.
2.  **Install Libraries**: Using the Library Manager in your IDE, install all the libraries listed in the [Software & Libraries](#software--libraries) section.
3.  **Configure Default Time Zone (Optional)**: The time zone can be set from the web interface later. However, to set the initial default, open the `.ino` file and find the `timezoneString` in the `defaultSettings` struct. Change it to match your local time zone. You can find a list of valid POSIX TZ strings online.
4.  **Upload Code**: Select your ESP32 board and the correct COM port, then upload the sketch.
### 5. WiFi Configuration

1.  On the first boot (or after resetting WiFi credentials), the clock will create its own WiFi network with the SSID `bttf-clock`.
2.  Connect to this network with your phone or computer.
3.  A captive portal page should automatically open in your browser. If it doesn't, navigate to `192.168.4.1`.
4.  Click "Configure WiFi", select your home WiFi network from the list, and enter your password.
5.  Click "Save". The clock will restart and automatically connect to your network.

## Usage

### Web Interface

Once connected to your WiFi, you can access the full configuration panel by navigating to **http://bttf-clock.local/** in your web browser.

<p align="center">
  <img src="webui.png" alt="Web UI">
</p>

From here, you can control all aspects of the clock.
#### Time Circuit Settings
*   **Destination (Alarm) Time**: Use the sliders to set the hour and minute for your alarm.
*   **Alarm On/Off**: A toggle switch to enable or disable the alarm. The `SET/STOP` LED on the clock will light up when the alarm is enabled.
*   **Snooze Time**: A slider to set the snooze duration in minutes (1-59).

#### Departure/Arrival (Sleep) Times
*   These settings define a "sleep" period where the displays will turn off to save power and prevent light disturbance at night.
*   **Departure**: The time the displays will turn off.
*   **Arrival**: The time the displays will turn back on.

#### Display & Sound Settings
*   **Display Brightness**: Adjusts the brightness of all LED displays.
*   **Notification Volume**: Sets the volume for all sound effects from the DFPlayer Mini.
*   **Time Travel Animation Every (min, 0=Off)**: Sets how often (in minutes) the "time travel" animation plays. Set to 0 to disable it.
*   **Time Travel Sound FX On**: A toggle to enable or disable all sound effects, including the alarm. The `SET/SOUND` LED on the clock will light up when sound is enabled.
*   **Power of Love (Flux Capacitor)**: A toggle that triggers the "Power of Love" easter egg sound. This is a one-time trigger and will turn itself off after being activated.
*   **Theme**: A dropdown to change the color scheme of the web interface.

#### Time Settings
*   **Timezone**: Select your local timezone from the dropdown list. This is crucial for correct time display and automatic Daylight Saving Time adjustments.
*   **24 Hour Format**: A toggle to switch the time display between 24-hour format (e.g., 14:30) and 12-hour AM/PM format (e.g., 02:30 PM).

#### Device Management
*   **Save All Settings**: Saves all your changes to the ESP32's memory.
*   **Reset to Default Settings**: Resets all clock settings to their original values and reboots the device.
*   **Reset WiFi Credentials**: Clears the saved WiFi network, reboots the device, and puts it back into AP mode for new WiFi configuration.
### Physical Buttons

The clock can also be operated using the four physical buttons for core functions.

*   **SET/STOP Button**:
    *   **Single Press**: Toggles the main alarm On or Off. A confirmation sound will play, and the corresponding LED will turn on or off.
    *   **Press and Hold**: Enters alarm setting mode. The time display will show the current alarm time. While holding this button, use the **HOUR** and **MINUTE** buttons to adjust the alarm time. Release the button to save the new alarm time.
    *   **During Alarm**: Stops the currently sounding alarm.

*   **SET/SOUND Button**:
    *   **Single Press**: Toggles all sound effects On or Off. A confirmation sound will play, and the corresponding LED will turn on or off.

*   **HOUR Button**:
    *   **When Idle**: Decreases the sound effect volume by one step.
    *   **In Alarm Setup Mode**: Increments the alarm hour.
    *   **During Alarm**: Activates the **Snooze** function, silencing the alarm for the configured snooze duration.

*   **MINUTE Button**:
    *   **When Idle**: Increases the sound effect volume by one step.
    *   **In Alarm Setup Mode**: Increments the alarm minute.
    *   **During Alarm**: Activates the **Snooze** function.

*   **Easter Egg**:
    *   Press the **HOUR** and **MINUTE** buttons simultaneously to trigger the "Power of Love" sound effect!

---

## License

```text
MIT License

Copyright (c) 2025 Randall North

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Acknowledgements

This project wouldn't be possible without the hard work of the open-source community and the creators of the fantastic libraries used. Special thanks to the teams behind the ESP32 Arduino core, WiFiManager, and all the Adafruit and DFRobot libraries. A huge thank you to TerryB on MakerWorld for the original 3D model design and for making modifications upon request.
