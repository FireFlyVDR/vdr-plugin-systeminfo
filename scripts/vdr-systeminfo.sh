#!/bin/bash
# vdr-systeminfo.sh: external data collection script
# This file belongs to the VDR plugin systeminfo
#
# See the main source file 'systeminfo.c' for copyright information and
# how to reach the author.
#
# $Id$
#
# possible output formats:
# (blanks around tabs only for better reading)
# 1)   Name \t Value            displays Name and Value
# 2)   Name \t Value1 \t Value2 displays Name, Value1 and Value2
# 3)   Name \t total used       displays an additional progress bar (percentage) after the values
# 4)   s \t Name \t ...         defines a static value, this line is only requested during the first cycle
#
# special keywords (they are replaced by the plugin with the actual value):
#      CPU%    CPU usage in percent
#
# test with: systeminfo.sh [a|all]
# this iterates over all entries and prints the results
#

case "$1" in
	a|all)  # iterate over all entries for testing
		i=1
		while :
		do
			RESULT=$($0 $i)
			if [ -z "$RESULT" ]; then
				break
			fi
			printf "%2d %s\n" $i "${RESULT#s$'\t'}"
			let i=i+1
		done
		;;

	1)	# distribution and release (static)
		if test -f /etc/os-release; then
			RELEASE=$(grep "^PRETTY_NAME=" /etc/os-release|cut -d'"' -f 2)
		elif test -f /etc/SuSE-release; then
			RELEASE=$(head -n 1 /etc/SuSE-release)
		elif test -f /etc/redhat-release; then
			RELEASE=$(head -n 1 /etc/redhat-release)
		elif test -f /etc/debian_version; then
			RELEASE=$(head -n 1 /etc/debian_version)
		elif test -f /etc/gentoo-release; then
			RELEASE=$(head -n 1 /etc/gentoo-release)
		elif test -f /etc/lsb-release; then
			RELEASE=$(grep DISTRIB_DESCRIPTION /etc/lsb-release|cut -d"=" -f 2)
		elif test -x /usr/bin/crux; then
			RELEASE=$(crux|cut -d" " -f 3)
		elif test -f /etc/arch-release; then
			RELEASE="rolling-release"
		else
			RELEASE="unknown"
		fi
		echo -ne "s\tDistribution:\t${RELEASE}"
		exit
		;;

	2)	# kernel version (static)
		KERNEL=$(uname -rm)
		echo -ne "s\tLinux Kernel:\t${KERNEL}"
		;;

	3)	# hostname and IP (static)
		interface="eth0"
		hostname=$(hostname)
		dnsname=$(dnsdomainname)
		IP=$(ip a sh dev $interface | grep "inet " | cut -d' ' -f6)
		speed=$(cat /sys/class/net/${interface}/speed)
		echo -ne "s\tHostname:\t${hostname:-<unknown>}.${dnsname:-<unknown>}\tIPv4: ${IP:-N/A}  ${speed:-<unknown>} Mbit/s"
		exit
		;;

	4) # uptime
		UPTIME=$(last -1 reboot|head -n 1|tr -s " "|cut -d' ' -f5-)
		echo -ne "uptime:\t${UPTIME}"
		exit
		;;

	5)	# CPU type (static)
		CPUTYPE=$(grep 'model name' /proc/cpuinfo | uniq | cut -d':' -f 2)
		echo -ne "s\tCPU Type:\t${CPUTYPE}"
		;;

	6)	# current CPU frequency
		VAR=$(cat /sys/devices/system/cpu/cpu?/cpufreq/scaling_cur_freq)
		echo -ne "CPU frequency:\t"$(sed 's/.\{3\}$/ MHz/' <<<"$VAR")
		exit
		;;

	7)	# CPU usage
		echo -e "CPU time:\tCPU%"
		exit
		;;

	7)	# X resolution
		RES=$(DISPLAY=:0 xrandr|grep '*')
		echo -ne "s\tX-Resolution:\t$(echo ${RES%\*+}) Hz"
		exit
		;;

	8)	# GPU frequency
		DRM_DEVICE="$(find /sys/devices -type d -name drm -print 2>/dev/null)"
		GPU_CUR=$(cat ${DRM_DEVICE}/card?/gt_cur*)
		GPU_MAX=$(cat ${DRM_DEVICE}/card?/gt_max*)
		echo -ne "GPU frequency:\tcur: ${GPU_CUR} MHz\tmax: ${GPU_MAX} MHz"
		exit
		;;

	9)	# fan speeds
		SENSORS=$(/usr/bin/sensors)
		CPU=$(echo "$SENSORS"|grep -i 'CPU FAN'|tr -s ' '|cut -d' ' -f 3)
		CASE1=$(echo "$SENSORS"|grep -i 'Front Fan'|tr -s ' '|cut -d' ' -f 3)
		echo -ne "Lüfter:\tCPU: "$CPU" rpm\tFront: "$CASE1" rpm"
		exit
		;;

	10)	# temperature of CPU and mainboard
		SENSORS=$(/usr/bin/sensors)
		CPU=$(echo "$SENSORS"|grep -i 'Package id 0'|tr -s ' '|cut -d' ' -f 4)
		MB=$(echo "$SENSORS"|grep -i 'SYSTIN'|tr -s ' '|cut -d' ' -f 2)
		echo -ne "Temperaturen:\tCPU: "$CPU"\tMB: "$MB
		exit
		;;

	11)	# temperature of a SSD and a HDD
		# sensors requires module "drivetemp" to be loaded into kernel to show HDD temp
		DISK1=$(sudo /usr/sbin/smartctl -a /dev/nvme0n1|grep "^Temperature:"|tr -s " "| cut -f 2 -d" ")
		DISK2=$(/usr/bin/sensors -Au|grep -A 2 drivetemp-scsi-2-0|tail -n 1|cut -d: -f2)
		echo -ne "\tnvme01: ${DISK1} °C\t/dev/sdb:${DISK2} °C"
		exit
		;;

	12)	# temperature of two HDD
		SENSORS=$(/usr/bin/sensors -Au)
		DISK1=$(echo "$SENSORS"|grep -A 2 drivetemp-scsi-4-0|tail -n 1|cut -d: -f2)
		DISK2=$(echo "$SENSORS"|grep -A 2 drivetemp-scsi-5-0|tail -n 1|cut -d: -f2)
		echo -ne "\tSCSI-4:${DISK1} °C\tSCSI-5:${DISK2} °C"
		;;

	13)	# header (static)
		echo -ne "s\t\ttotal / free"
		exit
		;;

	14)	# video disk usage
		VAR=$(df -k --output=size,avail /srv/vdr/video0 | tail -n 1)
		echo -ne "Video Disk:\t"$VAR
		exit
        	;;

	15)	# memory usage
		VAR=$( grep -E 'MemTotal|MemFree' /proc/meminfo | cut -d: -f2)
		echo -ne "Memory:\t"$VAR
		exit
        	;;

	16)	# swap usage
		VAR=$(grep -E 'SwapTotal|SwapFree' /proc/meminfo | cut -d: -f2)
		echo -ne "Swap:\t"$VAR
		exit
        	;;
esac
exit
