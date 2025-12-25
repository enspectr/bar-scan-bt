//
// Start barcode scanning on button press,
// get barcode from GM67 reader and print it to USB CDC
// and transmit to the connect host as BLE keyboard
//

#include <BleKeyboard.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <esp_bt.h>

// Comment it out to disable LED
#define RGB_LED 10
#ifdef RGB_LED
#include <Adafruit_NeoPixel.h>
#endif

// Comment it out to keep default power level
#define TX_PW_BOOST ESP_PWR_LVL_P18

#define BTN_PIN 0
#define BTN_DEBOUNCE_TOUT 50
#define BTN_LONG_PRESS_TOUT 1500

#define BarcodeSerial Serial1
#define TX_PIN 1
#define RX_PIN 3
#define BAUD_RATE 9600

#define BARCODE_NOP  0x00
#define START_DECODE 0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14
#define START_SCAN5S 0x08, 0xC6, 0x04, 0x08, 0x00, 0xF2, 0xFA, 0x05, 0xFD, 0x35
#define BARCODER_WRITE(cmd) BarcodeSerial.write(cmd, sizeof(cmd))

static unsigned boot_ts;

static const byte barcoder_wake_up[] = {BARCODE_NOP};
static const byte barcoder_start[] = {START_DECODE, BARCODE_NOP, START_SCAN5S};

static bool scan_inited, scan_done;
static String scan_buff;
static const String cmd_chsum_on ("jMRMf549y172QLpp");
static const String cmd_chsum_off("jMRMf549y172QLpq");
static bool scan_csum_on;

#ifdef RGB_LED
Adafruit_NeoPixel pixels(1, RGB_LED, NEO_GRB + NEO_KHZ800);
#define RGB_(color) pixels.Color(color)
#else
#define RGB_(color) 0
#endif

#define LED_BRIGHTNESS 2
#define LED_BRIGHTNESS_HIGH 16
#define RED_(br)     br, 0, 0
#define GREEN_(br)   0, br, 0
#define BLUE_(br)    0, 0, br
#define YELLOW_(br)  br, br/2, 0
#define MAGENTA_(br) br/2, 0, br/2
#define CYAN_(br)    0, br/2, br/2
#define RGB_RED      RGB_(RED_(LED_BRIGHTNESS))
#define RGB_BLUE     RGB_(BLUE_(LED_BRIGHTNESS))
#define RGB_YELLOW   RGB_(YELLOW_(LED_BRIGHTNESS))
#define RGB_HMAGENTA RGB_(MAGENTA_(LED_BRIGHTNESS_HIGH))
#define RGB_HRED     RGB_(RED_(LED_BRIGHTNESS_HIGH))
#define RGB_HGREEN   RGB_(GREEN_(LED_BRIGHTNESS_HIGH))
#define RGB_HCYAN    RGB_(CYAN_(LED_BRIGHTNESS_HIGH))

// #define DUMP_HEX

#define DEV_NAME "EScan"

static BleKeyboard bleKeyboard(DEV_NAME);

static inline char hex_digit(uint8_t v)
{
    return v < 10 ? '0' + v : 'A' + v - 10;
}

static inline void led_show_color(uint32_t c)
{
#ifdef RGB_LED
	static uint32_t last_color = ~0;
	if (c != last_color) {
		pixels.setPixelColor(0, c);
		pixels.show();
	}
#endif
}

void setup()
{
	boot_ts = millis();

	Serial.begin(BAUD_RATE);
	BarcodeSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
	BarcodeSerial.setTimeout(10);

	pinMode(BTN_PIN, INPUT_PULLUP);

#ifdef RGB_LED
	pixels.begin();
	led_show_color(RGB_HRED);
	delay(50);
#endif

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
#endif
}

static void reset_self(void)
{
	// Bright red pulse indicates reset
	led_show_color(RGB_HRED);
	esp_restart();
}

static void long_press_handler(void)
{
	reset_self();
}

