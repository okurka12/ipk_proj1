#!/bin/bash
#
# Usage: ./sniff.sh HOST
#
# sniffs on UDP + ICMP IPv4 packets, only prints those to/from HOST and only
# prints NUM * 16 bytes of them
#
# stores all the captured packets to FILE for later reference
#

NUM=7
FILE=tcpdump.log

# no arguments?
if [ "$1" = "" ]; then
    echo "Usage: ./sniff.sh HOST"
    exit
fi

# overwrite file?
if [ -f "$FILE" ]; then
    echo -n "overwrite '$FILE'? (y/n) "
    read OVERWRITE
    if [ $OVERWRITE != "y" ]; then
        exit
    fi
fi



TCPD="sudo tcpdump -X -l ip and (udp or icmp)"
TEELOG="tee $FILE"
GREPFILT="grep --color=auto $1 -A $NUM"

echo "$TCPD | $TEELOG | $GREPFILT"
$TCPD | $TEELOG | $GREPFILT


# -X        ... header and data in hex + ascci
# -l        ... line buffered
# -i lo     ... interface `lo` (default interface is eth0)
# ip and (udp or icmp) ... only ipv4 udp + icmp packets
