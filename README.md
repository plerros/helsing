# Helsing
Helsing is a command-line program that scans intervals for vampire numbers.

![helsing_gif](https://user-images.githubusercontent.com/48536508/184532048-42dbfd27-78bd-439f-8700-00e460a3a93c.gif)

The default algorithm has a time complexity of $O(u)$ and a space complexity of
$O(\sqrt{u})$, where $u$ is the upper bound argument `-u`.

In *helsing/configuration.h* you can toggle the algorithms and tune them,
adjust verbosity, change the numeral base system, and set a fang pairs
filter.
Be sure to read the documentation.

## Windows Preparation

Helsing uses posix threads, and since Windows isn't posix compatible, you'll
need to [install WSL](https://docs.microsoft.com/en-us/windows/wsl/install).

Note that the guide above will install WSL with Ubuntu, so you'll have to follow the Ubuntu instructions.


## MacOS Preparation

On MacOS you'll have to [install homebrew](https://brew.sh/).

## Building
1. Install dependencies
	<table>
		<tr>
			<td><b>Platform</b></td>
			<td><b>Install Command</b></td>
			<td><b>Packages</b></td>
			<td><b>Optional</b></td>
		</tr>
		<tr>
			<td>MacOS</td>
			<td><code>brew install</code></td>
			<td rowspan="5"><code>git gcc cmake</code></td>
			<td><code><strike>openssl</strike></code> (broken)</td>
		</tr>
		<tr>
			<td>Debian/Ubuntu</td>
			<td><code>sudo apt install</code></td>
			<td><code>libssl-dev</code></td>
		</tr>
		<tr>
			<td>Fedora/RHEL</td>
			<td><code>sudo dnf install</code></td>
			<td><code>openssl-devel</code></td>
		</tr>
		<tr>
			<td>FreeBSD:</td>
			<td><code>pkg install</code></td>
			<td><code>openssl-devel</code></td>
		</tr>
		<tr>
			<td>HaikuOS</td>
			<td><code>pkgman install</code></td>
			<td>(pre-installed)</td>
		</tr>
		<tr>
			<td>Openindiana</td>
			<td><code>pkg install</code></td>
			<td><code>git gcc-14 cmake </code></td>
			<td><code>library/security/openssl-3</code></td>
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
			<td rowspan="6"><code>mkdir build; cd build; cmake ..; make</code></td>
		</tr>
		<tr>
			<td>Debian/Ubuntu</td>
		</tr>
		<tr>
			<td>Fedora/RHEL</td>
		</tr>
		<tr>
			<td>FreeBSD:</td>
		</tr>
		<tr>
			<td>HaikuOS</td>
		</tr>
		<tr>
			<td>Openindiana</td>
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
#### Checkpoints:

```
./helsing -n number_of_digits -c checkpoint
```
```
./helsing -l min -u max -c checkpoint
```
Example:

```
$ ./helsing -n 4 -c my.checkpoint
Checking interval: [1000, 9999]
Found: 7 vampire number(s).

$ ls
build           configuration_adv.h  helsing   my.checkpoint  src
CMakeLists.txt  configuration.h      Makefile  scripts        test
```
To resume from it:

```
./helsing -c checkpoint
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
    FANG_PAIR_OUTPUTS=false
    VAMPIRE_NUMBER_OUTPUTS=true
        VAMPIRE_INDEX=false
        VAMPIRE_PRINT=false
        VAMPIRE_INTEGRAL=false
        VAMPIRE_HASH=false
    MIN_FANG_PAIRS=1
    MAX_FANG_PAIRS=1
    MEASURE_RUNTIME=false
    ALG_NORMAL=false
    ALG_CACHE=true
        PARTITION_METHOD=0
        MULTIPLICAND_PARTITIONS=2
        PRODUCT_PARTITIONS=3
    BASE=10
    MAX_TASK_SIZE=99999999999
    USE_CHECKPOINT=true
    LINK_SIZE=100
    LLMSENTENCE_LIMIT=10000
    TASKBOARD_LIMIT=1000000
    SAFETY_CHECKS=false
```
