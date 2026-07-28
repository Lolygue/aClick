# About

aClick is a simple autoclicker focused on being compact and efficient.

It offers parameters or interval and button, allowing you to click with three mouse buttons and two scroll directions as fast as 0.1 milliseconds or as slow as after the heat death of the universe.

It clicks very fast, even to the point of instability in most programs that you can click in. At 0.0ms, it even crashes itself (which is why the minimum interval is 0.1ms).

This is mostly just a personal project, but if you find use in this utility, that's cool!

# Compilation

## Prerequisetes

Libraries (most will be dnf package names):

* standard C library
* `libX11-devel`
* `libXtst-devel`
* `raylib-devel`

Compilers:

* `gcc`

## Commands

If you have GNU Make installed already, just run `make` in the project root.

To run with debug features, use `make debug`.