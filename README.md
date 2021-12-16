# Soul Injector

Offline firmware downloader for ARM Cortex-M, on an ESP32-S3.

## To-do list - Major building blocks

- [x] Run flash algorithm
- [x] Offline firmware flashing
- [x] More tests
- [x] Read verification
- [x] Desktop app (WIP): see [huming2207/soul-composer-app](https://github.com/huming2207/soul-composer-app)
- [x] USB CDC-ACM communication (WIP, almost there)
- [ ] SWD on SPI Master, instead of bit-banging

## To-do list - other platforms except SWD

- [ ] ESP32 Bootloader UART firmware downloading 
- [ ] GD32VF103 RISC-V, over JTAG??
- [ ] CH32VF103 RISC-V, over SWD/cJTAG?

## Post-MVP feature list

- [ ] Basic BLE functionality
- [ ] Compression on BLE transmission
- [ ] OCD server over BLE?
- [ ] USB DAP-Link probe? (but why not buy one from Taobao instead?)
- [ ] Abandon CMSIS-Pack's flash algorithm, move to my own impl of flash algorithm (to get it even faster)

## License

This project (except 3rd-party libraries in `components` directory) has two types of licenses:
  - AGPL-3.0 for non-commercial purposes
  - Commercial licenses for any commercial purposes, speak to me (the author, Jackson Ming Hu, huming2207 at gmail.com) for more details.

