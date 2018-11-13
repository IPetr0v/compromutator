import re
import os
import subprocess
import requests
import json
import sys

from mininet.node import RemoteController, Ryu
from mininet.net import Mininet
from mininet.topo import LinearTopo


class Compromutator:
    def __init__(self, path, log_file):
        self.path = os.path.expanduser(path)
        self.args = ''
        self.log_file = open(log_file, 'w') if log_file else None
        self._process = None

        if not self._check_path():
            raise ValueError('%s: not found' % path)

    def start(self):
        cmd = self.path + self.args
        self._process = subprocess.Popen(
            cmd.split(),
            stdin=subprocess.PIPE,
            stdout=self.log_file if self.log_file else subprocess.PIPE,
            stderr=self.log_file if self.log_file else subprocess.STDOUT,
            shell=True)

    def stop(self):
        self._process.kill()

    def _check_path(self):
        return os.path.isfile(self.path) and os.access(self.path, os.X_OK)


class Rule:
    """OpenFlow forwarding rule"""

    def __init__(self, dpid, info):
        self.dpid = dpid
        if type(info) is str:
            self._parse_str(info)
        elif type(info) is json:
            self._parse_dict(info)
        else:
            raise ValueError('Wrong input type', type(info))

    def _parse_str(self, info):
        self.cookie = re.findall(r'(cookie)=(\w+)', info)[0][1]
        self.table_id = int(re.findall(r'(table)=(\d+)', info)[0][1])
        self.packet_count = int(re.findall(r'(n_packets)=(\d+)', info)[0][1])
        self.byte_count = int(re.findall(r'(n_bytes)=(\d+)', info)[0][1])
        #self.match =
        #self.actions =

    def _parse_dict(self, info):
        self.cookie = info['cookie']
        self.table = info['table_id']
        self.priority = info['priority']
        self.packet_count = info['packet_count']
        self.byte_count = info['byte_count']
        self.match = info['match']
        self.actions = info['actions']

    def to_str(self):
        pass

    def to_dict(self):
        # Create rule dict for REST API (without actions)
        info = dict()
        info['flags'] = 0
        info['cookie'] = self.cookie
        # TODO: check cookie mask correctness
        #info['cookie_mask'] =
        info['table_id'] = self.table_id
        info['priority'] = self.priority
        info['match'] = self.match
        return info


class Dpctl:
    def __init__(self, switch):
        self.switch = switch

    def dump_flows(self):
        # TODO: use OpenFlow 1.3
        output = self.switch.dpctl('dump-flows', '-O OpenFlow13').splitlines()
        rules = [Rule(self.switch.dpid, o) for o in output if '0x88cc' not in o]
        return rules


class Controller(Ryu):
    def __init__(self, port=6633, rest_port=8080):
        args = '--observe-links'
        apps = 'ryu.app.simple_switch_13 ryu.app.gui_topology.gui_topology'
        ryu_args = args + ' ' + apps
        Ryu.__init__(self, 'ryu', ryu_args, port=port)
        self.rest_port = rest_port

    def get_rule_info(self, rule):
        url = 'http://localhost:%d/stats/flow/%d' % (self.rest_port, rule.dpid)
        data = json.dumps(rule.to_dict())
        headers = {'Content-Type':'application/json'}
        response = requests.get(url, data=data, headers=headers)
        if response.status_code != 200:
            raise ValueError('GET /stats/flow/ {}'.format(response.status_code))
        print response



#class NetworkBuilder:
#    def __init__(self, topo, controller):
#        self.network = Mininet(
#            topo=topo,
#            controller=RemoteController(self.controller.name, port=port))
#
#    def __call__(self):
#        return self.network


class TestStand:
    def __init__(self, topo, port=6653, controller_port=6633,
                 run_compromutator=True,
                 path='~/lib/compromutator/compromutator',
                 log_file=None):
        self.controller = Controller(controller_port)
        self.network = Mininet(
            topo=topo,
            controller=RemoteController(self.controller.name, port=port))
        self.run_compromutator = run_compromutator
        if self.run_compromutator:
            self.compromutator = Compromutator(path=path, log_file=log_file)
        else:
            self.compromutator = None

    # TODO: use __enter__ and __exit__

    def __del__(self):
        self.stop()

    def rules(self, count_zero=False):
        rule_list = []
        for switch in self.network.switches:
            switch_rules = Dpctl(switch).dump_flows()
            if not count_zero:
                switch_rules = [r for r in switch_rules if r.table_id != 0]
            rule_list += switch_rules
        return rule_list

    def get_counter(self, rule):
        return real_counter, predicted_counter

    def switch_num(self):
        return len(self.network.switches)

    def rule_num(self, count_zero=False):
        return len(self.rules(count_zero))

    def start(self):
        self.controller.start()
        self.network.start()
        if self.run_compromutator:
            self.compromutator.start()

    def stop(self):
        if self.run_compromutator:
            self.compromutator.stop()
        self.controller.stop()
        self.network.stop()

    def configure_network(self):
        # Disable IPv6
        for node in self.network.hosts + self.network.switches:
            node.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
            node.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
            node.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")

    @staticmethod
    def clear():
        os.popen('sudo mn -c > /dev/null 2>&1')
        os.popen('sudo pkill compromutator')
        os.popen('sudo pkill ryu')
