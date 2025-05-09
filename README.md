# ESP32 WiFi Penetration Tool with OLED Display

This is a modified version of the [ESP32 WiFi Penetration Tool](https://github.com/risinek/esp32-wifi-penetration-tool) originally created by [@risinek](https://github.com/risinek).

## Hardware Modifications
This fork adds hardware improvements to the original project:
- Added OLED display (SSD1306) for direct status monitoring
- Integrated physical buttons for easier control
- Hardware interface for direct interaction without web interface
- Standalone operation capability without smartphone connection

## Original Features
- PMKID capture
- WPA/WPA2 handshake capture and parsing
- Deauthentication attacks using various methods
- Denial of Service attacks
- Formatting captured traffic into PCAP format
- Parsing captured handshakes into HCCAPX file ready to be cracked by Hashcat
- Passive handshake sniffing
- Easily extensible framework for new attacks implementations
- Management AP for easy configuration on the go using smartphone

## Usage
1. Build and flash project onto ESP32 (DevKit or module)
2. Power ESP32
3. Use either:
   - OLED display and buttons for direct control
   - Web interface (connect to AP "ManagementAP" with password "mgmtadmin" and open 192.168.4.1)

## Build
This project is developed using ESP-IDF 4.1. Build using:
```shell
idf.py build
```

## Hardware Requirements
- ESP32 Development Board: 0.96" ESP32 ESP-32D WIFI Bluetooth Development Board OLED CH340C Module Type-C
- SSD1306 OLED Display (integrated on the board)
- Push buttons:
  - Red button: GPIO 12
  - Green button: GPIO 13
  - Boot button: GPIO 0 (Select button)
- Optional: Battery pack for portable operation

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Credits
- Original project by [@risinek](https://github.com/risinek)
- Original repository: [esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool)
- Modified by [@ralfs66](https://github.com/ralfs66)

## Disclaimer
This project demonstrates vulnerabilities of Wi-Fi networks and its underlying 802.11 standard and how ESP32 platform can be utilized to attack on those vulnerable spots. Use responsibly against networks you have permission to attack on.