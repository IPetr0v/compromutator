#!/bin/sh

#sudo ovs-ofctl add-flow s1 -O OpenFlow13 "in_port=1, actions=OUTPUT:2"
curl -X GET -d '{
    "dpid": 2,
    "cookie": 0,
    "cookie_mask": 0,
    "table_id": 0,
    "priority": 0,
    "flags": 0,
    "match": {
        "in_port": 3,
        "dl_src": "e2:00:b7:fc:2d:c3",
        "dl_dst": "92:dc:ce:1e:53:52"
    }
 }' http://localhost:8080/stats/flow/2