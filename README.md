# pc98-emu
A (somewhat) functional emulator targeting the PC-98 Platform

# Why?
Well, the motive is a bit silly. I feel like current emulators are either:
1) Somewhat bloated (as in, they don't *just* target the PC-98 platform)
2) Not open source
3) Not linux friendly (or if they are, they're barely maintained!!)

# So.. What needs to be done?
Currently, it's pretty much just the 8086 CPU that's been (almost!!) implemented. However the current plan is to have
- i386 support
- Properly working C-Bus implementation
- The ability to run the *original* PC-9801 BIOS
- Keyboard + Mouse driver (seemingly needed to boot the PC-98 bios)
- Video Driver
- More accurate emulation
- Code cleanup (x64 emulation currently exists in the codebase, but should not be needed for targeting the PC-98 platform)
