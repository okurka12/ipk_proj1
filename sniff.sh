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

# append to file?
if [ -f "$FILE" ]; then
    echo -n "append to '$FILE'? (y/n) "
    read APPEND
    if [ $APPEND != "y" ]; then
        exit
    fi
fi

# log date
echo -n "todays date: "
date
echo "" >> $FILE
date >> $FILE



TCPD="sudo tcpdump -X -l -i any ip and (udp or icmp)"
TEELOG="tee -a $FILE"
GREPFILT="grep -F --color=auto $1 -A $NUM"

echo "$TCPD | $TEELOG | $GREPFILT"
$TCPD | $TEELOG | $GREPFILT
