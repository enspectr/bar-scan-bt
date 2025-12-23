//
// Start barcode scanning on button press,
// get barcode from GM67 reader and print it to USB CDC
// and transmit to the connect host as BLE keyboard
//

#include <BleKeyboard.h>
#include <esp_mac.h>

#define BTN_PIN 0
#define BTN_DEBOUNCE_TOUT 50

#define BarcodeSerial Serial1
#define TX_PIN 1
#define RX_PIN 3
#define BAUD_RATE 9600

#define BARCODE_NOP  0x00
#define AUTO_LIGHT   0x08, 0xC6, 0x04, 0x08, 0x00, 0xF2, 0x02, 0x00, 0xFE, 0x32
#define START_DECODE 0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14
#define START_SCAN5S 0x08, 0xC6, 0x04, 0x08, 0x00, 0xF2, 0xFA, 0x05, 0xFD, 0x35

static const byte wakeUp[]   = {BARCODE_NOP};
static const byte startCmd[] = {AUTO_LIGHT, BARCODE_NOP, START_DECODE, BARCODE_NOP, START_SCAN5S};
static bool scan_inited;

#define BARCODER_WRITE(cmd) BarcodeSerial.write(cmd, sizeof(cmd))

// #define DUMP_HEX

#define DEV_NAME          "EScan"
#define DEV_NAME_SUFF_LEN 4

static BleKeyboard bleKeyboard;

static inline char hex_digit(uint8_t v)
{
    return v < 10 ? '0' + v : 'A' + v - 10;
}

static inline char byte_signature(uint8_t v)
{
    return hex_digit((v & 0xf) ^ (v >> 4));
}

void setup()
{
	Serial.begin(BAUD_RATE);
	BarcodeSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
	BarcodeSerial.setTimeout(10);

	pinMode(BTN_PIN, INPUT_PULLUP);

	std::string bt_dev_name(DEV_NAME);
#ifdef DEV_NAME_SUFF_LEN
	uint8_t mac[8] = {0};
	if (ESP_OK == esp_efuse_mac_get_default(mac)) {
		for (int i = 0; i < DEV_NAME_SUFF_LEN && i < sizeof(mac); ++i)
			bt_dev_name += byte_signature(mac[i]);
	}
#endif
	bleKeyboard.set_device_name(bt_dev_name);
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
				if (bleKeyboard.isConnected())
					bleKeyboard.write(c);
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
