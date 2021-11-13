# Soul Injector

Offline firmware downloader for ARM Cortex-M, on an ESP32(-S2/3).

Tested on ESP32-S2 only but it should work on ESP32-S3.

## To-do list - subject to change

- [x] Run flash algorithm
- [x] Offline firmware flashing
- [x] More tests
- [x] Read verification
- [-] Desktop app (Tauri + Rust + React web stuff)
- [-] USB CDC-ACM communication
- [ ] SWD on SPI Master, instead of bit-banging
- [ ] ESP32-to-ESP32 firmware downloading 

## Post-MVP feature list

- [ ] Basic BLE functionality
- [ ] Compression on BLE transmission
- [ ] OCD server over BLE?
- [ ] USB DAP-Link probe? (but why not buy one from Taobao instead?)
- [ ] Abandon CMSIS-Pack's flash algorithm, move to my own impl of flash algorithm (to get it even faster)

## License

This project (except 3rd-party libraries in `components` directory) has two types of licenses:
  - GPLv3 for non-commercial purposes
  - Commercial licenses for any commercial purposes, speak to me (the author) for more details.

