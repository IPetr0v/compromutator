#!/usr/bin/env python

import json
import requests
from pprint import pprint

url = 'http://localhost:8080/stats/flow/1'
data = {
    "dpid": 1,
    "cookie": 0,
    "cookie_mask": 0,
    "table_id": 0,
    "priority": 1,
    "flags": 0,
    "match": {
        "in_port": 1,
        "dl_src": "00:00:00:00:00:01",
        "dl_dst": "00:00:00:00:00:02"
    }
}
response = requests.post(url, data=json.dumps(data))
pprint(response.json())
