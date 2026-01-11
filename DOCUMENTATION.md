# Lilygo T5-4.7 Quote Display - Complete Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Flow Diagrams](#flow-diagrams)
4. [Module Documentation](#module-documentation)
5. [API Reference](#api-reference)
6. [Data Structures](#data-structures)
7. [Configuration](#configuration)

---

## Project Overview

### Description
An ESP32-based e-paper quote display that fetches random Italian quotes from a REST API and displays them on a 960x540 e-paper screen. The device uses deep sleep for power efficiency, waking periodically or on button press to refresh the quote.

### Hardware
- **Board**: Lilygo T5-4.7
- **Display**: 4.7" e-paper (ED047TC1), 960x540 resolution, 16 grayscale levels
- **MCU**: ESP32 with PSRAM
- **Memory**: 16MB Flash
- **Wake Buttons**:
  - GPIO 39: Quote refresh button
  - GPIO 35: Network reset button

### Key Features
- WiFi provisioning with captive portal
- Italian quote API integration (quotes-api-three.vercel.app)
- Unique AP SSID per device (MAC-based)
- Deep sleep with random wake intervals (10-60 minutes)
- Button wake for immediate quote refresh
- Network configuration reset via GPIO 35
- Quote counter and next update time display
- Battery voltage monitoring with percentage display
- Random loading gerund animations
- SNTP time synchronization (Europe/Rome)

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         LILYGO T5-4.7 DEVICE                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐           │
│  │   GPIO 39    │    │   GPIO 35    │    │  E-Paper     │           │
│  │ Quote Button │    │ Reset Button │    │  960x540     │           │
│  └──────┬───────┘    └──────┬───────┘    └──────▲───────┘           │
│         │                   │                   │                   │
│         │ EXT0 Wake         │ EXT1 Wake         │                   │
│         │                   │                   │                   │
│  ┌──────▼───────────────────▼───────────────────┴───────┐           │
│  │                                                      │           │
│  │                ESP32 Main Application                │           │
│  │                     (main.c)                         │           │
│  │                                                      │           │
│  └───┬─────────┬─────────┬─────────┬─────────┬─────────┬─────┐      │
│      │         │         │         │         │         │     │      │
│  ┌───▼─────┐ ┌▼──────┐ ┌▼──────┐ ┌▼──────┐ ┌▼──────┐ ┌▼───┐  │      │
│  │Display  │ │WiFi   │ │Web    │ │Sleep  │ │Wiki   │ │Batt│  │      │
│  │UI       │ │Mgr    │ │Server │ │Mgr    │ │Quote  │ │Mntr│  │      │
│  └───┬─────┘ └┬──────┘ └┬──────┘ └┬──────┘ └┬──────┘ └┬───┘  │      │
│      │        │         │         │         │         │   ┌──▼─┐    │
│      │        │         │         │         │         │   │Ger.│    │
│      │        │         │         │         │         │   └────┘    │
│      │        │         │         │         │                       │
│  ┌───▼────────▼─────────▼─────────▼─────────▼─────────▼───┐         │
│  │               ESP-IDF Framework                        │         │
│  │  ┌──────────┐  ┌──────┐  ┌────┐  ┌─────────┐           │         │
│  │  │  EPDiy   │  │ WiFi │  │HTTP│  │ NVS     │           │         │
│  │  │  Driver  │  │      │  │Serv│  │ Storage │           │         │
│  │  └──────────┘  └──────┘  └────┘  └─────────┘           │         │
│  └────────────────────────────────────────────────────────┘         │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                      PERSISTENT STORAGE (NVS)                       │
├─────────────────────────────────────────────────────────────────────┤
│  Namespace: "wifi_config"                                           │
│    - ssid: WiFi network name (max 32 chars)                         │
│    - password: WiFi password (max 64 chars)                         │
│    - quote_count: Total quotes displayed (uint32_t)                 │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                      EXTERNAL SERVICES                              │
├─────────────────────────────────────────────────────────────────────┤
│  - quotes-api-three.vercel.app/random (Italian quotes API)          │
│  - pool.ntp.org (SNTP time server)                                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Flow Diagrams

### 1. Boot and Main Flow

```
                            ┌──────────────┐
                            │  Power On /  │
                            │   Wake Up    │
                            └──────┬───────┘
                                   │
                            ┌──────▼───────┐
                            │ Initialize   │
                            │ NVS Flash    │
                            └──────┬───────┘
                                   │
                            ┌──────▼───────┐
                            │ Initialize   │
                            │ Sleep Mgr    │
                            └──────┬───────┘
                                   │
                            ┌──────▼───────┐
                            │ Check Wake   │
                            │   Source     │
                            └──────┬───────┘
                                   │
                   ┌───────────────┼───────────────┐
                   │               │               │
            ┌──────▼──────┐ ┌─────▼─────┐  ┌─────▼─────┐
            │ GPIO 35     │ │ GPIO 39   │  │  Timer    │
            │ Reset Button│ │Refresh Btn│  │  Wake     │
            └──────┬──────┘ └─────┬─────┘  └─────┬─────┘
                   │              │              │
                   │              └───────┬──────┘
                   │                      │
            ┌──────▼──────┐         ┌─────▼──────┐
            │  Display    │         │  Display   │
            │   Reset     │         │  Random    │
            │Confirmation │         │  Gerund    │
            └──────┬──────┘         └────┬───────┘
                   │                     │
            ┌──────▼──────┐              │
            │ Wait 10sec  │              │
            │ Monitor for │              │
            │ 3 Presses   │              │
            └──────┬──────┘              │
                   │                     │
           ┌───────┴────────┐            │
           │                │            │
      ┌────▼────┐      ┌────▼────┐       │
      │3 Press? │      │ Timeout │       │
      │  YES    │      │   NO    │       │
      └────┬────┘      └────┬────┘       │
           │                │            │
      ┌────▼────┐      ┌────▼────┐       │
      │ Delete  │      │ Reboot  │       │
      │  WiFi   │      │ No Chg  │       │
      │ Creds   │      └─────────┘       │
      └────┬────┘                        │
           │                             │
      ┌────▼────┐                        │
      │ Reboot  │                        │
      └─────────┘                        │
                                         │
                            ┌────────────▼────────────┐
                            │ Initialize Display      │
                            └────────────┬────────────┘
                                         │
                            ┌────────────▼────────────┐
                            │ Initialize WiFi Manager │
                            └────────────┬────────────┘
                                         │
                            ┌────────────▼────────────┐
                            │ Start WiFi Manager      │
                            │ (Check for credentials) │
                            └────────────┬────────────┘
                                         │
                                 ┌───────┴────────┐
                                 │                │
                        ┌────────▼──────┐  ┌──────▼────────┐
                        │ Credentials   │  │ No Credentials│
                        │    Found      │  │   or Failed   │
                        └────────┬──────┘  └──────┬────────┘
                                 │                │
                                 │         ┌──────▼────────┐
                                 │         │ Provisioning  │
                                 │         │     Mode      │
                                 │         └──────┬────────┘
                                 │                │
                                 │         ┌──────▼────────┐
                                 │         │ See Provision │
                                 │         │  Flow Below   │
                                 │         └───────────────┘
                                 │
                        ┌────────▼──────┐
                        │ Connect to    │
                        │ WiFi Network  │
                        └────────┬──────┘
                                 │
                        ┌────────▼──────┐
                        │ Sync Time via │
                        │     SNTP      │
                        └────────┬──────┘
                                 │
                        ┌────────▼──────┐
                        │ Fetch Quote   │
                        │  from API     │
                        └────────┬──────┘
                                 │
                        ┌────────▼──────┐
                        │ Display Quote │
                        │ with Metadata │
                        └────────┬──────┘
                                 │
                        ┌────────▼──────┐
                        │ Calculate     │
                        │ Random Sleep  │
                        │ (10-60 min)   │
                        └────────┬──────┘
                                 │
                        ┌────────▼──────┐
                        │ Enter Deep    │
                        │    Sleep      │
                        └────────┬──────┘
                                 │
                                 │ ┌─────────────┐
                                 └─┤ Wake Event  │
                                   │ (Loop back) │
                                   └─────────────┘
```

### 2. WiFi Provisioning Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                   PROVISIONING MODE                             │
└─────────────────────────────────────────────────────────────────┘

            ┌──────────────────┐
            │ No WiFi Creds OR │
            │Connection Failed │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │ Get MAC Address  │
            │ (Last 2 digits)  │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │ Create AP SSID:  │
            │  "WMQuote_XX"    │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │  Start WiFi AP   │
            │  192.168.4.1     │
            │  (Open, no pass) │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │ Start HTTP Server│
            │   on port 80     │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │  Display Screen: │
            │ "Connect to      │
            │ 'WMQuote_XX'     │
            │  network..."     │
            └────────┬─────────┘
                     │
            ┌────────▼─────────┐
            │   Wait for User  │
            │   to Connect     │
            └────────┬─────────┘
                     │
       ┌─────────────┴─────────────┐
       │                           │
       │  ┌────────────────────┐   │
       │  │ User connects to   │   │
       │  │   WMQuote_XX AP    │   │
       │  └─────────┬──────────┘   │
       │            │              │
       │  ┌─────────▼──────────┐   │
       │  │ User opens browser │   │
       │  │  192.168.4.1       │   │
       │  └─────────┬──────────┘   │
       │            │              │
       │  ┌─────────▼──────────┐   │
       │  │  GET / request     │   │
       │  │ Serve HTML form    │   │
       │  └─────────┬──────────┘   │
       │            │              │
       │  ┌─────────▼──────────┐   │
       │  │ User enters SSID   │   │
       │  │   and password     │   │
       │  └─────────┬──────────┘   │
       │            │              │
       │  ┌─────────▼──────────┐   │
       │  │ POST /save request │   │
       │  └─────────┬──────────┘   │
       │            │              │
       └────────────┼──────────────┘
                    │
            ┌───────▼────────┐
            │ Parse form data│
            │ URL decode     │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │ Validate SSID  │
            │  (1-32 chars)  │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │  Save to NVS:  │
            │  - ssid        │
            │  - password    │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │ Send HTTP 200  │
            │ "Saved! Will   │
            │  restart..."   │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │  esp_restart() │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │   Device       │
            │   Reboots      │
            └───────┬────────┘
                    │
            ┌───────▼────────┐
            │ Next boot will │
            │ use saved creds│
            └────────────────┘
```

### 3. Quote Fetch and Display Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                   QUOTE FETCH & DISPLAY                         │
└─────────────────────────────────────────────────────────────────┘

       ┌────────────────────┐
       │ WiFi Connected     │
       │ IP Address Got     │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Start Connection   │
       │   Setup Task       │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Calculate Random   │
       │ Sleep Duration     │
       │  (10-60 minutes)   │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Wait for SNTP Sync │
       │ (30 sec timeout)   │
       └─────────┬──────────┘
                 │
          ┌──────┴──────┐
          │             │
       ┌──▼───┐    ┌───▼────┐
       │ Sync │    │Timeout │
       │  OK  │    │  Fail  │
       └──┬───┘    └───┬────┘
          │            │
          │    ┌───────▼────────┐
          │    │  Set default   │
          │    │  time values   │
          │    └───────┬────────┘
          │            │
          └────────┬───┘
                   │
       ┌───────────▼────────┐
       │ Initialize Battery │
       │ Monitoring (ADC)   │
       └───────────┬────────┘
                   │
       ┌───────────▼────────┐
       │ Read Battery       │
       │ Voltage & Calculate│
       │ Percentage         │
       └───────────┬────────┘
                   │
       ┌───────────▼────────┐
       │ Get Current Time   │
       │ Format: DD/MM/YYYY │
       │        HH:MM       │
       └───────────┬────────┘
                   │
       ┌───────────▼────────┐
       │ Calculate Next     │
       │ Update Time:       │
       │ now + sleep_sec    │
       └───────────┬────────┘
                   │
       ┌───────────▼────────┐
       │ HTTP GET Request   │
       │ quotes-api-three   │
       │ .vercel.app/random │
       └───────────┬────────┘
                   │
            ┌──────┴──────┐
            │             │
       ┌────▼────┐   ┌────▼────┐
       │ Success │   │  Failed │
       │ 200 OK  │   │   Err   │
       └────┬────┘   └────┬────┘
            │             │
       ┌────▼────┐   ┌────▼────┐
       │  Parse  │   │ Fallback│
       │  JSON   │   │  Quote  │
       │ Extract:│   │ "Quote  │
       │ - quote │   │  error" │
       │ - author│   └────┬────┘
       └────┬────┘        │
            │             │
            └─────┬───────┘
                  │
       ┌──────────▼──────────┐
       │ Increment Quote     │
       │ Counter in NVS      │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Get Quote Count     │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Format Status Line: │
       │ "Last update: ...   │
       │  quotes: X          │
       │  next: HH:MM        │
       │  batt: XX%"         │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Display Quote:      │
       │ - Clear screen      │
       │ - White refresh     │
       │ - Draw quote        │
       │ - Draw author       │
       │ - Draw timestamp    │
       │ - Draw logo 64x64   │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Stop SNTP Service   │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Disconnect WiFi     │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │ Enter Deep Sleep    │
       │ with:               │
       │ - Timer (random)    │
       │ - GPIO 39 wake      │
       │ - GPIO 35 wake      │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │   Sleep Mode...     │
       └─────────────────────┘
```

### 4. Network Reset Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                   NETWORK RESET FLOW                            │
└─────────────────────────────────────────────────────────────────┘

       ┌────────────────────┐
       │ Deep Sleep Mode    │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ User Presses       │
       │    GPIO 35         │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ EXT1 Wake Trigger  │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │   Device Boots     │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Check Wake Source  │
       │ = EXT1 (GPIO 35)   │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Display Screen:    │
       │ "To reset network  │
       │ configuration...   │
       │ press 3 times in   │
       │ 10 seconds..."     │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Start 10 Second    │
       │   Countdown        │
       └─────────┬──────────┘
                 │
       ┌─────────▼──────────┐
       │ Monitor GPIO 35    │
       │ Poll every 50ms    │
       │ Edge detection:    │
       │ High → Low         │
       └─────────┬──────────┘
                 │
                 │ ┌──────────────┐
                 ├─┤ Press Count  │
                 │ │  = 0         │
                 │ └──────────────┘
                 │
       ┌─────────▼──────────┐
       │ Button Pressed?    │
       └─────────┬──────────┘
                 │
            Yes  │  No
       ┌─────────┼──────────┐
       │         │          │
       ▼         │          ▼
   ┌───────┐     │     ┌─────────┐
   │Press++│     │     │Continue │
   │Log it │     │     │Polling  │
   └───┬───┘     │     └────┬────┘
       │         │          │
       │         │     ┌────▼─────┐
       │         │     │10s Over? │
       │         │     └────┬─────┘
       │         │          │
       │         │      No  │  Yes
       │         │          │
       │         │     ┌────▼─────┐
       │         │     │ Timeout  │
       │         │     │ Reached  │
       │         │     └────┬─────┘
       │         │          │
   ┌───▼─────────▼──────────▼───┐
   │  Check Press Count         │
   └───────────┬────────────────┘
               │
       ┌───────┴────────┐
       │                │
   ┌───▼────┐      ┌────▼────┐
   │Count>=3│      │Count<3  │
   │  YES   │      │   NO    │
   └───┬────┘      └────┬────┘
       │                │
   ┌───▼────────┐  ┌────▼─────────┐
   │ Log:       │  │ Log: Timeout │
   │"Reset      │  │ Only X/3     │
   │confirmed!" │  │ presses      │
   └───┬────────┘  └────┬─────────┘
       │                │
   ┌───▼────────┐  ┌────▼─────────┐
   │ NVS Open   │  │ Log: Reset   │
   │ Erase:     │  │ cancelled    │
   │ - ssid     │  └────┬─────────┘
   │ - password │       │
   └───┬────────┘       │
       │                │
   ┌───▼────────┐       │
   │ NVS Commit │       │
   └───┬────────┘       │
       │                │
   ┌───▼────────┐  ┌────▼─────────┐
   │ Log:       │  │ esp_restart()│
   │"Creds      │  │ (no changes) │
   │ deleted"   │  └──────────────┘
   └───┬────────┘
       │
   ┌───▼────────┐
   │Wait 1 sec  │
   └───┬────────┘
       │
   ┌───▼────────┐
   │esp_restart()│
   └───┬────────┘
       │
   ┌───▼────────┐
   │ Device     │
   │ Reboots    │
   └───┬────────┘
       │
   ┌───▼─────────────┐
   │ Next Boot:      │
   │ No credentials  │
   │ → Provisioning  │
   │    Mode         │
   └─────────────────┘
```

### 5. State Machine Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                   DEVICE STATE MACHINE                          │
└─────────────────────────────────────────────────────────────────┘

                       ╔════════════════╗
                       ║   POWER ON /   ║
                       ║   WAKE UP      ║
                       ╚═══════┬════════╝
                               │
                       ╔═══════▼════════╗
                       ║  INITIALIZING  ║
                       ║  - NVS         ║
                       ║  - Display     ║
                       ║  - Sleep Mgr   ║
                       ╚═══════┬════════╝
                               │
                 ┌─────────────┼─────────────┐
                 │             │             │
         ╔═══════▼═══════╗ ╔═══▼════╗ ╔═════▼═════╗
         ║ RESET_CONFIRM ║ ║ LOADING║ ║   COLD    ║
         ║  (GPIO 35)    ║ ║(Wakeup)║ ║   BOOT    ║
         ╚═══════┬═══════╝ ╚═══┬════╝ ╚═════┬═════╝
                 │             │             │
                 │             └──────┬──────┘
                 │                    │
                 │             ╔══════▼═════════╗
                 │             ║ CHECK_WIFI_CFG ║
                 │             ║ (Read NVS)     ║
                 │             ╚══════┬═════════╝
                 │                    │
                 │          ┌─────────┴─────────┐
                 │          │                   │
                 │   ╔══════▼═════════╗  ╔══════▼═══════╗
                 │   ║  WIFI_CONNECT  ║  ║ PROVISIONING ║
                 │   ║ (STA mode)     ║  ║  (AP mode)   ║
                 │   ╚══════┬═════════╝  ╚══════┬═══════╝
                 │          │                   │
                 │          │ ┌─────────────────┘
                 │          │ │ Retry Failed
                 │          │ │
                 │   ╔══════▼═▼════════╗
                 │   ║   CONNECTING    ║
                 │   ║  (Retry up to   ║
                 │   ║   3 times)      ║
                 │   ╚══════┬══════════╝
                 │          │
                 │   ╔══════▼══════════╗
                 │   ║   CONNECTED     ║
                 │   ║ (Got IP)        ║
                 │   ╚══════┬══════════╝
                 │          │
                 │   ╔══════▼══════════╗
                 │   ║  TIME_SYNC      ║
                 │   ║  (SNTP)         ║
                 │   ╚══════┬══════════╝
                 │          │
                 │   ╔══════▼══════════╗
                 │   ║ FETCHING_QUOTE  ║
                 │   ║ (HTTP GET)      ║
                 │   ╚══════┬══════════╝
                 │          │
                 │   ╔══════▼══════════╗
                 │   ║ DISPLAYING      ║
                 │   ║ (E-paper draw)  ║
                 │   ╚══════┬══════════╝
                 │          │
                 │   ╔══════▼══════════╗
                 │   ║   CLEANUP       ║
                 │   ║ - Stop SNTP     ║
                 │   ║ - Disconnect    ║
                 │   ╚══════┬══════════╝
                 │          │
                 └──────────┼──────────┘
                            │
                     ╔══════▼══════════╗
                     ║   DEEP_SLEEP    ║
                     ║ - Timer wake    ║
                     ║ - GPIO 39 wake  ║
                     ║ - GPIO 35 wake  ║
                     ╚══════┬══════════╝
                            │
                            │ Wake Event
                            │
                            └────────────┐
                                         │
                            ╔════════════▼═╗
                            ║   WAKE UP    ║
                            ╚══════════════╝
```

---

## Module Documentation

### Module 1: main.c

**Purpose**: Application entry point and orchestration

**Responsibilities**:
- Initialize all subsystems (NVS, display, WiFi, sleep manager)
- Determine wake source and route to appropriate handler
- Implement GPIO 35 network reset confirmation logic
- Main application loop

**Key Functions**:

#### `app_main()`
Main application entry point.

**Flow**:
1. Initialize NVS flash
2. Initialize sleep manager
3. Check wake source (cold boot, timer, GPIO 39, GPIO 35)
4. Handle GPIO 35 reset flow if applicable
5. Show loading screen for normal wake
6. Initialize and start WiFi manager
7. Enter infinite loop (WiFi manager handles rest via events)

#### `wait_for_reset_confirmation()`
Monitors GPIO 35 for 3 button presses within 10 seconds.

**Parameters**: None

**Returns**:
- `true` - User pressed button 3 times (reset confirmed)
- `false` - Timeout reached with <3 presses (reset cancelled)

**Algorithm**:
```
press_count = 0
start_time = current_tick
last_button_state = read_gpio_35()

while (elapsed_time < 10_seconds):
    current_state = read_gpio_35()

    if (last_state == HIGH and current_state == LOW):
        # Falling edge detected (button pressed)
        press_count++

        if press_count >= 3:
            return true

        # Wait for button release (debounce)
        while (read_gpio_35() == LOW):
            delay(10ms)

    last_state = current_state
    delay(50ms)  # Polling interval

return false  # Timeout
```

**Constants**:
- `RESET_BUTTON_GPIO`: GPIO_NUM_35
- `RESET_TIMEOUT_MS`: 10000 (10 seconds)
- `RESET_PRESS_COUNT`: 3

---

### Module 2: display_ui.c / display_ui.h

**Purpose**: E-paper display rendering for all UI states

**Responsibilities**:
- Initialize EPDiy driver with Lilygo T5-4.7 configuration
- Render provisioning mode screen
- Render quote display with word wrapping
- Render loading screen with random gerund
- Render connection status
- Render network reset confirmation

**Display Specifications**:
- Resolution: 960x540 pixels (landscape)
- Color depth: 4-bit grayscale (16 levels)
- Fonts: FiraSans_20, FiraSans_12, OpenSans8
- Logos: 256x256 (provisioning), 64x64 (quote display)

**Key Functions**:

#### `void display_init()`
Initialize e-paper display hardware.

**Actions**:
- Initialize EPD with `epd_board_lilygo_t5_47` and `ED047TC1`
- Use 64K LUT for 4-bit color
- Initialize high-level state with builtin waveform
- Set landscape orientation (960x540)

#### `void display_provisioning_mode(const char* ap_name)`
Display WiFi provisioning instructions.

**Parameters**:
- `ap_name`: Access point SSID to display (e.g., "WMQuote_A3")

**Layout**:
```
┌─────────────────────────────────────────────────────────┐
│                     960x540                             │
│                                                         │
│     ┌────────┐   Connect to                             │
│     │        │   'WMQuote_XX' network                   │
│     │  LOGO  │   to configure WiFi                      │
│     │ 256x256│                                          │
│     │        │   Open: http://192.168.4.1               │
│     └────────┘                                          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

#### `void display_connected_mode(const char* quote, const char* author, const char* datetime_text)`
Display fetched quote with metadata.

**Parameters**:
- `quote`: Quote text (will be word-wrapped)
- `author`: Author name (displayed in parentheses)
- `datetime_text`: Status line with timestamp, count, next update

**Layout**:
```
┌─────────────────────────────────────────────────────────┐
│                     960x540                             │
│                                                         │
│              Quote text here with                       │
│              automatic word wrapping                    │
│              centered on screen                         │
│                                                         │
│                   (Author Name)                         │
│                                                         │
│                                                         │
│ Last update: ...         ┌──────┐                       │
│ quotes: X                │ LOGO │                       │
│ next: HH:MM              │ 64x64│                       │
└─────────────────────────────────────────────────────────┘
```

**Word Wrapping Algorithm**:
```
max_width = 860 pixels
line_height = 35 pixels
start_y = 150

for each word in quote:
    if (current_line + word) > max_width:
        draw current_line (centered)
        y += line_height
        current_line = word
    else:
        current_line += word

draw final line
```

**White Screen Refresh**:
Before drawing quote, performs complete white refresh to eliminate ghosting:
1. Fill framebuffer with 0xFF (white)
2. Update screen with MODE_GC16
3. Wait 500ms
4. Clear and draw new content

#### `void display_loading(const char* gerund)`
Display loading screen with random gerund.

**Parameters**:
- `gerund`: Gerund word (e.g., "Thinking", "Pondering")

**Layout**:
```
┌─────────────────────────────────────────────────────────┐
│                     960x540                             │
│                                                         │
│     ┌────────┐                                          │
│     │        │                                          │
│     │  LOGO  │   Thinking...                            │
│     │ 256x256│                                          │
│     │        │                                          │
│     └────────┘                                          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

Text position: x=380, y=270 (close to logo, vertically centered)

#### `void display_connecting(const char* ssid)`
Display WiFi connection attempt message.

**Parameters**:
- `ssid`: Network name being connected to

#### `void display_reset_confirmation()`
Display network reset confirmation prompt.

**Layout**:
```
┌─────────────────────────────────────────────────────────┐
│                     960x540                             │
│                                                         │
│     ┌────────┐   To reset network                       │
│     │        │   configuration press                    │
│     │  LOGO  │   same button 3 times                    │
│     │ 256x256│   in next 10 seconds                     │
│     │        │   or wait to cancel.                     │
│     └────────┘                                          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

### Module 3: wifi_manager.c / wifi_manager.h

**Purpose**: WiFi connection management, provisioning, and credential storage

**Responsibilities**:
- Manage WiFi state machine (STA and AP modes)
- Store/retrieve credentials from NVS
- Handle WiFi events (connect, disconnect, got IP)
- Start provisioning AP with unique SSID
- Coordinate quote fetch, display, and sleep
- Track quote counter

**NVS Schema**:
```
Namespace: "wifi_config"
Keys:
  - "ssid": string (max 32 bytes)
  - "password": string (max 64 bytes)
  - "quote_count": uint32_t
```

**Key Functions**:

#### `esp_err_t wifi_manager_init()`
Initialize WiFi subsystem.

**Actions**:
- Initialize TCP/IP adapter
- Create default event loop
- Initialize WiFi with default config
- Set storage to flash
- Register event handlers:
  - WiFi events → `wifi_event_handler()`
  - IP events → `ip_event_handler()`

#### `esp_err_t wifi_manager_start(bool silent)`
Start WiFi manager and check for credentials.

**Parameters**:
- `silent`: If true, skip "Connecting..." display (for wake from sleep)

**Flow**:
```
Open NVS namespace "wifi_config"

Try to read "ssid" key
if exists:
    Read "password" key

    if not silent:
        display_connecting(ssid)

    Configure WiFi STA mode with credentials
    Start WiFi
    esp_wifi_connect()

    return ESP_OK
else:
    # No credentials found
    start_provisioning_mode()

    return ESP_OK
```

#### `static void start_provisioning_mode()`
Start WiFi access point for web-based configuration.

**Flow**:
```
# Get MAC address
esp_read_mac(mac, ESP_MAC_WIFI_STA)

# Create SSID: WMQuote_XX
snprintf(ap_ssid, "WMQuote_%02X", mac[5])

# Configure AP
ap_config = {
    .ssid = ap_ssid,
    .password = "",  # Open network
    .max_connection = 1,
    .authmode = WIFI_AUTH_OPEN,
    .channel = 1
}

# Start WiFi in AP mode
esp_wifi_set_mode(WIFI_MODE_AP)
esp_wifi_set_config(WIFI_IF_AP, &ap_config)
esp_wifi_start()

# Start web server
start_webserver()

# Display provisioning screen
display_provisioning_mode(ap_ssid)
```

**AP Network Configuration**:
- IP: 192.168.4.1
- Gateway: 192.168.4.1
- Netmask: 255.255.255.0

#### `static void wifi_event_handler()`
Handle WiFi connection events.

**Events**:
- `WIFI_EVENT_STA_START`: Auto-connect initiated
- `WIFI_EVENT_STA_DISCONNECTED`:
  - Retry connection (up to 3 times)
  - If max retries exceeded → start provisioning mode

#### `static void ip_event_handler()`
Handle IP assignment events.

**Events**:
- `IP_EVENT_STA_GOT_IP`:
  - Log IP address
  - Start connection setup task (quote fetch)

#### `static void connection_setup_task(void* pvParameters)`
Main task for quote fetch, display, and sleep.

**Flow**:
```
# Calculate random sleep duration FIRST
random_minutes = 10 + (esp_random() % 51)  # 10-60
sleep_seconds = random_minutes * 60

# Start SNTP time sync
sntp_setoperatingmode(SNTP_OPMODE_POLL)
sntp_setservername(0, "pool.ntp.org")
sntp_set_timezone(1)  # Europe/Rome
sntp_init()

# Wait for time sync (30 sec timeout)
for i in 0..60:
    time(&now)
    if now > (time from 2020):
        break
    delay(500ms)

# Initialize and read battery
battery_init()
battery_percent = battery_read_percentage()

# Get current time
localtime_r(&now, &timeinfo)
strftime(time_part, "Last update: %d/%m/%Y %H:%M", &timeinfo)

# Calculate next update time
next_update = now + sleep_seconds
localtime_r(&next_update, &next_update_tm)
strftime(next_update_str, "%H:%M", &next_update_tm)

# Fetch quote from API
wikiquote_fetch(&quote, &author)

# Increment and get quote count
wifi_manager_increment_quote_count()
count = wifi_manager_get_quote_count()

# Format complete status string with battery
if battery_percent >= 0:
    snprintf(datetime_str, "%s - quotes: %lu - next: %s - batt: %.0f%%",
             time_part, count, next_update_str, battery_percent)
else:
    snprintf(datetime_str, "%s - quotes: %lu - next: %s - batt: --%%",
             time_part, count, next_update_str)

# Display quote
display_connected_mode(quote, author, datetime_str)

# Cleanup
sntp_stop()
esp_wifi_disconnect()

# Enter deep sleep
sleep_manager_enter_deep_sleep(sleep_seconds)
```

#### `esp_err_t wifi_manager_save_credentials(const char* ssid, const char* password)`
Save WiFi credentials to NVS.

**Parameters**:
- `ssid`: Network SSID (max 32 chars)
- `password`: Network password (max 64 chars)

**Returns**: ESP_OK on success

**Implementation**:
```
Open NVS handle "wifi_config" (READWRITE)
nvs_set_str(handle, "ssid", ssid)
nvs_set_str(handle, "password", password)
nvs_commit(handle)
Close handle
```

#### `uint32_t wifi_manager_get_quote_count()`
Retrieve quote counter from NVS.

**Returns**: Quote count (0 if not found)

#### `esp_err_t wifi_manager_increment_quote_count()`
Increment quote counter in NVS.

**Implementation**:
```
count = wifi_manager_get_quote_count()
count++

Open NVS handle "wifi_config" (READWRITE)
nvs_set_u32(handle, "quote_count", count)
nvs_commit(handle)
Close handle
```

#### `esp_err_t wifi_manager_delete_credentials()`
Erase WiFi credentials from NVS.

**Implementation**:
```
Open NVS handle "wifi_config" (READWRITE)
nvs_erase_key(handle, "ssid")
nvs_erase_key(handle, "password")
nvs_commit(handle)
Close handle
```

**Note**: Quote counter is NOT deleted during reset

---

### Module 4: webserver.c / webserver.h

**Purpose**: HTTP server for WiFi provisioning web interface

**Responsibilities**:
- Serve HTML configuration form
- Handle form submission
- Parse and save WiFi credentials
- Trigger device restart

**Endpoints**:

#### `GET /`
Serve HTML configuration page.

**Response**:
- Content-Type: text/html
- Body: HTML form from `config_page.h`

#### `POST /save`
Receive WiFi credentials and save to NVS.

**Request**:
- Content-Type: application/x-www-form-urlencoded
- Body: `ssid=<ssid>&password=<password>`

**Flow**:
```
Parse POST body
Extract "ssid" parameter (URL decode)
Extract "password" parameter (URL decode)

if ssid.length < 1 or ssid.length > 32:
    return 400 Bad Request

wifi_manager_save_credentials(ssid, password)

Send 200 OK response:
  "WiFi credentials saved! Device will restart..."

delay(1000ms)
esp_restart()
```

**Key Functions**:

#### `httpd_handle_t start_webserver()`
Start HTTP server on port 80.

**Configuration**:
- Port: 80
- Max open sockets: 7
- Max URI handlers: 8
- Stack size: 4096 bytes

**URI Handlers**:
- `GET /` → `root_get_handler`
- `POST /save` → `save_post_handler`

---

### Module 5: wikiquote.c / wikiquote.h

**Purpose**: Fetch random quotes from Italian quotes API

**API Details**:
- Endpoint: `http://quotes-api-three.vercel.app/random`
- Method: GET
- Response: JSON with `quote` and `author` fields

**Key Functions**:

#### `esp_err_t wikiquote_fetch(char** quote, char** author)`
Fetch random Italian quote from API.

**Parameters**:
- `quote`: Output pointer for quote text (caller must free)
- `author`: Output pointer for author name (caller must free)

**Returns**:
- ESP_OK: Success
- ESP_FAIL: HTTP error or parsing error

**Flow**:
```
# Configure HTTP client
esp_http_client_config_t config = {
    .url = "http://quotes-api-three.vercel.app/random",
    .method = HTTP_METHOD_GET,
    .timeout_ms = 10000
}

client = esp_http_client_init(&config)

# Perform request
esp_http_client_perform(client)

# Check status
status = esp_http_client_get_status_code(client)
if status != 200:
    return ESP_FAIL

# Read response body
buffer = malloc(4096)
length = esp_http_client_read(client, buffer, 4096)

# Parse JSON
Find "quote": " ... "
Find "author": " ... "

# Allocate and copy
*quote = strdup(quote_value)
*author = strdup(author_value)

cleanup:
free(buffer)
esp_http_client_cleanup(client)

return ESP_OK
```

**JSON Response Example**:
```json
{
  "quote": "La vita è quella cosa che accade mentre tu stai facendo altri progetti.",
  "author": "John Lennon"
}
```

**Error Handling**:
- Network timeout → Return ESP_FAIL
- Non-200 status → Return ESP_FAIL
- JSON parse error → Use fallback quote: "Quote fetch error" / "Unknown"

---

### Module 6: sleep_manager.c / sleep_manager.h

**Purpose**: Deep sleep management with multiple wake sources

**Responsibilities**:
- Configure GPIO wake buttons
- Enter deep sleep with timer
- Detect wake source on boot

**Wake Sources**:
1. **Timer**: Random interval (10-60 minutes)
2. **EXT0 (GPIO 39)**: Quote refresh button
3. **EXT1 (GPIO 35)**: Network reset button

**Key Functions**:

#### `void sleep_manager_init()`
Initialize sleep manager and configure GPIO.

**Actions**:
```
# Configure GPIO 39 as input with pull-up
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_39),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
}
gpio_config(&io_conf)

# Configure GPIO 35 as input with pull-up
io_conf.pin_bit_mask = (1ULL << GPIO_NUM_35)
gpio_config(&io_conf)
```

**Note**: Both buttons are active-low (pressed = 0, released = 1)

#### `void sleep_manager_enter_deep_sleep(uint32_t sleep_seconds)`
Enter deep sleep with timer and button wake sources.

**Parameters**:
- `sleep_seconds`: Sleep duration in seconds

**Actions**:
```
# Enable timer wake
esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000ULL)

# Enable GPIO 39 wake (EXT0)
esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0)  # Wake on LOW

# Enable GPIO 35 wake (EXT1)
gpio_mask = (1ULL << GPIO_NUM_35)
esp_sleep_enable_ext1_wakeup(gpio_mask, ESP_EXT1_WAKEUP_ALL_LOW)

Log sleep duration and wake sources

esp_deep_sleep_start()  # Enter sleep (never returns)
```

#### `bool sleep_manager_is_wakeup_from_sleep()`
Check if device woke from deep sleep (vs cold boot).

**Returns**: `true` if wake from sleep, `false` if cold boot

**Implementation**:
```
wake_cause = esp_sleep_get_wakeup_cause()
return wake_cause != ESP_SLEEP_WAKEUP_UNDEFINED
```

#### `bool sleep_manager_is_wakeup_from_button()`
Check if wake source was GPIO 39 button.

**Returns**: `true` if GPIO 39 triggered wake

**Implementation**:
```
return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0
```

#### `bool sleep_manager_is_wakeup_from_reset_button()`
Check if wake source was GPIO 35 reset button.

**Returns**: `true` if GPIO 35 triggered wake

**Implementation**:
```
return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1
```

**Wake Cause Values**:
- `ESP_SLEEP_WAKEUP_UNDEFINED`: Cold boot
- `ESP_SLEEP_WAKEUP_TIMER`: Timer expired
- `ESP_SLEEP_WAKEUP_EXT0`: GPIO 39 button
- `ESP_SLEEP_WAKEUP_EXT1`: GPIO 35 button

---

### Module 7: gerunds.c / gerunds.h

**Purpose**: Provide random loading screen text

**Contents**:
- Array of 89 gerund words
- Random selection function

**Key Functions**:

#### `const char* get_random_gerund()`
Return random gerund from list.

**Returns**: Pointer to gerund string (e.g., "Thinking")

**Implementation**:
```
random_index = esp_random() % GERUNDS_COUNT
return gerunds[random_index]
```

**Gerund List** (89 words):
Accomplishing, Actioning, Actualizing, Baking, Booping, Brewing, Building, Calculating, Calibrating, Cogitating, Compiling, Computing, Concocting, Configuring, Connecting, Constructing, Contemplating, Cooking, Crafting, Creating, Deliberating, Designing, Developing, Devising, Doing, Engineering, Envisioning, Executing, Fabricating, Formulating, Generating, Grinding, Ideating, Implementing, Improvising, Innovating, Inventing, Iterating, Launching, Machinating, Making, Manufacturing, Musing, Operating, Orchestrating, Organizing, Performing, Planning, Plotting, Pondering, Preparing, Producing, Programming, Realizing, Reasoning, Reflecting, Rendering, Resolving, Ruminating, Running, Scheming, Simulating, Solving, Speculating, Strategizing, Structuring, Synthesizing, Systemizing, Theorizing, Thinking, Tinkering, Troubleshooting, Tuning, Understanding, Visualizing, Whipping, Wizarding, Working, Wrangling

---

### Module 8: battery.c / battery.h

**Purpose**: Battery voltage monitoring via ADC

**Responsibilities**:
- Initialize ADC1 with calibration for accurate voltage readings
- Read battery voltage from GPIO 36 with 2:1 voltage divider compensation
- Calculate battery percentage based on Li-ion 18650 discharge curve
- Save readings to NVS for debugging when running on battery power
- Provide graceful error handling for display integration

**Hardware Details**:
- **GPIO**: 36 (ADC1_CHANNEL_0)
- **Voltage Divider**: 2:1 ratio (2x 100kΩ resistors, hardware-integrated)
- **ADC Resolution**: 12-bit (0-4095 raw values)
- **Attenuation**: 11dB (0-3.3V input range)
- **Calibration**: eFuse vref for production accuracy
- **Sampling**: 64 samples averaged with 2ms inter-sample delays
- **Stabilization**: 100ms delay after EPD power-on
- **Li-ion Range**: 3.0V (0% - safe cutoff) to 4.2V (100%)
- **Nominal Voltage**: 3.7V (~50-65%)

**Key Functions**:

#### `esp_err_t battery_init(void)`
Initialize battery voltage monitoring.

**Actions**:
```
# Configure ADC1 width (12-bit resolution)
adc1_config_width(ADC_WIDTH_BIT_12)

# Configure ADC1 channel 0 (GPIO 36) with 11dB attenuation
adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11)

# Characterize ADC using eFuse vref
esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars)

# Log calibration method used (eFuse vref, Two Point, or Default)
```

**Returns**:
- ESP_OK on success
- ESP error code on failure

#### `float battery_read_voltage(void)`
Read raw battery voltage in volts.

**Algorithm**:
```
if not initialized:
    return -1.0

# Power on EPD to enable voltage divider circuit
epd_poweron()
delay(100ms)  # Increased stabilization time for voltage settling

# Read and average 64 ADC samples with delays between samples
adc_sum = 0
for i in 0..63:
    raw = adc1_get_raw(ADC1_CHANNEL_0)
    if raw < 0:
        epd_poweroff()
        return -1.0
    adc_sum += raw

    # Small delay between samples to allow ADC to settle
    if i < 63:
        delay(2ms)

adc_average = adc_sum / 64

# Power off EPD
epd_poweroff()

# Convert ADC reading to millivolts using calibration
voltage_mv = esp_adc_cal_raw_to_voltage(adc_average, &adc_chars)

# Compensate for 2:1 voltage divider
actual_voltage = (voltage_mv / 1000.0) * 2.0

# Cache raw data for NVS logging
last_reading.adc_raw = adc_average
last_reading.voltage_mv = voltage_mv
last_reading.actual_voltage = actual_voltage

return actual_voltage
```

**Returns**: Battery voltage in volts (3.0-4.2V typical range), or -1.0 on error

#### `float battery_read_percentage(void)`
Read battery percentage (0-100%).

**Algorithm**:
```
voltage = battery_read_voltage()

if voltage < 0:
    return -1.0

# Linear mapping from Li-ion 18650 voltage curve
# 3.0V = 0% (safe discharge cutoff)
# 3.7V = ~50-65% (nominal voltage)
# 4.2V = 100% (fully charged)
percentage = ((voltage - 3.0) / (4.2 - 3.0)) * 100.0

# Clamp to valid range (handles charging spikes and deep discharge)
if percentage > 100.0:
    percentage = 100.0
else if percentage < 0.0:
    percentage = 0.0

# Save complete reading to NVS for debugging
last_reading.percentage = percentage
last_reading.timestamp = current_time()

# Save to NVS namespace "battery_log"
nvs_set_blob("last_reading", &last_reading, sizeof(battery_reading_t))

return percentage
```

**Returns**: Battery percentage (0-100), or -1.0 on error

**Power Management**:
- Battery reading requires EPD power rail active for entire duration
- Stabilization: 100ms after EPD power-on
- Sampling: 64 samples × 2ms = 128ms
- Total read time: ~228ms (100ms stabilization + 128ms sampling)
- Power consumption: ~10-15mA during reading
- Timing: Read after WiFi connects but before SNTP/HTTP (minimal interference)
- Impact: Negligible vs total wake cycle (~15-30 seconds)

**Error Handling**:
- Returns -1.0 on all error conditions
- WiFi manager displays "batt: --%" when percentage < 0
- All errors logged with ESP_LOGE or ESP_LOGW
- Module continues to work after transient ADC errors

**Calibration Types**:
1. **eFuse Vref**: Factory-calibrated reference voltage stored in eFuse (best accuracy)
2. **Two Point**: Factory two-point calibration (good accuracy)
3. **Default Vref**: Fallback to 1100mV reference (acceptable accuracy)

**Note**: Uses deprecated ESP-IDF ADC APIs (`driver/adc.h`, `esp_adc_cal.h`) for compatibility with EPDiy library patterns. Consider migrating to `esp_adc/adc_oneshot.h` in future versions.

---

## API Reference

### Display UI API

```c
// Initialize e-paper display hardware
void display_init(void);

// Show WiFi provisioning instructions with AP name
void display_provisioning_mode(const char* ap_name);

// Show quote with author and metadata
void display_connected_mode(const char* quote, const char* author,
                           const char* datetime_text);

// Show WiFi connection attempt
void display_connecting(const char* ssid);

// Show random gerund loading screen
void display_loading(const char* gerund);

// Show network reset confirmation prompt
void display_reset_confirmation(void);
```

### WiFi Manager API

```c
// Initialize WiFi subsystem and event handlers
esp_err_t wifi_manager_init(void);

// Start WiFi (check creds, connect or provision)
esp_err_t wifi_manager_start(bool silent);

// Save WiFi credentials to NVS
esp_err_t wifi_manager_save_credentials(const char* ssid,
                                       const char* password);

// Get total quote count from NVS
uint32_t wifi_manager_get_quote_count(void);

// Increment quote counter in NVS
esp_err_t wifi_manager_increment_quote_count(void);

// Delete WiFi credentials from NVS
esp_err_t wifi_manager_delete_credentials(void);
```

### Web Server API

```c
// Start HTTP server on port 80
httpd_handle_t start_webserver(void);

// Stop HTTP server
void stop_webserver(httpd_handle_t server);
```

### Quote Fetch API

```c
// Fetch random quote from API
// Caller must free returned strings
esp_err_t wikiquote_fetch(char** quote, char** author);
```

### Sleep Manager API

```c
// Initialize GPIO and sleep configuration
void sleep_manager_init(void);

// Enter deep sleep with timer and button wake
void sleep_manager_enter_deep_sleep(uint32_t sleep_seconds);

// Check if woke from deep sleep (vs cold boot)
bool sleep_manager_is_wakeup_from_sleep(void);

// Check if GPIO 39 triggered wake
bool sleep_manager_is_wakeup_from_button(void);

// Check if GPIO 35 triggered wake
bool sleep_manager_is_wakeup_from_reset_button(void);
```

### Gerunds API

```c
// Get random gerund word for loading screen
const char* get_random_gerund(void);
```

### Battery API

```c
// Initialize battery voltage monitoring
// Configures ADC1 Channel 0 (GPIO 36) with calibration
// NOTE: Must be called before battery_read_percentage()
esp_err_t battery_init(void);

// Read battery voltage in volts
// Powers on EPD, reads ADC with 100ms stabilization and 2ms inter-sample delays
// Accounts for 2:1 voltage divider on hardware
// Caches raw data for NVS logging
// Returns: Battery voltage in volts, or -1.0 on error
float battery_read_voltage(void);

// Read battery percentage (0-100%)
// Powers on EPD voltage divider, reads ADC, calculates percentage
// Uses averaged readings (64 samples with 2ms delays) for stability
// Automatically saves reading to NVS with timestamp for debugging
// Li-ion range: 3.0V (0%) to 4.2V (100%), nominal 3.7V (~50-65%)
// Returns: Battery percentage (0-100), or -1.0 on error
float battery_read_percentage(void);

// Get last battery reading from NVS
// Useful for debugging when serial console not available
// Returns: ESP_OK if reading retrieved, ESP_ERR_NVS_NOT_FOUND if none stored
esp_err_t battery_get_last_reading(battery_reading_t* reading);

// Print last battery reading from NVS to console
// Automatically called at startup to show previous battery state
// Displays: timestamp, ADC raw, ADC mV, actual voltage, percentage
void battery_print_last_reading(void);
```

---

## Data Structures

### NVS Storage Layout

```c
// Namespace: "wifi_config"
typedef struct {
    char ssid[32];           // WiFi SSID (null-terminated)
    char password[64];       // WiFi password (null-terminated)
    uint32_t quote_count;    // Total quotes displayed
} wifi_config_t;

// Namespace: "battery_log"
typedef struct {
    time_t timestamp;        // Unix timestamp of reading
    uint32_t adc_raw;        // Raw ADC value (0-4095)
    uint32_t voltage_mv;     // Voltage in millivolts (after calibration)
    float actual_voltage;    // Actual battery voltage (after divider compensation)
    float percentage;        // Battery percentage (0-100)
} battery_reading_t;
```

### Quote Data

```c
typedef struct {
    char* quote;    // Dynamically allocated quote text
    char* author;   // Dynamically allocated author name
} quote_data_t;
```

### Font Properties

```c
typedef struct {
    uint8_t fg_color;        // Foreground (0=black, 15=white)
    uint8_t bg_color;        // Background (0=black, 15=white)
    int fallback_glyph;      // Glyph for missing characters
    int flags;               // Rendering flags
} EpdFontProperties;
```

### Display Rectangle

```c
typedef struct {
    int x;          // X coordinate (top-left)
    int y;          // Y coordinate (top-left)
    int width;      // Width in pixels
    int height;     // Height in pixels
} EpdRect;
```

---

## Configuration

### GPIO Pin Mapping

```c
// Wake buttons
#define WAKEUP_BUTTON_GPIO    GPIO_NUM_39  // Quote refresh
#define RESET_BUTTON_GPIO     GPIO_NUM_35  // Network reset

// Button characteristics
// - Active LOW (pressed = 0, released = 1)
// - Internal pull-up enabled
// - Wake on falling edge

// Battery monitoring
#define BATT_PIN_GPIO         36           // ADC1_CHANNEL_0
#define BATT_ADC_CHANNEL      ADC1_CHANNEL_0
#define BATT_VOLTAGE_DIVIDER  2.0          // Hardware 2:1 divider
#define BATT_MIN_VOLTAGE      3.0          // Li-ion 0% (safe cutoff)
#define BATT_MAX_VOLTAGE      4.2          // Li-ion 100%
#define BATT_SAMPLES          64           // Average 64 readings (with 2ms delays)
```

### Network Configuration

```c
// Access Point (Provisioning Mode)
#define AP_SSID_PREFIX       "WMQuote_"    // + MAC last byte
#define AP_PASSWORD          ""             // Open network
#define AP_MAX_CONN          1
#define AP_CHANNEL           1
#define AP_IP                "192.168.4.1"

// Station Mode
#define WIFI_MAXIMUM_RETRY   3              // Connection retries
#define WIFI_CONNECT_TIMEOUT 30000          // 30 seconds
```

### Sleep Configuration

```c
#define MIN_SLEEP_MINUTES    10             // Minimum sleep
#define MAX_SLEEP_MINUTES    60             // Maximum sleep
```

### Display Configuration

```c
// Display dimensions
#define DISPLAY_WIDTH        960
#define DISPLAY_HEIGHT       540
#define DISPLAY_ROTATION     EPD_ROT_LANDSCAPE

// Update mode
#define UPDATE_MODE          MODE_GC16      // 16 grayscale
#define TEMPERATURE          25             // Celsius

// Quote layout
#define QUOTE_MAX_WIDTH      860            // Pixels
#define QUOTE_LINE_HEIGHT    35             // Pixels
#define QUOTE_START_Y        150            // Top margin

// Logo positions
#define LOGO_256_X           80
#define LOGO_256_Y           142            // (540-256)/2
#define LOGO_64_X            886            // 960-64-10
#define LOGO_64_Y            466            // 540-64-10

// Text positions
#define STATUS_TEXT_X        10             // Bottom-left
#define STATUS_TEXT_Y        525            // 540-15
```

### API Configuration

```c
// Quote API
#define QUOTE_API_URL        "http://quotes-api-three.vercel.app/random"
#define QUOTE_API_TIMEOUT    10000          // 10 seconds

// SNTP
#define SNTP_SERVER          "pool.ntp.org"
#define SNTP_TIMEZONE        1              // UTC+1 (Europe/Rome)
#define SNTP_SYNC_TIMEOUT    30000          // 30 seconds
```

### Reset Configuration

```c
#define RESET_TIMEOUT_MS     10000          // 10 seconds
#define RESET_PRESS_COUNT    3              // Required presses
#define RESET_POLL_INTERVAL  50             // Poll every 50ms
#define RESET_DEBOUNCE_MS    10             // Debounce delay
```

---

## Build Information

### ESP-IDF Version
- **Version**: v5.4.1
- **Toolchain**: xtensa-esp32-elf-gcc 13.2.0

### Memory Usage
- **Flash**: ~1.46 MB / 3 MB (48%)
- **SRAM**: ~45 KB / 520 KB (data + bss)
- **PSRAM**: ~500 KB (framebuffer)

### Key Dependencies
- `epdiy`: E-paper driver library
- `nvs_flash`: Non-volatile storage
- `esp_wifi`: WiFi stack
- `esp_http_server`: HTTP server
- `esp_http_client`: HTTP client
- `esp_event`: Event loop
- `lwip`: TCP/IP stack
- `esp_sntp`: SNTP client

### Build Commands
```bash
# Configure
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor

# Erase NVS (for testing)
idf.py erase-flash
```

---

## Debugging

### Log Tags
```c
"MAIN"          // Main application
"DISPLAY_UI"    // Display rendering
"WIFI_MANAGER"  // WiFi operations
"WEBSERVER"     // HTTP server
"WIKIQUOTE"     // Quote API
"SLEEP_MANAGER" // Deep sleep
```

### Common Issues

#### Issue: Display shows garbage
**Cause**: Framebuffer not cleared
**Solution**: Call `epd_clear()` before drawing

#### Issue: WiFi won't connect
**Cause**: Saved credentials incorrect
**Solution**: Use GPIO 35 reset to clear credentials

#### Issue: Quote not fetching
**Cause**: SNTP time not synced
**Solution**: Check logs for SNTP timeout, verify network

#### Issue: Random sleep not working
**Cause**: esp_random() not seeded
**Solution**: WiFi init automatically seeds RNG

#### Issue: Button wake not working
**Cause**: GPIO pull-up not configured
**Solution**: Verify `sleep_manager_init()` called

---

## License

This project uses ESP-IDF (Apache 2.0) and EPDiy (MIT).

---

**Last Updated**: 2024-12-28
**Version**: 1.0
**Author**: Antonio
