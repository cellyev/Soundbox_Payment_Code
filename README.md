# 🔊 Soundbox Payment IoT Device

Proyek ini adalah bagian dari **Tugas Akhir kelas XII** dengan judul:  
**"Alat Soundbox Payment Berbasis IoT dengan Pengembangan Web Penjualan dan Payment Gateway."**

Kode ini merupakan program utama yang dijalankan pada perangkat IoT bernama **Soundbox Payment**. Perangkat ini digunakan untuk memberikan notifikasi suara secara otomatis ketika pembayaran digital berhasil dilakukan. Sistem ini membantu pelaku usaha menerima konfirmasi pembayaran secara **real-time** tanpa harus mengecek layar.

---

## 🎯 Fitur Utama

- ✅ Notifikasi suara otomatis saat pembayaran berhasil diterima.
- 🔄 Replay suara transaksi terakhir melalui tombol.
- 🔊 Pengaturan volume suara menggunakan tombol.
- 🔗 Terhubung ke server melalui WiFi dan API berbasis HTTP.
- ⚡ Fetch data transaksi secara berkala (real-time polling).

---

## 📦 Teknologi yang Digunakan

- **Mikrokontroler:** ESP32 / NodeMCU
- **Bahasa:** Arduino C++
- **Koneksi:** WiFi
- **Output:** Speaker / Buzzer
- **Input:** Tombol (Replay, Volume)
- **Tambahan (opsional):** OLED Display

---

## 🔁 Skenario Kerja & Fetch API

### Alur Umum

1. Perangkat menyambung ke jaringan WiFi yang telah dikonfigurasi.
2. Secara berkala, perangkat mengirim permintaan (`GET`) ke server untuk mengecek apakah ada transaksi baru.
3. Jika ditemukan transaksi baru:
   - Perangkat mengubah nominal menjadi teks audio.
   - Notifikasi suara diputar melalui speaker.
   - Informasi transaksi terakhir disimpan untuk fitur replay.
4. Tombol `Replay` akan memutar ulang suara transaksi terakhir.
5. Tombol `Volume` akan menaikkan atau menurunkan level volume sesuai logika yang telah diatur.
