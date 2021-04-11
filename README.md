# Helsing
A POSIX vampire number generator, with heapsort built in.
The focus of this project is performance, code readability and portability.

In *helsing/configuration.h* you can easily set the number of threads, select the algorithm implementation and tune it, adjust verbosity and enable resume from checkpoint.
Be sure to read the documentation!

Windows isn't posix compatible. You'll need to set up a virtual machine or (if you are on Windows10) install WSL and a linux distribution from the Windows Store.

## Dependencies
 - a C compiler (clang or gcc should do just fine)
 - GNU make
 - findutils
 - OpenSSL
   Platform | Package name
   -------- | ------------
   Homebrew | openssl
   Debian | libssl-dev
   Fedora/RHEL | openssl-devel
   FreeBSD | openssl-devel


## How to compile for GNU/Linux & Homebrew
```
cd helsing
make
```
## How to compile for FreeBSD
```
cd helsing
gmake
```
## Run
```
./helsing min max
```
Examples:
```
./helsing 1260 1260
./helsing 1 1172560176
./helsing 18446744073709551615 18446744073709551615
```
## Recover from checkpoint (if enabled in configuration)
```
./helsing
```
