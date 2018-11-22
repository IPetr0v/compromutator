#!/bin/sh

echo s1
sudo ovs-ofctl dump-flows s1 -O OpenFlow13
echo special
echo s2
sudo ovs-ofctl dump-flows s2 -O OpenFlow13
#  | grep 'cookie=0x0'
#sudo ovs-ofctl dump-flows s1 \
#"cookie=0x0/-1, table=0, in_port=2,dl_src=6a:86:f9:b4:64:c5,dl_dst=66:b5:14:82:2c:33" \
#-O OpenFlow13