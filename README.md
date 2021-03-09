# Helsing
A POSIX compatible vampire number generator, with heapsort built in.

This code utilizes posix threads. Windows isn't posix compatible, you'll need to install WSL / WSL2.

In the source .c file there is a COMPILATION OPTIONS section.
There you can easily set the number of threads, tune the algorithm and adjust verbosity.
Be sure to read the documentation!

Compile with:
```
gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c
```
If the compilation fails, try:
```
gcc -O3 -Wall -Wextra -pthread -o helsing helsing.c -lm
```
Run with: 
```
./helsing min max
```
Examples:
```
./helsing 1260 1260
./helsing 1000000000000000 9999999999999999
```
