@echo off
rem The study run of the FSR 4.1.1 timing kit (see STUDY.txt): each change of the exact DLL timed on its own, for RDNA2 and RDNA3 graphics.
cd /d "%~dp0"
fsr4time.exe --study %*
