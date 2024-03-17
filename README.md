# ec_tpfan

This is forked from https://github.com/Soberia/EmbeddedController

All interfaces are consistent with no change , just parameterized special methods for read / write EC's registers.

## **📋 Compile / Build in Visual Studio 2019**
You need to compile your app with `c++17` flag (this is completed already in app property settings)
This app uses `WinRing0` driver to access the hardware, make sure you place `WinRing0x64.sys` or `WinRing0.sys` beside your binary files.
Your program has to run with administrator privileges to work properly.

## Usage
Please turn on the driver signature test before use. 
```
bcdedit -set TESTSIGNING ON
```
**Note:** If BitLock is turned on, after restarting, it will ask for the key to enter! (Back up the BitLock key to a USB flash drive first)

Without any parameters, you will be prompted for usage:

Please forgive me for my poor English and coding skills. I am too lazy to change it at the moment.
```
Usage: [-dump] [-rpm] [-r {port}] [-w {port} {value}]

-dump displays the current values (hexadecimal) of all operable registers of EC (range 0x00 to 0xFF)
-rpm displays the current fan speed (only for Thinkpad E16 Gen 1 Intel model, not sure about other models, the main principle is to read the register 0x84, 0x85 values)
-r {port} Here {port} is replaced with a register address (0x00 to 0xFF) you want to read, and the read data is displayed in hexadecimal.
-w {port} {value} Here {port} is replaced with a register address you want to write (0x00 to 0xFF), and {value} is replaced with the value you want to write (1 byte, expressed in hexadecimal, Range is 0x00 to 0xFF)
```

## How to control Thinkpad E16 Fan RPM
Here are some practical examples. They are only valid for Thinkpad E16 and are not guaranteed to be valid for other laptops. Please combine the analysis principles of this article to find out how to manually control the fan speed in your laptop's EC register!

Lao Wang wants to adjust the E16 fan speed to a maximum of 5800 RPM (the maximum is only 4100 RPM in automatic mode), execute the command:
```
ec.exe -w 0x2f 0x40
```
Lao Wang wants to adjust the E16 fan speed to the lowest manual gear of 2500 RPM, execute the command:
```
ec.exe -w 0x2f 0x01
```
From gear 1 to gear 7, change the last parameter to 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07

Lao Wang wants to turn off the E16 fan and execute the command: (Danger! This is a manual mode. After turning off the fan, the CPU temperature rises to 100 degrees, and the EC will not automatically turn on the fan, which risks burning the CPU!)
```
ec.exe -w 0x2f 0x00
```

Lao Wang wants to restore EC automatic fan adjustment, execute the command:
```
ec.exe -w 0x2f 0x80
```

# **Disclaimer: I am not responsible for any hardware damage caused by modifying the EC register with this program!**
