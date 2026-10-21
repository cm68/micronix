/*
 * Copy to secrets.h (ignored by git) and fill in.
 *
 * JOIN is what puts the card on its network; there is no way to pass
 * credentials over the byte channel, so they are compiled in.  Build
 * flags work too: -DWIFI_SSID=\"...\" -DWIFI_PASS=\"...\".
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-passphrase"
