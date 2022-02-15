# CREATE LAB Binder Jet Printer
This repository contains code for Binder Jetting Printer's control system, and belongs to the CREATE Lab

## Dependencies
- Galil API: gclib
https://www.galil.com/downloads/api
  - version 1.31.7

- IDS API: ueye
https://www.ids-imaging.us/download-details/AB00518.html#anc-software-272
  - IDS Software Suite 4.95.1 for Windows 32/64-bit
  - for IDS UI-3370CP-M-GL camera

- OpenCV:
  - version 4.5.5 for MSVC

- MSCV2019 compiler
- QT5

## Installation Instructions
- clone this directory
- install Galil and IDS libraries
- install OpenCV
- add Galil x64 bin folder to system PATH
  - C:\Program Files (x86)\Galil\gclib\dll\x64
- add QT5 bin to system PATH
  - C:\Qt\5.15.2\msvc2019_64\bin
