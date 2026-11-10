#!/bin/sh
# Runs a Windows program of the port's cross build (mingw-w64-x86_64.cmake)
# with wine, quietly, in a wine prefix of the port's own (WINEPREFIX, else
# ~/.cache/crusader-port-wine) rather than the user's. Without the drivers
# of the host's USB, Bluetooth and HID devices (wine's HID parser can crash
# on a Bluetooth gamepad the prefix's start finds) and
# without winedbg (a crash ends the program at once rather than waiting on
# the debugger).
: "${WINEPREFIX:=${XDG_CACHE_HOME:-$HOME/.cache}/crusader-port-wine}"
: "${WINEDEBUG:=-all}"
: "${WINEDLLOVERRIDES:=mscoree,mshtml=;winebus.sys,wineusb.sys,winebth.sys=d;winedbg.exe=d}"
export WINEPREFIX WINEDEBUG WINEDLLOVERRIDES
# wine makes the prefix but not the directory it is in (on a new machine,
# ~/.cache may not be there yet).
mkdir -p "${WINEPREFIX%/*}"
exec wine "$@"