static bool readBtn(void)
{
	static bool btn_pressed;
	static unsigned first_pressed, last_pressed;
	if (!digitalRead(BTN_PIN)) {
		last_pressed = millis();
		if (!btn_pressed) {
			first_pressed = last_pressed;
			btn_pressed = true;
		} else if (last_pressed - first_pressed > BTN_LONG_PRESS_TOUT) {
			long_press_handler();
		}
	} else if (btn_pressed && millis() - last_pressed > BTN_DEBOUNCE_TOUT) {
		btn_pressed = false;
	}
	return btn_pressed;
}

static inline char b64symbol(uint8_t code)
{
	uint8_t const LETTERS = 'Z' - 'A' + 1;
	if (code < LETTERS)
		return 'A' + code;
	if (code < 2*LETTERS)
		return 'a' + code - LETTERS;
	if (code < 2*LETTERS + 10)
		return '0' + code - 2*LETTERS;
	if (code == 2*LETTERS + 10)
		return '+';
	if (code == 2*LETTERS + 11)
		return '/';
	return 0; // Invalid code
}

static void append_csum(String& s)
{
	unsigned sum = 0;
	for (unsigned i = 0; i < s.length(); ++i)
		sum += (unsigned char)s[i];
	unsigned const b64mask = ((1 << 6) - 1);
	s += b64symbol((sum >> 6) & b64mask);
	s += b64symbol(sum & b64mask);
}

static void process_barcoder_byte(char c)
{
#ifdef DUMP_HEX
	static char buf[5] = {};
	unsigned const n = snprintf(buf, sizeof(buf)-1, "%02x ", c);
	Serial.write(buf, n);
#else
	static char last_byte;
	if (scan_inited) {
		Serial.write(c);
		if (!scan_done) {
			if (c != 0xA && c != 0xD)
				scan_buff += c;
			else {
				/* For some reason while reporting 1D and 2D codes
				 * the scanner uses different line endings.
				 */
				if (scan_buff == cmd_chsum_on) {
					scan_csum_on = true;
					// Bright cyan pulse indicates control code reception
					led_show_color(RGB_HCYAN);
					delay(30);
				} else if (scan_buff == cmd_chsum_off) {
					scan_csum_on = false;
					// Bright cyan pulse indicates control code reception
					led_show_color(RGB_HCYAN);
					delay(30);
				} else {
					// Bright green pulse indicates scanning completion
					if (scan_csum_on)
						append_csum(scan_buff);
					led_show_color(RGB_HGREEN);
					bleKeyboard.print(scan_buff);
					bleKeyboard.press(KEY_RETURN);
					delay(30);
					bleKeyboard.release(KEY_RETURN);
				}
				scan_done = true;
			}
		}
	} else if (last_byte == 0xff && c == 0x28)
		scan_inited = true;
	last_byte = c;
#endif
}

static void wait(unsigned msec)
{
	unsigned const start_ts = millis();
	static char buff[256];

	while (millis() - start_ts < msec) {
		unsigned sz = BarcodeSerial.readBytes(buff, sizeof(buff));
		for (unsigned i = 0; i < sz; ++i)
			process_barcoder_byte(buff[i]);
	}
}

static inline void start_scan(void)
{
	scan_inited = scan_done = false;
	scan_buff.clear();
#ifdef DUMP_HEX
	Serial.write('\n');
#endif
	// Bright magenta pulse indicates scanning start
	led_show_color(RGB_HMAGENTA);
	BARCODER_WRITE(barcoder_wake_up);
	wait(50);
	BARCODER_WRITE(barcoder_start);
}

void loop()
{
	static bool btn_pressed;

	bool const pressed = readBtn();
	if (pressed && !btn_pressed)
		start_scan();
	btn_pressed = pressed;

	const int is_connected = bleKeyboard.isConnected();
	if (!is_connected)
		bleKeyboard.restart_advertising();

	// Indicate connection status
	led_show_color(is_connected ? RGB_BLUE : RGB_YELLOW);

	wait(10);
}
