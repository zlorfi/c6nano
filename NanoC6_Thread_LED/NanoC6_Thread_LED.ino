/**
 * NanoC6_Thread_LED.ino
 *
 * M5Stack NanoC6 — OpenThread UDP LED control
 *
 * Joins (or creates) a Thread network and listens on UDP port 5000 for
 * simple text commands to control the onboard blue LED and RGB LED.
 *
 * Commands (send as plain UTF-8 strings via UDP):
 *   LED_ON      — turn blue LED on
 *   LED_OFF     — turn blue LED off
 *   LED_TOGGLE  — toggle blue LED
 *   RGB_ON      — turn RGB LED on (white)
 *   RGB_OFF     — turn RGB LED off
 *
 * Board:            M5NanoC6
 * Partition Scheme: Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)
 * Flash Size:       4MB
 * USB CDC On Boot:  Enabled
 *
 * Required libraries:
 *   - M5NanoC6          (board support)
 *   - Adafruit NeoPixel (RGB LED)
 *
 * OpenThread is bundled with the M5Stack board package — no extra install.
 */

#include <M5NanoC6.h>
#include <Adafruit_NeoPixel.h>
#include "OThreadCLI.h"
#include "OThreadCLI_Util.h"
#include <openthread/udp.h>
#include <openthread/thread.h>
#include <openthread/ip6.h>
#include <openthread/instance.h>
#include <esp_openthread.h>

// ── Pin definitions ───────────────────────────────────────────────────────────
#define BLUE_LED_PIN   7    // onboard blue LED
#define RGB_DATA_PIN   20   // WS2812 data
#define RGB_PWR_PIN    19   // WS2812 power enable (active HIGH)
#define BUTTON_PIN     9    // onboard button (active LOW)

// ── Thread network credentials ────────────────────────────────────────────────
// Change these to match your Thread network, or leave as defaults to form
// a new standalone network.
#define THREAD_NETWORK_KEY  "00112233445566778899aabbccddeeff"
#define THREAD_CHANNEL      "15"
#define THREAD_PANID        "1234"

// ── UDP ───────────────────────────────────────────────────────────────────────
#define UDP_PORT  5000

// ── Globals ───────────────────────────────────────────────────────────────────
Adafruit_NeoPixel rgb(1, RGB_DATA_PIN, NEO_GRB + NEO_KHZ800);

static otUdpSocket  sUdpSocket;
static bool         sUdpOpen      = false;
static bool         blueLedState  = false;
static bool         rgbState      = false;
static otInstance  *sInstance     = NULL;

// ── Helper: set blue LED ──────────────────────────────────────────────────────
void setBlueLed(bool on) {
  blueLedState = on;
  digitalWrite(BLUE_LED_PIN, on ? HIGH : LOW);
  Serial.printf("[LED] Blue LED %s\n", on ? "ON" : "OFF");
}

// ── Helper: set RGB LED ───────────────────────────────────────────────────────
void setRgbLed(bool on) {
  rgbState = on;
  if (on) {
    rgb.setPixelColor(0, rgb.Color(80, 80, 80));  // dim white
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  }
  rgb.show();
  Serial.printf("[LED] RGB LED %s\n", on ? "ON" : "OFF");
}

// ── UDP receive callback ──────────────────────────────────────────────────────
void udpReceiveCallback(void *aContext, otMessage *aMessage,
                        const otMessageInfo *aMessageInfo) {
  char buf[64] = {0};
  uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
  if (length >= sizeof(buf)) {
    length = sizeof(buf) - 1;
  }
  otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);
  buf[length] = '\0';

  // Trim trailing whitespace / newlines
  for (int i = (int)length - 1; i >= 0; i--) {
    if (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ') {
      buf[i] = '\0';
    } else {
      break;
    }
  }

  Serial.printf("[UDP] Received: \"%s\"\n", buf);

  if (strcmp(buf, "LED_ON") == 0) {
    setBlueLed(true);
  } else if (strcmp(buf, "LED_OFF") == 0) {
    setBlueLed(false);
  } else if (strcmp(buf, "LED_TOGGLE") == 0) {
    setBlueLed(!blueLedState);
  } else if (strcmp(buf, "RGB_ON") == 0) {
    setRgbLed(true);
  } else if (strcmp(buf, "RGB_OFF") == 0) {
    setRgbLed(false);
  } else {
    Serial.printf("[UDP] Unknown command: \"%s\"\n", buf);
  }
}

