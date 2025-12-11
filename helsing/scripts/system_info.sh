#!/bin/bash

: '
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
'

bold=$(tput bold)
normal=$(tput sgr0)

echo -e "\n-------------------------------CPU boost check -------------------------------"
if ! command -v cpupower >/dev/null 2>&1; then
	echo "cpupower could not be found"
	exit 1
fi

cpupower frequency-info
echo

boost="$(cpupower frequency-info | grep -o 'Active: .*')"
if [ "$boost" != "Active: no" ]; then
	echo "${bold}It's advised to turn off CPU boost.${normal}"
	echo "That can usually be done through:"
	echo "    /sys/devices/system/cpu/intel_pstate/no_turbo"
	echo "    /sys/devices/system/cpu/cpufreq/boost"
	echo "    /sys/devices/system/cpu/amd_pstate/cpb_boost"
	echo
	read -rsn1 -p "Press any key to continue"
	echo
fi

echo -e "\n----------------------------------ASLR check----------------------------------"
aslr="$(cat /proc/sys/kernel/randomize_va_space)"
echo "ASLR = $aslr"
echo

if [ "$aslr" != "0" ]; then
	echo "${bold}On some older systems it's important to turn off ASLR.${normal}"
	echo "That can usually be done through:"
	echo "    /proc/sys/kernel/randomize_va_space"
	echo
	read -rsn1 -p "Press any key to continue"
	echo
fi

