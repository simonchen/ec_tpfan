# ec_tpfan

This is forked from [EmbeddedController](https://github.com/Soberia/EmbeddedController)

All interfaces are consistent with no change , just build console app that have parameterized special methods for read / write EC's registers.
Especially, it has ability to control Fan RPM special for Thinkpad E16.

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
Usage: ec.exe [-dump] [-rpm] [-r {port}] [-w {port} {value}]

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

# Theory 
If you want to manually control the Thinkapd E16 fan speed, you need to prepare:

Laptop EC monitoring software [RWEverything](http://rweverything.com/download/)
Before installation, please turn off "Core Isolotion > Memory integrity" in Windows 10/11, run the following command with administrator privileges to allow driver signature testing, and then restart the machine.
```
bcdedit -set TESTSIGNING ON
```
**Note:** If BitLock is turned on, after restarting, it will ask for the key to enter! (So, it is best to back up the BitLock key to a USB flash drive first)

After RWEverything is installed and started, click the EC button on the toolbar to start displaying and monitoring the current 256 register values of EC, and the values of these registers can be modified manually (note that modifications are risky and may cause hardware damage!).
Note that the default is to refresh every 1.3 seconds (click the refresh button on the upper right to change the monitoring interval).

Now you can do some stress tests (such as cpu-z) to gradually increase the notebook fan speed, and you can observe which registers have changed. For example, the fan speed value is in registers 0x84 and 0x85 (represented by 2 bytes).
Unfortunately, although this value can be changed, it is quickly automatically updated by the EC firmware program. It is also possible that the BIOS/UEFI firmware program takes over the automatic update.

So is there any way to manually control the fan speed? Although, RWEverything gave the changed registers, and I took the risk to change them, but there was no result.
Fortunately there was no hardware damage.

After reading a short article written by a laptop fan program developer (How to analyze the laptop's DSDT table) [Analyze your notebook's DSDT](https://github.com/hirschmann/nbfc/wiki/Analyze-your-notebook%27s-DSDT),
I roughly understand the principle of how the operating system communicates with the BIOS/UEFI firmware program. This article is very enlightening. The analysis method is similar.

BIOS/UEFI firmware will provide the operating system with a standard ACPI advanced power management interface. DSDT (Difference System Description Table) is part of it and describes a lot of hardware device information.
Among them, EC is included. However, this DSDT table is assembly code and needs to be disassembled into ASL readable code.
Fortunately, the tool software RWEverything can not only read the ACPI > DSDT table, but also automatically disassemble the ASL readable code:
```
OperationRegion(ECOR, EmbeddedControl, Zero, 0x0100)
                        Field(ECOR, ByteAcc, NoLock, Preserve)
                        {
                                HDBM, 1,
                                , 1,
                                , 1,
......

HFSP, 8

......
```
The above code describes the description table of the ECOR (EC control) operable register, indicating the offset bit value of the HFSP 8-bit register (only the key part is intercepted here),
After hexadecimal calculation using the programmer's calculator, the offset value of the HFSP register is 0x2F.
Why are you so interested in the HFSP register? Because this register is the key EC register of Thinkpad E16 used to change the fan speed!
Looking at the ASL code found below, it clearly tells us that if we modify HFSP = 0x40, that is the maximum fan speed!
```
    If(LEqual(HFSP, 0x40))
                                {
                                        ADBG("MAX FAN speed")
                                        If(And(Arg0, 0x00040000))
                                        {
                                                Store(One, SCRM)
                                        }
                                        Else
                                        {
                                                Store(Zero, SCRM)
                                        }
                                }
                                Else
                                {
                                        ADBG("Allow to change FAN speed")
                                        If(And(Arg0, 0x00040000))
                                        {
                                                Store(One, SCRM)
                                                Store(0x07, HFSP)
                                        }
                                        Else
                                        {
                                                Store(Zero, SCRM)
                                                Store(0x80, HFSP)
                                        }
                                }
```
The above code shows that 3 values ​​can be set to the HFSP register, 0x40, 0x07, 0x80,
In fact, combined with the RWEverything tool software, modifying the HFSP register can adjust the manual 9-speed speed, as follows:
```
HFSP = 0x40 / 64 (5800 RPM)
HFSP = 0x07 / 7 (4400 RPM)
HFSP = 0x06 / 6 (4000 RPM)
HFSP = 0x05 / 5 (3700 RPM)
HFSP = 0x04 / 4 (3400 RPM)
HFSP = 0x03 / 3 (3100 RPM)
HFSP = 0x02 / 2 (2800 RPM)
HFSP = 0x01 / 1 (2500 RPM)
HFSP = 0X00 / 0 (turn off fan)
```
If the HFSP register is set to a value of 0x80, it means switching to the EC control program to automatically adjust the fan speed (0 - 1800 RPM-4100 RPM).
What’s a little interesting is that setting the HFSP register value to 0x40 (64) can adjust a maximum speed of 5800 RPM that the default EC automatic control fan does not have!
This speed is indeed very powerful, and the sound and wind noise reach the extreme...

# **Disclaimer: I am not responsible for any hardware damage caused by modifying the EC register with this program!**
