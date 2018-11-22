#!/bin/sh

#sudo ovs-ofctl add-flow s1 -O OpenFlow13 "in_port=1, actions=OUTPUT:2"
curl -X GET -d '{
    "dpid": 1,
    "cookie": 0,
    "cookie_mask": 0,
    "table_id": 0,
    "priority": 1,
    "flags": 0
 }' http://localhost:8080/stats/flow/1