# ATC_TLSR_Paper
Custom BLE firmware for Hanshow E-Paper Shelf Labels / Price Tags using the TLSR8359 ARM SOC

### You can support my work via PayPal: https://paypal.me/hoverboard1 this keeps projects like this coming.

As BLE can be enabled on the Telink TLSR8359 so i decided to make a custom firmware instead of reversing the stock firmware and its 2.4Ghz RF Protocol


WebSerial Firmware flasher Tool: 
https://atc1441.github.io/ATC_TLSR_Paper_UART_Flasher.html

#### Compiling:
Python needs to be installed
##### Windows:
To compile under windows navigate with a command prompt to the "Firmware" folder
Enter "makeit.exe" and wait till the Compiling is done.

##### Linux:
Navigate with a Terminal into the "Firmware" Folder
Enter "chmod 777 -R ./" 
Enter "make" and wait till the Compiling is done.

#### Flashing:
Open the Compiled .bin firmware with the WebSerial Flasher and write it to Flash.

On first Connection it is needed to Unlock the flash of the TLSR8359

