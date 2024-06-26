> :bell: continued in [https://github.com/calint/platformio-bam](https://github.com/calint/platformio-bam)

<video width="240" height="320" src="https://github.com/calint/bam/assets/1920811/1e1f7bb0-4d1e-4ce2-937a-10c6ac176b2e"></video>

# ESP32-2432S028R
a.k.a. cheap-yellow-display (CYD)

### about the device
* [community](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
* [purchased at](https://www.aliexpress.com/item/1005004502250619.html)
* [manufacturer](http://www.jczn1688.com/)

### intention
* developing using arduino framework with visual code and platformio
* exploring the device by developing a toy game
* developing a platform-independent toy game engine featuring:
  - smooth scrolling tile map
  - sprites in layers with pixel precision on-screen collision detection
  - intuitive definition of game objects and logic
  - decent performance, ~30 frames per second on the device

### development environment
* Visual Code 1.89.0
* PlatformIO 6.1.15
* Espressif 32 (6.6.0) > Espressif ESP32 Dev Module
* packages:
  - framework-arduinoespressif32 @ 3.20014.231204 (2.0.14) 
  - tool-esptoolpy @ 1.40501.0 (4.5.1) 
  - toolchain-xtensa-esp32 @ 8.4.0+2021r2-patch5
* dependencies:
  - SPI @ 2.0.0
  - TFT_eSPI @ 2.5.43
  - XPT2046_Touchscreen @ 1.4
