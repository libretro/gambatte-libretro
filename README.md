# About

This is a GameBoy emulation core, forked from Gambatte by Sindre Aamås.
It has been modified by the EmulationOnline team for the PicoBoy emulator project.

It has been refactored and modified to run in a minimal environment, making it suitable
for loading as a shared library on platforms which may use incompatible standard libraries.

This project has two outputs:
1. main, which provides a minimal sdl2 based client for running on desktop platforms.
2. libgb.so, which implements the interface in corelib.h for embedded applications. 

## License
This core, consistent with the Gambatte project it is derived from, is available
under the GPL v2 license. The full license follows below.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 2 for more details.

You should have received a copy of the GNU General Public License
version 2 along with this program; if not, write to the
Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
--------------------------------------------------------------------------------
Gambatte
Copyright (C) 2007 by Sindre Aamås
aamas@stud.ntnu.no

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 2 for more details.

You should have received a copy of the GNU General Public License
version 2 along with this program; if not, write to the
Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

--------------------------------------------------------------------------------
Game Boy and Game Boy Color are registered trademarks of
Nintendo of America Inc.
Gambatte is not affiliated with or endorsed by any of the companies mentioned.
