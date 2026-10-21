/*
 * A bench fallback, not the normal path: credentials are provisioned at
 * runtime into NVS, over the setup access point or the USB console (see
 * README.md).  Copy this to secrets.h (ignored by git) only when the
 * card must join a network with no one there to type at it; build flags
 * -DWIFI_SSID=\"...\" -DWIFI_PASS=\"...\" work too.  NVS wins when it
 * holds a pair.
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-passphrase"
