#!/bin/bash
# sniffs on localhost and filters UDP packets on port 4567

COMMAND="sudo tcpdump -X -l ip and (udp or icmp)"
echo "$COMMAND"
$COMMAND
# -X        ... header and data in hex + ascci
# -l        ... line buffered
# -i lo     ... interface `lo`
# port 4567 ... filter by port 4567 (src or dst) (this only filters AUTH
#               messages from client tho)
