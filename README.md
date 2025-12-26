# bar-scan-bt
BT adapter for GM67 bar code scanner

Its using ESP32 and https://github.com/enspectr/ESP32-BLE-Keyboard for communicating with the host.

## Wiring

The following figure shows connections to ESP32C3Zero module used as a controller. Pressing the START button starts scanning.
Scanning completes either after 5 seconds or earlier if the code was scanned successfully.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/wiring.gif" />
</p>

## Building

Use Arduino to compile and flash https://github.com/enspectr/bar-scan-bt/blob/main/BarScanBLE/BarScanBLE.ino
To be able to build this code add the following to Arduino Additional board manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then go to Boards Manager and install **esp32 by Espressif Systems**. The code is tested with version 3.0.7.
The implementation is using NimBLE stack https://github.com/h2zero/NimBLE-Arduino

## First steps

The scanner module factory configuration is continuous scan emulating USB keyboard. So it will not work with ESP32
controller out of the box. There are two control codes that should be scanned once to properly configure GM67 module.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/host_mode.jpg" />
</p>

After switching to host mode the scanner stops continuous scan. Use button on the module board to scan the following code.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/serial_interface.jpg" />
</p>

## Testing

The adapter presents scanner as BT keyboard with name started with **EScan** followed by a unique suffix made up of 6 hexadecimal digits.
Once scanned, the code will be entered using this virtual keyboard at the place where the input focus is currently located.
To check how it works one may scan the following figure and compare result with the code at the bottom of the figure.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/test_qrcode.jpg" />
</p>

## LED indicator functions

The adapter uses on-board RGB LED to indicate its operational status as described in the following table.

|   Color       |   Meaning                            |
|---------------|--------------------------------------|
| Red           | Adapter is starting or resetting     |
| Yellow        | Adapter is not connected to the host |
| Blue          | Adapter is connected to the host     |
| Magenta pulse | Scanning started                     |
| Green pulse   | Scanning completed                   |
| Cyan pulse    | Control code scanned                 |

## Notes on code integrity and optional checksums

The bar-code scanner uses virtual keyboard to transfer code scanned to host computer. Unfortunately the low energy Bluetooth is inherently unreliable. So its possible that some symbols may be lost in transit and not be received by the host. If the host does not validate bar-code it has no means to detect code corruption.

The scanner has an option to append checksum to the code scanned so that the host will be able to validate it and detect code corruption in transit. The checksum represents the sum of all ASCII codes in the scanned text by modulo 4096. Its encoded as 2 digit number using *[base64 alphabet](https://en.wikipedia.org/wiki/Base64#Alphabet)* and appended to the end of the scanned text.

To enable checksums one should scan the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/csum_on.png" />
</p>

Once enabled checksums will always be used even after power cycling. To disable checksums one should scan the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/csum_off.png" />
</p>

## Troubleshooting

If things go wrong one can try the following steps to recover:

### Reset adapter

To reset the adapter, press the start button and hold it for more than 1.5 seconds.

### Reconnect adapter to the host

Sometimes scan codes are not being delivered to the host while the connection is established. To root cause of this problem is unknown. Disconnect adapter and connect to it again to recover.

### Reset adapter to factory defaults

This is the method to be used if nothing else have helped. To reset scanner to factory defaults using the following control code and repeat configuration procedure.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/default_settings.jpg" />
</p>

### Finding adapter version

If some features don't work the first step to investigate the root cause is finding device software version. Below is a special control code that can be scanned to find out the firmware version and build date.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/show_version.png" />
</p>

## Power consumption

Total consumption from 5V source together with GM67 module is 60mA in idle state, 240mA while scanning.
