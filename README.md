# Helsing
Helsing is a command-line program that scans intervals for vampire numbers.

![helsing_gif](https://user-images.githubusercontent.com/48536508/184532048-42dbfd27-78bd-439f-8700-00e460a3a93c.gif)

The default algorithm has a time complexity of $O(n)$ and a space complexity of
$O(\sqrt{n})$.

In *helsing/configuration.h* you can toggle the algorithms and tune them,
adjust verbosity, change the numeral base system, set a minimum fang pairs
filter, and enable resume from checkpoint.
Be sure to read the documentation.

## Windows Preparation

Helsing uses posix threads, and since Windows isn't posix compatible, you'll
need to [install WSL](https://docs.microsoft.com/en-us/windows/wsl/install).

Note that the guide above will install WSL with Ubuntu, so you'll have to follow the Ubuntu instructions.


## MacOS Preparation

On MacOS you'll have to [install homebrew](https://brew.sh/).

## Installation
1. Install dependencies
	<table>
		<tr>
			<td><b>Platform</b></td>
			<td><b>Install</b></td>
			<td><b>Install optional</b></td>
		</tr>
		<tr>
			<td>MacOS</td>
			<td><code>brew install git gcc gmake findutils</code></td>
			<td><code><strike>brew install openssl</strike></code> (broken)</td>
		</tr>
		<tr>
			<td>Debian/Ubuntu</td>
			<td><code>sudo apt install git gcc make findutils</code></td>
			<td><code>sudo apt install libssl-dev</code></td>
		</tr>
		<tr>
			<td>Fedora/RHEL</td>
			<td><code>sudo dnf install git gcc make findutils</code></td>
			<td><code>sudo dnf install openssl-devel</code></td>
		</tr>
		<tr>
			<td>FreeBSD:</td>
			<td><code>pkg install git gcc gmake findutils</code></td>
			<td><code>pkg install openssl-devel</code></td>
		</tr>
		<tr>
			<td>HaikuOS</td>
			<td><code>pkgman install git gcc make findutils</code></td>
			<td>(pre-installed)</td>
		</tr>
		<tr>
			<td>Openindiana</td>
			<td><code>pkg install git gcc gmake findutils</code></td>
			<td><code>pkg install library/security/openssl-11</code></td>
		</tr>
	</table>
	You can also use clang instead of gcc

2. Download

	```
	git clone https://github.com/plerros/helsing.git
	cd helsing/helsing
	```
3. Compile
	<table>
		<tr>
			<td><b>Platform</b></td>
			<td><b>Compile</b></td>
		</tr>
		<tr>
			<td>MacOS</td>
			<td><code>gmake</code></td>
		</tr>
		<tr>
			<td>Debian/Ubuntu</td>
			<td><code>make</code></td>
		</tr>
		<tr>
			<td>Fedora/RHEL</td>
			<td><code>make</code></td>
		</tr>
		<tr>
			<td>FreeBSD:</td>
			<td><code>gmake</code></td>
		</tr>
		<tr>
			<td>HaikuOS</td>
			<td><code>make</code></td>
		</tr>
		<tr>
			<td>Openindiana</td>
			<td><code>gmake</code></td>
		</tr>
	</table>

## Run
```
./helsing -l min -u max
```
Examples:

```
$ ./helsing -l 1260 -u 1260
Checking interval: [1260, 1260]
Found: 1 vampire number(s).

$ ./helsing -l 0 -u 1172560176
Adjusted min from 0 to 10
Checking interval: [10, 99]
Checking interval: [1000, 9999]
Checking interval: [100000, 999999]
Checking interval: [10000000, 99999999]
Checking interval: [1000000000, 1172560176]
Found: 10000 vampire number(s).

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

$ ./helsing -n 4
Checking interval: [1000, 9999]
Found: 7 vampire number(s).

$ ./helsing -n 12
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

$ ./helsing -n 14
Checking interval: [10000000000000, 99999999999999]
Found: 208423682 vampire number(s).

$ ./helsing -n 16
Checking interval: [1000000000000000, 9999999999999999]
Found: 11039126154 vampire number(s).
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

real	0m36.781s
user	0m36.678s
sys	0m0.025s

$ time ./helsing -n 12 -t 2
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m18.592s
user	0m36.581s
sys	0m0.037s

$ time ./helsing -n 12 -t 4
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m9.402s
user	0m37.361s
sys	0m0.041s

time ./helsing -n 12 -t 8
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m4.913s
user	0m38.510s
sys	0m0.044s

time ./helsing -n 12 -t 12
Checking interval: [100000000000, 999999999999]
Found: 4390670 vampire number(s).

real	0m3.344s
user	0m39.233s
sys	0m0.030s
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
#### Dry run
```
./helsing --dry-run
```
Example:

```
$ ./helsing -l 0 -u 18446744073709551615 --dry-run
Adjusted min from 0 to 10
Checking interval: [10, 99]
Checking interval: [1000, 9999]
Checking interval: [100000, 999999]
Checking interval: [10000000, 99999999]
Checking interval: [1000000000, 9999999999]
Checking interval: [100000000000, 999999999999]
Checking interval: [10000000000000, 99999999999999]
Checking interval: [1000000000000000, 9999999999999999]
Checking interval: [100000000000000000, 999999999999999999]
Checking interval: [10000000000000000000, 18446744073709551615]
Found: 0 vampire number(s).
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
    MAX_FANG_PAIRS=1
    MEASURE_RUNTIME=false
    ALG_NORMAL=false
    ALG_CACHE=true
    COMPARISON_BITS=64
    PARTITION_METHOD=0
    MULTIPLICAND_PARTITIONS=2
    PRODUCT_PARTITIONS=3
    BASE=10
    MAX_TASK_SIZE=99999999999
    USE_CHECKPOINT=false
    LINK_SIZE=100
    SAFETY_CHECKS=false
```
#### Recover from checkpoint (if enabled in configuration)
```
./helsing
```
