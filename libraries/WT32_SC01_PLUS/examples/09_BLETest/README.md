# 09_BLETest — Bluetooth Low Energy validation

This Arduino example validates the ESP32-S3 BLE path in two stages.

## Stage 1 — autonomous BLE scan

No phone interaction is required for the first stage.

The sketch:

- initializes the BLE stack;
- performs a 6 s active BLE scan;
- prints advertisement address, RSSI and device name when present;
- counts discovered BLE devices.

Expected intermediate result:

```text
BLE RADIO / SCAN PHYSICAL PASS CANDIDATE
Advertising/GATT validation: PENDING USER ACTION
```

## Stage 2 — peripheral / GATT validation

After the scan, the board starts advertising as:

```text
WT32-SC01-PLUS-BLE
```

Service UUID:

```text
7b9d0001-5d9f-4c8a-a6e8-4f7f6b320001
```

Characteristic UUID:

```text
7b9d0002-5d9f-4c8a-a6e8-4f7f6b320001
```

The characteristic supports READ, WRITE and NOTIFY.

Initial value:

```text
WT32-BLE-READY
```

### Phone procedure

Use a BLE utility such as nRF Connect or another generic GATT client.

1. Scan for `WT32-SC01-PLUS-BLE`.
2. Connect.
3. Open the service/characteristic above.
4. Read the characteristic; expect `WT32-BLE-READY`.
5. Enable notifications if the app requires it.
6. Write ASCII/UTF-8 text:

```text
WT32-BLE-PING
```

7. The characteristic is updated and notified with:

```text
WT32-BLE-PONG
```

Expected Serial Monitor result:

```text
[PASS] BLE client connected
[PASS] GATT write matched WT32-BLE-PING
[PASS] Characteristic updated/notified with WT32-BLE-PONG

BLE TEST PHYSICAL PASS CANDIDATE
Scan + advertise + connect + GATT read/write/notify passed.
```

## Claim boundary

A successful run validates BLE radio discovery and a basic BLE peripheral/GATT data path on the tested Panlee specimen and Arduino-ESP32 environment. It does not establish maximum RF range, throughput, all PHY modes, bonding/pairing/security behavior, power consumption, coexistence limits or every WT32-SC01-PLUS OEM revision.

ESP32-S3 BLE is tested here; Bluetooth Classic is not part of this diagnostic.
