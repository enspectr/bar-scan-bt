//
// Start barcode scanning on button press,
// get barcode from GM67 reader and print it to USB CDC
// and transmit to the connect host as BLE keyboard
//

#include <BleKeyboard.h>
#include <esp_mac.h>
#include <esp_bt.h>

// Comment it out to keep default power level
#define TX_PW_BOOST ESP_PWR_LVL_P15

#define BTN_PIN 0
#define BTN_DEBOUNCE_TOUT 50

#define BarcodeSerial Serial1
#define TX_PIN 1
#define RX_PIN 3
#define BAUD_RATE 9600

#define BARCODE_NOP  0x00
#define START_DECODE 0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14
#define START_SCAN5S 0x08, 0xC6, 0x04, 0x08, 0x00, 0xF2, 0xFA, 0x05, 0xFD, 0x35

static const byte wakeUp[]   = {BARCODE_NOP};
static const byte startCmd[] = {START_DECODE, BARCODE_NOP, START_SCAN5S};
static bool scan_inited;

#define BARCODER_WRITE(cmd) BarcodeSerial.write(cmd, sizeof(cmd))

// #define DUMP_HEX

#define DEV_NAME "EScan"

static BleKeyboard bleKeyboard(DEV_NAME);

static inline char hex_digit(uint8_t v)
{
    return v < 10 ? '0' + v : 'A' + v - 10;
}

void setup()
{
	Serial.begin(BAUD_RATE);
	BarcodeSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
	BarcodeSerial.setTimeout(10);

	pinMode(BTN_PIN, INPUT_PULLUP);

	uint8_t mac[8] = {0};
	if (ESP_OK == esp_efuse_mac_get_default(mac))
	{
		std::string bt_dev_name(DEV_NAME);
		uint8_t sig[] = {(uint8_t)(mac[0] ^ mac[3]), (uint8_t)(mac[1] ^ mac[4]), (uint8_t)(mac[2] ^ mac[5])};
		bt_dev_name += hex_digit(sig[0] >> 4);
		bt_dev_name += hex_digit(sig[0] & 0xf);
		bt_dev_name += hex_digit(sig[1] >> 4);
		bt_dev_name += hex_digit(sig[1] & 0xf);
		bt_dev_name += hex_digit(sig[2] >> 4);
		bt_dev_name += hex_digit(sig[2] & 0xf);
		bleKeyboard.set_device_name(bt_dev_name);
	}
	bleKeyboard.begin();
#ifdef TX_PW_BOOST
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, TX_PW_BOOST);
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     TX_PW_BOOST);
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,    TX_PW_BOOST);
#endif
}

static bool readBtn(void)
{
	static bool btn_pressed;
	static unsigned last_pressed;
	if (!digitalRead(BTN_PIN)) {
		last_pressed = millis();
		btn_pressed = true;
	} else if (btn_pressed && millis() - last_pressed > BTN_DEBOUNCE_TOUT) {
		btn_pressed = false;
	}
	return btn_pressed;
}

static void wait(unsigned msec)
{
	unsigned const start_ts = millis();
	static char buff[256];

	while (millis() - start_ts < msec) {
		unsigned sz = BarcodeSerial.readBytes(buff, sizeof(buff));
		for (unsigned i = 0; i < sz; ++i) {
#ifdef DUMP_HEX
			static char buf[5] = {};
			unsigned const n = snprintf(buf, sizeof(buf)-1, "%02x ", buff[i]);
			Serial.write(buf, n);
#else
			static char last_byte;
			char c = buff[i];
			if (scan_inited) {
				Serial.write(c);
				if (bleKeyboard.isConnected()) {
					if (c != 0xA && c != 0xD)
						bleKeyboard.write(c);
					else if (c == 0xA) {
						bleKeyboard.press(KEY_RETURN);
						delay(30);
						bleKeyboard.release(KEY_RETURN);
					}
				}
			} else if (last_byte == 0xff && c == 0x28)
				scan_inited = true;
			last_byte = c;
#endif
		}
	}
}

static inline void start_scan(void)
{
	scan_inited = false;
#ifdef DUMP_HEX
	Serial.write('\n');
#endif
	BARCODER_WRITE(wakeUp);
	wait(50);
	BARCODER_WRITE(startCmd);
}

void loop() {
	static bool btn_pressed;

	bool const pressed = readBtn();
	if (pressed && !btn_pressed)
		start_scan();
	btn_pressed = pressed;

	wait(10);
}
