# 08_WiFiTest — Wi-Fi validation

**Test status:** `PHYSICAL TEST VALIDATED`  
**Reference specimen:** Panlee `ZX3D50CE08S-V15-USRC / 230208`  
**Validation date:** 2026-08-16

This Arduino example validates the ESP32-S3 Wi-Fi path in two levels.

## Credential handling

Real Wi-Fi credentials are kept out of Git.

The repository tracks only:

```text
wifi_secrets.example.h
```

To enable the full infrastructure test, copy it locally to:

```text
wifi_secrets.h
```

and fill in:

```cpp
static const char *WIFI_TEST_SSID = "your-ssid";
static const char *WIFI_TEST_PASSWORD = "your-password";
```

`wifi_secrets.h` is explicitly listed in the repository `.gitignore` and must never be committed.

The sketch uses `__has_include("wifi_secrets.h")`:

- if the local secrets file exists, it is loaded and the full test runs;
- if it does not exist, safe empty credentials are compiled in and the test automatically stays in scan-only mode.

This means CI and GitHub always build with non-secret placeholder credentials while local Arduino IDE runs can use the real network.

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

The reference specimen physically passed the scan stage and detected nearby 2.4 GHz networks with valid SSID/BSSID/channel/RSSI/authentication reporting.

## Level 2 — infrastructure validation

With a populated local `wifi_secrets.h`, the full test performs:

1. active Wi-Fi scan;
2. association to the configured AP;
3. DHCP/IP acquisition;
4. SSID/BSSID/channel/RSSI/MAC/IP/gateway/subnet/DNS reporting;
5. DNS resolution of `example.com`;
6. TCP connection to `example.com:80`;
7. HTTP `HEAD /` request and response-line verification;
8. three explicit disconnect/reconnect cycles.

## Physical validation result

The full infrastructure run passed on the reference specimen:

```text
Wi-Fi scan        : PASS
Association       : PASS
DHCP              : PASS
DNS               : PASS
TCP               : PASS
HTTP              : HTTP/1.1 200 OK
Reconnect 1/3     : PASS
Reconnect 2/3     : PASS
Reconnect 3/3     : PASS
```

Final diagnostic result:

```text
WIFI TEST PHYSICAL PASS CANDIDATE
Scan + association + DHCP + DNS + TCP/HTTP + reconnect passed.
```

This promotes `08_WiFiTest` itself to:

```text
PHYSICAL TEST VALIDATED
```

## Video evidence

A real-hardware run showing the Wi-Fi scan, association, DHCP/DNS/TCP/HTTP path and reconnect completion is available here:

[YouTube Shorts — WT32-SC01-PLUS Wi-Fi physical validation](https://youtube.com/shorts/vxB9QxJIfA8)

The video is supporting physical-run evidence. The repository remains the source for the test logic, claim boundary and reproducible procedure.

## Why HTTP rather than HTTPS in this diagnostic

The purpose of this example is to isolate and validate the board's Wi-Fi radio, IP stack and basic Internet connectivity. Plain HTTP avoids making certificate stores, TLS configuration or wall-clock validity part of the hardware acceptance criterion.

A later application can test TLS separately.

## Safety / isolation

The test does not initialize LCD, touch, SD, audio, RS485 or LVGL. It uses only the ESP32-S3 Wi-Fi subsystem and Serial diagnostics.

## Claim boundary

A successful run on the reference specimen validates the Arduino Wi-Fi path under the tested access-point and RF conditions. It does not establish maximum RF range, antenna gain, throughput, coexistence limits, all security modes, all regulatory domains or every WT32-SC01-PLUS OEM revision.
