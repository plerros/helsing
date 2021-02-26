# Helsing
The fastest vampire number generator.

This code utilizes posix threads. Windows isn't posix compatible, you'll need to install WSL.

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
Example:
```
./helsing 1000 9999
```
