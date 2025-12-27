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

## Initial configuration steps

The scanner module factory configuration is continuous scan emulating USB keyboard. So it will not work with ESP32
controller out of the box. There are two control codes that should be scanned once to properly configure GM67 module.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/host_mode.jpg" />
</p>

After switching to host mode the scanner stops continuous scan. Use button on the module board to scan the following code.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/serial_interface.jpg" />
</p>

## Operation

The adapter presents scanner as BT keyboard with name started with **EScan** followed by a unique suffix made up of 6 hexadecimal digits.
Once scanned, the code will be entered using this virtual keyboard at the place where the input focus is currently located.

Right after powering on the adapter starts in standby mode to save power (if its not [disabled explicitly](#disabling-standby)). In this mode the BT is not turned on so the connection to the host computer is impossible. To turn on BT one should press start button once. In case there is no connection to the host for more than 5 minutes the adapter switches to standby mode automatically.

Pressing start button while the adapter is not connected to the host has no effect.

## Testing

To check if adapter works one may scan the following figure and compare result with the code at the bottom of the figure.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/test_qrcode.jpg" />
</p>

Another quick test is to [query adapter version](#finding-adapter-version).

## LED indicator functions

The adapter uses on-board RGB LED to indicate its operational status as described in the following table.

|   LED Color   |   Meaning                            |
|---------------|--------------------------------------|
| LED off       | Standby mode                         |
| Red           | Adapter is starting or resetting     |
| Yellow        | Adapter is not connected to the host |
| Blue          | Adapter is connected to the host     |
| Magenta pulse | Scanning started                     |
| Green pulse   | Scanning completed                   |
| Cyan pulse    | Control code scanned                 |

## Advanced configuration

### Code integrity and optional checksums

The bar-code scanner uses virtual keyboard to transfer code scanned to host computer. Unfortunately the low energy Bluetooth is inherently unreliable. So its possible that some symbols may be lost in transit and not be received by the host. If the host does not validate bar-code it has no means to detect code corruption.

The scanner has an option to append checksum to the code scanned so that the host will be able to validate it and detect code corruption in transit. The checksum represents the sum of all ASCII codes in the scanned text by modulo 4096. Its encoded as 2 digit number using *[base64 alphabet](https://en.wikipedia.org/wiki/Base64#Alphabet)* and appended to the end of the scanned text following the ~ separator. The separator allows the host to determine whether the scanned code has a checksum or not, provided that the ~ character is not used in the codes themselves.

To enable checksums one should scan the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/csum_on.png" />
</p>

Once enabled checksums will always be used even after power cycling. To disable checksums one should scan the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/csum_off.png" />
</p>

### Disabling standby

The standby mode provides more than 2x power saving while the scanner is idle. Yet having to wake it up from standby may be inconvenient if its used frequently. User can disable standby mode by scanning the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/standby_dis.png" />
</p>

Once disabled standby mode will not be used even after power cycling. To enable it back one should scan the following control code:

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/standby_en.png" />
</p>

## Troubleshooting

If things go wrong one can try the following steps to recover:

### Reset adapter

To reset the adapter, press the start button and hold it for more than 1.5 seconds. It will restart in standby mode (if its not [disabled explicitly](#disabling-standby)).

### Reconnect adapter to the host

Sometimes scan codes are not being delivered to the host while the connection is established. The root cause of this problem is unknown. Disconnect adapter and connect to it again to recover.

### Reset adapter to factory defaults

This is the method to be used if nothing else have helped. To reset scanner to factory defaults using the following control code and repeat [initial configuration procedure](#initial-configuration-steps).

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/default_settings.jpg" />
</p>

### Finding adapter version

If some features don't work the first step to investigate the root cause is finding device software version. Below is a special control code that can be scanned to find out the firmware version and build date.

<p align="center">
  <img src="https://github.com/enspectr/bar-scan-bt/blob/main/doc/show_version.png" />
</p>

## Power consumption

The following table summarize power consumption from 5V source in various operation modes.

| Operation mode  | BT adapter consumption | GM67 module consumption | Total consumption |
|-----------------|------------------------|-------------------------|-------------------|
| Standby         | 7mA                    | 18mA                    | 25mA              |
| Idle            | 42mA                   | 18mA                    | 60mA              |
| Scanning        | 42mA                   | ~200mA                  | ~242mA            |
| Scanning with collimation off | 42mA     | ~185mA                  | ~227mA            |

Turning off collimation light beam (red flashing strip) provides marginal power saving in scanning mode. Consult [GM67 User manual](https://github.com/enspectr/bar-scan-bt/blob/main/doc/GM67_Barcode.pdf) to see how to do it.
