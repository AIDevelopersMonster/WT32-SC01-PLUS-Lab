# 08_WiFiTest — Wi-Fi validation

This Arduino example validates the ESP32-S3 Wi-Fi path in two levels.

## Level 1 — radio / scan validation

This level requires no credentials and runs automatically after flashing.

It validates:

- station-mode Wi-Fi initialization;
- STA MAC readout;
- active network scan;
- SSID discovery;
- BSSID discovery;
- channel reporting;
- RSSI reporting;
- authentication-mode reporting.

Expected final status when credentials are left blank:

```text
WIFI RADIO / SCAN PHYSICAL PASS CANDIDATE
Full association/DHCP/DNS/TCP/reconnect validation: PENDING
```

## Level 2 — infrastructure validation

For the full test, edit these two lines locally in `08_WiFiTest.ino`:

```cpp
static const char *WIFI_TEST_SSID = "your-ssid";
static const char *WIFI_TEST_PASSWORD = "your-password";
```

Do not commit real credentials to the repository.

The full test performs:

1. active Wi-Fi scan;
2. association to the configured AP;
3. DHCP/IP acquisition;
4. SSID/BSSID/channel/RSSI/MAC/IP/gateway/subnet/DNS reporting;
5. DNS resolution of `example.com`;
6. TCP connection to `example.com:80`;
7. HTTP `HEAD /` request and response-line verification;
8. three explicit disconnect/reconnect cycles.

Expected final result:

```text
WIFI TEST PHYSICAL PASS CANDIDATE
Scan + association + DHCP + DNS + TCP/HTTP + reconnect passed.
```

## Why HTTP rather than HTTPS in this diagnostic

The purpose of this example is to isolate and validate the board's Wi-Fi radio, IP stack and basic Internet connectivity. Plain HTTP avoids making certificate stores, TLS configuration or wall-clock validity part of the hardware acceptance criterion.

A later application can test TLS separately.

## Safety / isolation

The test does not initialize LCD, touch, SD, audio, RS485 or LVGL. It uses only the ESP32-S3 Wi-Fi subsystem and Serial diagnostics.

## Claim boundary

A successful run on the reference specimen validates the Arduino Wi-Fi path under the tested access-point and RF conditions. It does not establish maximum RF range, antenna gain, throughput, coexistence limits, all security modes, all regulatory domains or every WT32-SC01-PLUS OEM revision.
