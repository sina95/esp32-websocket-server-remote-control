# Firmware for ESP32 with WebSocket Server, Mailing Service, and Remote Control

This firmware enables the ESP32 to perform the following functions:
- **Regular Reporting:** The ESP32 sends status reports to the server every few hours to confirm its activity and operational status.
- **Real-Time Commands:** A manual switch on the ESP32 allows real-time command sending to client devices via WebSocket.
- **Unlimited Clients:** The system can support an unlimited number of client devices.
- **Secure Connectivity:** Password authentication for WebSocket connections is supported, even in isolated networks.
- **Firmware Updates:** The firmware includes a feature for remote firmware updates.

## Installation Procedure

1. **Clone the Repository:**
   Clone the repository to your local environment.

2. **Install Arduino IDE:**
   If you haven't installed the Arduino IDE, download and install it from [this link](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE).

3. **Configure Settings:**
   - Update WiFi network credentials and other configuration variables according to your needs.
   - Set up an IFTTT account for handling mail events.

4. **Connect Hardware and Upload Code:**
   - Open the Arduino IDE and compile the code for the ESP32 hardware board.
   - Upload the compiled code to the ESP32.

Happy testing! :)
