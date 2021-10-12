# Helsing
A mostly POSIX-compliant utility that scans a given interval for vampire numbers.

In *helsing/configuration.h* you can select the algorithm implementation and tune it, adjust verbosity, change the numeral base system, set a minimum fang pairs filter, and enable resume from checkpoint.
Be sure to read the documentation.

Windows isn't posix compatible. You'll need to install WSL and a linux distribution from the Windows Store.

## Dependencies
 - a C compiler (clang or gcc should do just fine)
 - GNU make
 - findutils
 - OpenSSL (optional)

   Platform | Package name
   -------- | ------------
   Debian | libssl-dev
   Fedora/RHEL | openssl-devel
   FreeBSD | openssl-devel
   HaikuOS | (pre-installed)
   Homebrew | openssl
   Openindiana | library/security/openssl-11

## Download
```
git clone https://github.com/plerros/helsing.git
cd helsing
```
## Compile
#### GNU/Linux, HaikuOS, Homebrew, WSL
```
cd helsing
make
```
#### FreeBSD, OpenIndiana
```
cd helsing
gmake
```
## Run
```
./helsing -l min -u max
```
Examples:
```
./helsing -l 1260 -u 1260
./helsing -l 0 -u 1172560176
./helsing -l 18446744073709551615 -u 18446744073709551615
```
#### Run for all n-digit numbers
```
./helsing -n number_of_digits
```
examples:
```
./helsing -n 2
./helsing -n 4
./helsing -n 18
```
#### Set the number of threads
```
./helsing -t threads
```
#### Display progress
```
./helsing --progress
```
#### Recover from checkpoint (if enabled in configuration)
```
./helsing
```
