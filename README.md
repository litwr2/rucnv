# rucnv
Encoding converter for texts in Russian language

##General Information
Up to 14 different encodings are used to represent texts in Russian language (koi-7, koi-8r, cp866, cp1251, cp10007, utf-8, ...)! This program can perform conversion between them. It also can produce Unicode names for the symbol codes.

This program may also be used for transliteration (reversible substitution Cyrillic letters by Latin ones). Transliteration is the only way to make deal with Russian texts on the system without Cyrillic fonts. RUCNV knows two types of transliteration: GOST 7.79-2000 and KOI-7 based.

RUCNV may be used for conversion any Russian or Unicode texts into the form which will be accepted by any 8-bit text editor and for followed backward conversion. 

This program is distributed under the GNU General Public License.


##Basic Installation
You need C++ compiler and C++ Standard Library.

The simplest way to make this program is:

1. Edit makefile to set correct MANDIR and BINDIR for your system.
2. Type `make' to compile the program.
3. Type `make install' to install the program and documentation.


##Microsoft DOS/Windows Installation
The simplest way to make this with DELORIE DJGPP V2 system for C/C++ 
(htpp://www.delorie.com) program is:

1. Type `gxx -o rucnv.exe rucnv.cpp -lstdcxx` to get the executable file.
2. Type `strip rucnv.exe` to remove symbols from file.
3. Type `exe2coff rucnv.exe` and then 
     `copy /b [dir for djgpp]\bin\cwsdstub.exe + rucnv rucnv.exe` to get the 
     executable file independent from DJGPP system.

Steps 2 and 3 are optional.


##A NOTE ON USAGE
You may use rucnv with the same filenames for input and output.


##BUGS, SUGGESTIONS, ETC.
Please write me if you have trouble running rucnv, or if you 
have suggestions or patches.

	Vladimir Lidovski
	litwr@yandex.ru
	http://litwr2.atspace.eu


##COPYRIGHT/LICENSE
All source code is Copyright (C) 2002 Lidovski Vladimir

This program is distributed under the GNU General Public License, Version
2, or, at your discretion, any later version. The GNU General Public License
is available via the Web at <http://www.gnu.org/copyleft/gpl.html>. The GPL
is designed to allow you to alter and redistribute the package, as long as
you do not remove that freedom from others.
