#!/usr/bin/env bash

#sudo ovs-ofctl del-flows s1 -O OpenFlow13 "table=1,in_port=1,dl_src=00:00:00:00:00:01,dl_dst=00:00:00:00:00:02"
sudo ovs-ofctl add-flow s1 -O OpenFlow13 \
"table=1,priority=10,in_port=1,dl_src=00:00:00:00:00:01,dl_dst=00:00:00:00:00:02,actions=drop"