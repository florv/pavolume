# Introduction

`pavolume` is a small command-line tool allowing you to change the volume of
the default audio sink of the PulseAudio server.


# Usage

    ./pavolume [mute|unmute|toggle] [+|-|=]value[%|dB]

Set the volume to 25%: `./pavolume 25%`

Set the volume to -20dB: `./pavolume =-20dB`

Set the volume to 30%: `./pavolume 0.3`

Increase the volume by 10%: `./pavolume +10%`

Decrease the volume by 5dB: `./pavolume -5dB`

Unmute and set the volume to 50%: `./pavolume unmute 50%`


# Compiling

You'll need the libpulse library with its header file. On a Debian (or
Debian-based system) `apt install libpulse-dev` will install the required
files.

The program can then be compiled with:
`gcc -O2 $(pkg-config --cflags --libs libpulse)" -O pavolume pavolume.c`
