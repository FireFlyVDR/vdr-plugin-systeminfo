#!/bin/bash
# systeminfo.sh: external data collection script
# This file belongs to the VDR plugin systeminfo
#
# See the main source file 'systeminfo.c' for copyright information and
# how to reach the author.
#
# $Id$
#
# possible output formats:
# (blanks around tabs only for better reading)
# 1)   Name \t Value         	displays Name and Value
# 2)   Name \t Value1 \t Value2 displays Name, Value1 and Value2
# 3)   Name \t total used       displays an additional progress bar (percentage) after the values
# 4)   s \t Name \t ...         defines a static value, will not be called again
#
# test with: for i in $(seq 1 16); do /usr/local/bin/systeminfo.sh $i;echo;done
#

case "$1" in
	1)	# kernel version (static)
		KERNEL=$(/bin/uname -rm)
		echo -ne "s\tLinux Kernel:\t"$KERNEL
        	;;

	2)	# distribution release (static)
		if test -f /etc/SuSE-release; then
			DISTRI="openSuSE"
			RELEASE=$(head -n 1 /etc/SuSE-release)
		elif test -f /etc/redhat-release; then
			DISTRI="RedHat"
			RELEASE=$(head -n 1 /etc/redhat-release)
		elif test -f /etc/debian_version; then
			DISTRI="Debian"
			RELEASE=$(head -n 1 /etc/debian_version)
		elif test -f /etc/gentoo-release; then
			DISTRI="Gentoo"
			RELEASE=$(head -n 1 /etc/gentoo-release)
		elif test -f /etc/lsb-release; then
			DISTRI=$(grep DISTRIB_ID /etc/lsb-release|cut -d"=" -f 2)
			RELEASE=$(grep DISTRIB_DESCRIPTION /etc/lsb-release|cut -d"=" -f 2)

		else
			DISTRI="unknown"
			RELEASE="unknow"
		fi
		echo -ne "s\tDistribution:\t"$RELEASE
		exit
        	;;

	3)	# CPU type (static)
		CPUTYPE=$(/usr/bin/grep 'model name' /proc/cpuinfo | /usr/bin/cut -d':' -f 2  | /usr/bin/cut -d' ' -f2- | /usr/bin/uniq)
		echo -ne "s\tCPU Type:\t"$CPUTYPE
        	;;

	4)	# current CPU speed
		VAR=$(/usr/bin/grep 'cpu MHz' /proc/cpuinfo|/usr/bin/sed 's/.*: *\([0-9]*\)\.[0-9]*/\1 MHz/')
		echo -ne "CPU speed:\t"$VAR
		exit
        	;;

	5)	# hostname and IP (static)
		hostname=$(/bin/hostname)
		dnsname=$(/bin/dnsdomainname)
		IP=$(/sbin/ifconfig br0|/usr/bin/grep inet|/usr/bin/cut -d: -f2|/usr/bin/cut -d' ' -f1)
		echo -ne "s\tHostname:\t"${hostname:-<unbekannt>}"."${dnsname:-<unbekannt>}"\tIP: "${IP:-N/A}
		exit
        	;;

	6)	# fan speeds
		CPU=$(/usr/bin/sensors|/usr/bin/grep -i 'CPU FAN'|/usr/bin/tr -s ' '|/usr/bin/cut -d' ' -f 3)
		CASE=$(/usr/bin/sensors|/usr/bin/grep -i 'SYS Fan'|/usr/bin/tr -s ' '|/usr/bin/cut -d' ' -f 3)
		echo -ne "Lüfter:\tCPU: "$CPU" rpm\tGehäuse: "$CASE" rpm"
		exit
        	;;

	7)	# temperature of CPU and mainboard
		CPU=$(/usr/bin/sensors|/usr/bin/grep -i 'CPU TEMP'|/usr/bin/tr -s ' '|/usr/bin/cut -d' ' -f 3)
		MB=$(/usr/bin/sensors|/usr/bin/grep -i 'Sys temp'|/usr/bin/tr -s ' '|/usr/bin/cut -d' ' -f 3)
		echo -ne "Temperaturen:\tCPU: "$CPU"\tMB: "$MB
		exit
        	;;

	8)	# temperature of hard disks
		DISK1=$(/usr/sbin/hddtemp /dev/sda|/usr/bin/cut -d: -f1,3)
		DISK2=$(/usr/sbin/hddtemp /dev/sdb|/usr/bin/cut -d: -f1,3)
		echo -ne "\t"$DISK1"\t"$DISK2
		exit
        	;;

	9)	# CPU usage
		echo -ne "CPU time:\t"
                ps -eo%C | awk '/[.]/ { a=a+$1 } ; END { print a " %" }'
		exit
        	;;

	10)	# header (static)
		echo -ne "s\t\tgesamt / frei"
		exit
		;;

	11)	# video disk usage
		echo -ne "Video Disk:\t"
		/bin/df -Pk /mnt/vdr|/usr/bin/tail -n 1|/usr/bin/tr -s ' '|/usr/bin/cut -d' ' -f 2,4
		exit
        	;;

	12)	# memory usage
		VAR=$(/usr/bin/grep -E 'MemTotal|MemFree' /proc/meminfo|/usr/bin/cut -d: -f2|/usr/bin/tr -d ' ')
		echo -ne "Memory:\t"$VAR
		exit
        	;;

	13)	# swap usage
		VAR=$(/usr/bin/grep -E 'SwapTotal|SwapFree' /proc/meminfo|/usr/bin/cut -d: -f2|/usr/bin/tr -d ' ')
		echo -ne "Swap:\t"$VAR
		exit
        	;;
	test)
		echo ""
		echo "Usage: systeminfo.sh {1|2|3|4|...}"
		echo ""
		exit 1
		;;
esac
exit
