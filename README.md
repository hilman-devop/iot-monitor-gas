# IoT Monitor Gas System

![Arduino](https://img.shields.io/badge/Arduino-2f81f7?style=flat) ![ESP8266](https://img.shields.io/badge/ESP8266-2f81f7?style=flat) ![Blynk](https://img.shields.io/badge/Blynk-2f81f7?style=flat) ![MQ-6](https://img.shields.io/badge/MQ-6-2f81f7?style=flat) ![MQ-135](https://img.shields.io/badge/MQ-135-2f81f7?style=flat) ![PlatformIO](https://img.shields.io/badge/PlatformIO-2f81f7?style=flat)

> Prototipe monitoring gas dan kualitas udara dengan sensor, alarm lokal, serta dashboard jarak jauh.

## Tentang proyek

Prototipe monitoring gas dan kualitas udara dengan sensor, alarm lokal, serta dashboard jarak jauh. Dokumentasi ini dibuat sebagai panduan singkat untuk memahami ruang lingkup proyek, menjalankannya secara lokal, dan membaca batasan penggunaannya.

## Fitur utama

- Pembacaan sensor MQ-6 dan MQ-135
- Telemetri ke Blynk
- Kalibrasi dan auto-baseline
- Alarm lokal untuk kondisi berbahaya
- Struktur proyek PlatformIO

## Teknologi

- Arduino
- ESP8266
- Blynk
- MQ-6
- MQ-135
- PlatformIO

## Menjalankan secara lokal

Prasyarat dan konfigurasi dapat berbeda menurut sistem operasi. Contoh langkah utama:

```bash
Buka folder di PlatformIO, sesuaikan konfigurasi board, lalu upload firmware.
```

## Struktur dan pengembangan

Mulai dari entry point dan folder konfigurasi proyek. Pisahkan credential lokal dari source code, tambahkan test untuk perubahan penting, dan gunakan environment variable untuk konfigurasi deployment.

## Keamanan

- Jangan commit API key, password, token, session ID, atau data pribadi.
- Gunakan file environment lokal dan contoh konfigurasi tanpa nilai rahasia.
- Uji perubahan pada environment development sebelum deployment.

## Status

Proyek ini merupakan bagian dari portofolio **Hilman Arifin Ilham** dan dapat dikembangkan sesuai kebutuhan penggunaan.

> **Catatan:** Token Blynk dan credential Wi-Fi harus disimpan di konfigurasi lokal yang tidak di-commit.

## Lisensi

Belum ada lisensi open-source yang dinyatakan di repository ini. Hubungi pemilik repository sebelum menggunakan ulang atau mendistribusikan kode.

---

Dibuat oleh [Hilman Arifin Ilham](https://github.com/hilman-devop).
