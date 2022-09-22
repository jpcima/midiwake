# midiwake
A utility to block the screen saver during MIDI activity

## Description

This program shows up in the notification area, and it listens for output
produced by hardware MIDI devices, such as a synthesizer keyboard.

While the device is being played with, the program prevents the desktop
from entering idle mode, which might lock the session or activate the screen
saver. For instance, one can practice a piece of music on screen without
risks of interruption.

As of now, this program is compatible only with Linux.
It uses some desktop-specific D-bus interfaces, which hopefully work with most
desktop environments, although they are not equally tested.
Feedback regarding particular desktops is appreciated.

## Packages

Official RPM packages are available on [COPR].

[COPR]: https://copr.fedorainfracloud.org/coprs/jpcima/midiwake/

## Release notes

**1.0.0**

- Initial release of the program
