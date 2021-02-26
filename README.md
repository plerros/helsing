# Helsing
The fastest vampire number generator.

This code utilizes posix threads. Windows isn't posix compatible, you'll need to install WSL.

Compile with: gcc -O3 -Wall -Wextra -pthread -lm -o helsing helsing.c

If the compilation fails, try: gcc -O3 -Wall -Wextra -pthread -o helsing helsing.c -lm

Run with: ./helsing min max (example: ./helsing 1000 9999).
