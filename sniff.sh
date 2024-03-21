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

USAGE="Usage: ./sniff.sh tcp|udp HOST [port]"

# no first argument
if [ "$1" = "" ]; then
    echo "$USAGE"
    exit
fi

# first argument is not tcp|udp
if [ "$1" != "tcp" -a "$1" != "udp" ]; then
    echo "$USAGE"
    exit
fi

# no second argument
if [ "$2" = "" ]; then
    echo "$USAGE"
    exit
fi

# append to file?
if [ -f "$FILE" ]; then
    echo -n "append to '$FILE'? (y/n) "
    read APPEND
    if [ "$APPEND" != "y" ]; then
        exit
    fi
fi

# tcp with no port filter?
if [ "$1" = "tcp" -a "$3" = "" ]; then
    echo -n "capturing tcp with no port filter generates very large files, \
continue? (y/n) "
    read TCP_NOFILT_CONT
    if [ "$TCP_NOFILT_CONT" != "y" ]; then
        exit
    fi
fi

# either filter by port directly by tcpdump or dont
if [ "$3" != "" ]; then
    PORTFI="port $3"
fi

# todo: ask whether capture on loopback or any
echo -n "Capture on loopback interface (l) or any interface (a)? "
read CAPT_INTER
if [ "$CAPT_INTER" = "l" ]; then
    INTERFI=lo
fi
if [ "$CAPT_INTER" = "a" ]; then
    INTERFI=any
fi
if [ "$INTERFI" = "" ]; then
    exit
fi


TCPD="sudo tcpdump -X -l -i $INTERFI ip and ($1 $PORTFI or icmp)"
TEELOG="tee -a $FILE"
GREPFILT="grep -F --color=auto $2 -A $NUM"

# log date and command to console
echo -n "todays date: "
date
echo "$TCPD | $TEELOG | $GREPFILT"

# log date and command to file
echo "" >> $FILE
date >> $FILE
echo "$TCPD | $TEELOG | $GREPFILT" >> $FILE

# run the thing
$TCPD | $TEELOG | $GREPFILT
