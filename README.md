Push
====

Gaming tools. Monitor GPU/CPU/RAM usage in-game.

Features
--------

OSD: 
- gpu/cpu load 
- gpu/cpu temp 
- vram/ram usage 
- max cpu usage 
- max thread usage 
- disk read/write rate
- disk response time
- backbuffer count 
- time

RAMDisk: 
- automatically cache files to ram disk

D3D Tweaks: 
- force vsync 
- frame limiter 

Other: 
- thread optimization
- auto-force max gpu clocks
- in-game settings menu

Credits
-------

- OvRender (https://github.com/Volkanite/OvRender)
- DetourXS (https://github.com/DominicTobias/detourxs)

Notes
-----

- The driver isn't signed. To use you have to enable test mode.
- I dumped x86 driver builds but will implement again if requested.
- Allot of stuff not found in the gui can be enabled via the ini file. I wanted to keep it as simple as possible so only important stuff are kept in the gui. If you need information on how to use something just ask.
- Press [Insert] to bring up the in-game settings menu. Be warned it is poorly implemented. When this project has matured a bit more, it can be used to change gpu clocks and fan speeds while in-game :)
