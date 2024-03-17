# ec_tpfan

This is forked from https://github.com/Soberia/EmbeddedController

All interfaces are consistent with no change , just parameterized special methods for read / write EC's registers.

# **📋 Compile / Build in Visual Studio 2019**
You need to compile your app with `c++17` flag (this is completed already in app property settings)
This app uses `WinRing0` driver to access the hardware, make sure you place `WinRing0x64.sys` or `WinRing0.sys` beside your binary files.
Your program has to run with administrator privileges to work properly.

# How to control Thinkpad E16 Fan RPM
