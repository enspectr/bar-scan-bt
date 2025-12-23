# bar-scan-bt
BT adapter for GM67 bar code scanner

Its using ESP32 and https://github.com/enspectr/ESP32-BLE-Keyboard for communicating with the host.

## Wiring

The following figure shows connections to ESP32C3Zero module used as a controller. Pressing the START button starts scanning.
Scanning completes either after 5 seconds or earlier if the code was scanned successfully.

![The ESP32C3Zero module connections](https://github.com/enspectr/bar-scan-bt/blob/main/doc/wiring.gif)

## Building

Use Arduino to compile and flash https://github.com/enspectr/bar-scan-bt/blob/main/BarScanBLE/BarScanBLE.ino

## First steps

The scanner module factory configuration is continuous scan emulating USB keyboard. So it will not work with ESP32
controller out of the box. There are two control codes that should be scanned once to properly configure GM67 module.
Use button on the module board to scan the following codes.

![Switch to host mode control code](https://github.com/enspectr/bar-scan-bt/blob/main/doc/host_mode.jpg)

![Switch to RS232 control code](https://github.com/enspectr/bar-scan-bt/blob/main/doc/serial_interface.jpg)

## Testing

## Power consumption
Total consumption from 5V source together with GM67 module is 60mA in idle state, 240mA while scanning.

## Troubleshooting

If things go wrong try to reset scanner to factory defaults using the following control code and repeat configuration procedure.

![Reset settings to factory defaults](https://github.com/enspectr/bar-scan-bt/blob/main/doc/default_settings.jpg)
