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
$ ./helsing -l 1260 -u 1260
Checking interval: [1260, 1260]
Found: 1 vampire number(s).
```
```
$ ./helsing -l 0 -u 1172560176
Adjusted min from 0 to 10
Checking interval: [10, 99]
Checking interval: [1000, 9999]
Checking interval: [100000, 999999]
Checking interval: [10000000, 99999999]
Checking interval: [1000000000, 1172560176]
Found: 10000 vampire number(s).
```
```
$ ./helsing -l 18446744073709551615 -u 18446744073709551615
Checking interval: [18446744073709551615, 18446744073709551615]
Found: 0 vampire number(s).
```
#### Run for all n-digit numbers
```
./helsing -n number_of_digits
```
Examples:

```
$ ./helsing -n 2
Checking interval: [10, 99]
Found: 0 vampire number(s).
```
```
$ ./helsing -n 4
Checking interval: [1000, 9999]
Found: 7 vampire number(s).
```
```
$ ./helsing -n 12
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).
```
#### Set the number of threads
```
./helsing -t threads
```
Examples:

```
$ time ./helsing -n 12 -t 1
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m39.360s
user	0m39.294s
sys 	0m0.010s
```
```
$ time ./helsing -n 12 -t 2
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m20.036s
user	0m39.746s
sys 	0m0.022s
```
```
$ time ./helsing -n 12 -t 4
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m10.514s
user	0m41.624s
sys 	0m0.022s
```
#### Display progress
```
./helsing --progress
```
Example:

```
$ ./helsing -n 12 --progress
Checking interval: [100000000000, 999999999999]
100000000000, 199999999999  1/9
200000000000, 299999999999  2/9
300000000000, 399999999999  3/9
400000000000, 499999999999  4/9
500000000000, 599999999999  5/9
600000000000, 699999999999  6/9
700000000000, 799999999999  7/9
800000000000, 899999999999  8/9
900000000000, 999998000001  9/9
Found: 4390670 vampire number(s).
```
#### Display build configuration
```
./helsing --buildconf
```
Example:

```
$ ./helsing --buildconf
  configuration:
    VERBOSE_LEVEL=2
    MIN_FANG_PAIRS=1
    MEASURE_RUNTIME=false
    CACHE=true
    COMPARISON_BITS=64
    DEDICATED_BITFIELDS=false
    USE_PDEP=false
    BASE=10
    MAX_TASK_SIZE=99999999999
    USE_CHECKPOINT=false
    LINK_SIZE=100
    SANITY_CHECK=false
```
#### Recover from checkpoint (if enabled in configuration)
```
./helsing
```
