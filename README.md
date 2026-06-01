# Industrial Extrusion Monitoring System

Industrial embedded control system based on ESP32-S3 for real-time extrusion process automation, production monitoring and industrial data logging.

Originally developed for rubber profile extrusion lines operating in industrial environments.

<p align="center">
  <img src="docs/images/home.jpg" width="900">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP--IDF-v5.5-blue">
  <img src="https://img.shields.io/badge/LVGL-v9-green">
  <img src="https://img.shields.io/badge/MCU-ESP32--S3-red">
  <img src="https://img.shields.io/badge/Industrial-HMI-orange">
  <img src="https://img.shields.io/badge/WiFi-Enabled-0090E7">
</p>

---

# Overview

DeepMove is an industrial HMI and monitoring platform designed for extrusion production lines.

The controller integrates:

* Industrial touchscreen interface
* Real-time production monitoring
* SD card production logging
* WiFi connectivity
* Web dashboard
* OTA firmware updates
* Industrial relay and sensor interfaces
* Modular ESP-IDF firmware architecture

The system operates directly on the production machine and continuously calculates extrusion metrics in real time.

---

# Main Features

* Industrial touchscreen HMI
* Real-time extrusion speed calculation
* Produced length measurement
* Automatic relay actuation for marking/cutting
* Production profile management
* SD card production logging
* Historical statistics
* WiFi production dashboard
* Integrated HTTP API
* OTA firmware update support
* Automatic firmware rollback
* Modular scalable architecture
* ESP-IDF based firmware

---

# Industrial Application

The controller was designed for rubber profile extrusion systems.

The system:

* Detects linear material movement using an inductive sensor
* Calculates extrusion speed and production statistics
* Controls paint marking or automatic cutting systems
* Stores production history
* Generates process alarms and warnings
* Allows complete profile-based configuration

---

# User Interface

## Main Screen

<p align="center">
<img src="docs/images/home.jpg" width="750">
</p>

---

## Profile Selection

<p align="center">
<img src="docs/images/profiles.jpg" width="750">
</p>

---

## Profile Information

<p align="center">
<img src="docs/images/profile.jpg" width="750">
</p>

---

## Production Ready State

<p align="center">
<img src="docs/images/ready.jpg" width="750">
</p>

---

## Production Recording

<p align="center">
<img src="docs/images/recording.jpg" width="750">
</p>

---

## System Settings

<p align="center">
<img src="docs/images/system.jpg" width="750">
</p>

---

# Web Dashboard

## Production Dashboard

<p align="center">
<img src="docs/images/dashboard.jpg" width="1000">
</p>

---

## Profile Creation

<p align="center">
<img src="docs/images/create.jpg" width="1000">
</p>

---

# Mechanical Integration

## Paint Marking Machine

<p align="center">
<img src="docs/images/painting.jpg" width="450">
</p>

---

# Hardware

* ESP32-S3
* 800x480 RGB touchscreen display
* LVGL graphics framework
* Integrated WiFi
* SD Card storage
* RTC
* Relay outputs
* 24V opto-isolated industrial I/O
* EMI filtered power input

---

# Production Metrics

The system continuously calculates:

* Instantaneous speed
* Average speed
* Produced meters
* Produced kilograms
* Cut quantity
* Runtime and downtime
* Daily / weekly / monthly / yearly statistics
* Process efficiency
* Production history

---

# Software Architecture

The firmware is based on a modular ESP-IDF architecture.

Main modules:

* BSP
* UI
* Drivers
* Production logic
* Storage
* WiFi communication
* HTTP server
* OTA update system

---

# Build

## Requirements

* ESP-IDF v5.5.x
* Python
* Ninja
* Xtensa Toolchain

---

## Compile

```bash
idf.py build
```

---

## Flash

```bash
idf.py flash monitor
```

---

# OTA Firmware

Firmware releases:

https://github.com/Meina88/integral-controller/releases

---

# Disclaimer

This project was originally developed for private industrial applications related to rubber profile extrusion systems.

The author does not guarantee the suitability of this software or hardware for critical, safety-related or commercial production environments.

This repository may contain experimental features, work-in-progress modules and incompatible changes between versions.

Use this system entirely at your own risk.

---

# Development

## Pablo Meinardo

Mechanical Engineer | Embedded Systems | Industrial Automation

* Mechanical Engineer — UNRC
* Instructional Design — UTN
* Industry 4.0 — UNSAM

---

# Contact

📱 WhatsApp
https://wa.me/5493586547097

📧 Email
[meinardop@gmail.com](mailto:meinardop@gmail.com)

📍 Río Cuarto, Córdoba, Argentina