// ── Open UDP socket ───────────────────────────────────────────────────────────
void openUdpSocket() {
  if (sUdpOpen) return;

  otSockAddr sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.mPort = UDP_PORT;

  otError err = otUdpOpen(sInstance, &sUdpSocket, udpReceiveCallback, NULL);
  if (err != OT_ERROR_NONE) {
    Serial.printf("[UDP] otUdpOpen failed: %d\n", err);
    return;
  }

  err = otUdpBind(sInstance, &sUdpSocket, &sockAddr, OT_NETIF_THREAD_HOST);
  if (err != OT_ERROR_NONE) {
    Serial.printf("[UDP] otUdpBind failed: %d\n", err);
    otUdpClose(sInstance, &sUdpSocket);
    return;
  }

  sUdpOpen = true;
  Serial.printf("[UDP] Listening on Thread port %d\n", UDP_PORT);
}

// ── Print IPv6 addresses ──────────────────────────────────────────────────────
void printIPv6Addresses() {
  char buf[OT_IP6_ADDRESS_STRING_SIZE];
  const otNetifAddress *addr = otIp6GetUnicastAddresses(sInstance);
  Serial.println("[Thread] IPv6 addresses:");
  while (addr != NULL) {
    otIp6AddressToString(&addr->mAddress, buf, sizeof(buf));
    Serial.printf("  %s\n", buf);
    addr = addr->mNext;
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== NanoC6 OpenThread LED Control ===");

  // Board init
  NanoC6.begin();

  // Blue LED
  pinMode(BLUE_LED_PIN, OUTPUT);
  setBlueLed(false);

  // RGB power enable + NeoPixel init
  pinMode(RGB_PWR_PIN, OUTPUT);
  digitalWrite(RGB_PWR_PIN, HIGH);
  rgb.begin();
  setRgbLed(false);

  // Button (input, active low — built-in pull-up via M5NanoC6)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ── Start OpenThread ────────────────────────────────────────────────────────
  // Pass false = do NOT autostart; we configure the dataset manually first
  OThread.begin(false);
  OThreadCLI.begin();

  Serial.println("[Thread] Configuring network dataset...");
  OThreadCLI.println("dataset init new");
  delay(100);
  OThreadCLI.println("dataset networkkey " THREAD_NETWORK_KEY);
  delay(100);
  OThreadCLI.println("dataset channel " THREAD_CHANNEL);
  delay(100);
  OThreadCLI.println("dataset panid 0x" THREAD_PANID);
  delay(100);
  OThreadCLI.println("dataset commit active");
  delay(100);
  OThreadCLI.println("ifconfig up");
  delay(200);
  OThreadCLI.println("thread start");
  delay(200);

  sInstance = esp_openthread_get_instance();

  Serial.println("[Thread] Waiting for network role...");
  // Blink blue LED while waiting to join / form a network
  unsigned long t = millis();
  while (OThread.otGetDeviceRole() < OT_ROLE_CHILD) {
    if (millis() - t > 30000) {
      Serial.println("[Thread] Timeout waiting for network — continuing anyway");
      break;
    }
    digitalWrite(BLUE_LED_PIN, !digitalRead(BLUE_LED_PIN));  // fast blink
    delay(300);
  }
  setBlueLed(false);

  Serial.printf("[Thread] Role: %s\n", OThread.otGetStringDeviceRole());
  printIPv6Addresses();

  // Open UDP listener
  openUdpSocket();

  Serial.println("[Ready] Send UDP commands to this node on port " + String(UDP_PORT));
  Serial.println("  LED_ON / LED_OFF / LED_TOGGLE");
  Serial.println("  RGB_ON / RGB_OFF");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  // Re-open UDP socket if it closes (e.g. after role change)
  if (!sUdpOpen && OThread.otGetDeviceRole() >= OT_ROLE_CHILD) {
    openUdpSocket();
  }

  // Physical button toggles blue LED
  static bool lastButtonState = HIGH;
  bool currentButton = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButton == LOW) {
    delay(20);  // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      setBlueLed(!blueLedState);
    }
  }
  lastButtonState = currentButton;

  // Flush any OThreadCLI output to Serial (useful for debugging)
  while (OThreadCLI.available()) {
    Serial.write(OThreadCLI.read());
  }

  delay(10);
}
