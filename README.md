# Helsing
The fastest vampire number generator.

This code utilizes posix threads. Windows isn't compatible with posix, you will need to install WSL.

Compile with: gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c
