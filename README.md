# Helsing
A POSIX vampire number generator, with heapsort built in.
The focus of this project is performance, code readability and portability.

In the source helsing.c file there is a COMPILATION OPTIONS section.
There you can easily set the number of threads, tune the algorithm and adjust verbosity.
Be sure to read the documentation!

Windows isn't posix compatible, you'll need to install WSL or WSL2.

Compile with:
```
gcc -O2 -Wall -Wextra -pthread -o helsing helsing.c
```
Run with: 
```
./helsing min max
```
Examples:
```
./helsing 1260 1260
./helsing 1 1172560176
./helsing 18446744073709551615 18446744073709551615
```
