# ATC_TLSR_Paper
Custom BLE firmware for Hanshow E-Paper Shelf Labels / Price Tags using the TLSR8359 ARM SOC

### You can support my work via PayPal: https://paypal.me/hoverboard1 this keeps projects like this coming.

As BLE can be enabled on the Telink TLSR8359 so i decided to make a custom firmware instead of reversing the stock firmware and its 2.4Ghz RF Protocol

This repo is made together with this explanation video:(click on it)

[![YoutubeVideo](https://img.youtube.com/vi/ANHz7EgWx7k/0.jpg)](https://www.youtube.com/watch?v=ANHz7EgWx7k)


WebSerial Firmware flasher Tool: 
https://atc1441.github.io/ATC_TLSR_Paper_UART_Flasher.html

WebBluetooth Image Uploader:
https://atc1441.github.io/ATC_TLSR_Paper_Image_Upload.html

WebBluetooth Firmware OTA Flashing:
https://atc1441.github.io/ATC_TLSR_Paper_OTA_writing.html

#### Compiling:
Python needs to be installed
##### Windows:
To compile under windows navigate with a command prompt to the "Firmware" folder

Enter "makeit.exe" and wait till the Compiling is done.


##### Linux:
Navigate with a Terminal into the "Firmware" Folder

Enter "make" and wait till the Compiling is done.

#### Flashing:
Open the Compiled .bin firmware with the WebSerial Flasher and write it to Flash.

On first Connection it is needed to Unlock the flash of the TLSR8359

#### About the display:
The e-ink panel used in this ESL is 250 by 122 pixels, black and white (no gray levels...yet).

Larry Bank added his OneBitDisplay (https://github.com/bitbank2/OneBitDisplay) and TIFF_G4 (https://github.com/bitbank2/TIFF_G4) libraries to make it easy to generate text and graphics. For anyone wanting to write directly to the display buffer, the memory is laid out like a typical 1-bpp bitmap except that it is rotated 90 degrees clockwise. In other words, the display is really 122 wide by 250 tall, but laying on its side. Each byte contains 8 pixels with the most significant bit on the left. Black is 0 and white is 1. Each row of 122 pixels uses 16 bytes. Here is an example function to set a pixel given the x,y of the orientation (portrait) that the display is used:<br>
<br>
```
void SetPixel(int x, int y, uint8_t *pDisplayBuffer)
{
   uint8_t *d = &pDisplayBuffer[(y >> 3) + (249-x)*16];
   *d &= ~(0x80 >> (y & 7)); // set pixel to black
}
```
