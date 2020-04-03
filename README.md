# LuaKeyboardHelper

With this Windows program you can intercept and generate keyboard events with a Lua script.

## Usage

1. Edit the script.lua file for your application

  The demo script expects a consecutive 56 digits number from a barcode scanner for
  Australian Post 2D barcode, which is consumed by the script, and then extracts the
  tracking number, which is sent as new key events to the program which is in focus.

2. Start the LuaKeyboardHelper program

  You should see a minimal Windows program with a File and Help menu. As long as this
  program is running, the Lua script is running as well. The LuaKeyboardHelper.exe file
  and the script.lua file must be in the same directory.

## Detailled description

In the Lua script, you have to define the function onKeyPressed and onKeyReleased, which are
called when a key is pressed or released. From the script you can call the timeout
function, which causes the onTimeout function to be called after the specified number
of milliseconds. If you want to simulate a key press / release sequence, you can call
the sendKey function, with the ASCII code of the desired key.

## Build instructions

Install Visual Studi 2017 or later, open the LuaKeyboardHelper.sln project file, and 
build as usual. Tested for the 32 bit target.
