import re
import os
import subprocess
import requests
import json
import random
from collections import OrderedDict
from time import time, sleep
from retry import retry

from mininet.node import RemoteController, Ryu
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.topo import LinearTopo
from mininet.topolib import Topo, TreeTopo, TorusTopo
from mininet.examples.cluster import MininetCluster, SwitchBinPlacer


class Compromutator:
    def __init__(self, path, controller, log_file):
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
            #preexec_fn=lambda: os.nice(-2))

    def stop(self):
        self._process.kill()
        os.popen('sudo pkill compromutator')

    def _check_path(self):
        return os.path.isfile(self.path) and os.access(self.path, os.X_OK)


class Rule:
    """OpenFlow forwarding rule"""

    def __init__(self, dpid, info):
        if type(dpid) == str:
            self.dpid = int(dpid, 16)
        else:
            self.dpid = dpid

        if type(info) is str:
            self._from_str(info)
        elif type(info) is dict:
            self._from_dict(info)
        else:
            raise ValueError('Wrong input type', type(info))

    def _from_str(self, info):
        # Parse dpctl output
        self.cookie = re.findall(r'cookie=(\w+)', info)[0]
        self.table_id = int(re.findall(r'table=(\d+)', info)[0])
        self.packet_count = int(re.findall(r'n_packets=(\d+)', info)[0])
        self.byte_count = int(re.findall(r'n_bytes=(\d+)', info)[0])
        self.priority = int(re.findall(r'priority=(\d+)', info)[0])
        self.actions = re.findall(r'actions=(\S+)', info)[0]

        # Parse match
        match_str = re.findall(r'priority=\d+,(\S+)', info)
        if match_str:
            self.match = dict(re.findall(
                r'(\w+)=(\"\S+\"|\S[^,]*)',
                match_str[0]))
            if 'in_port' in self.match:
                self.match['in_port'] = self._get_port(self.match['in_port'])
        else:
            self.match = dict()

    def _from_dict(self, info):
        self.cookie = str(hex(info['cookie']))
        self.table_id = int(info['table_id'])
        self.priority = int(info['priority'])
        self.packet_count = int(info['packet_count'])
        self.byte_count = int(info['byte_count'])
        self.match = info['match']
        self.actions = info['actions']

    def to_str(self):
        match = self._match_str()
        info = 'cookie=%s/-1, table=%d, %s'
        info = info % (self.cookie, self.table_id, match)
        return info

    def to_dict(self):
        # Create rule dict for REST API (without actions)
        info = dict()
        info['dpid'] = self.dpid
        info['cookie'] = self.cookie
        # TODO: Use cookie_mask
        #info['cookie_mask'] = -1
        info['table_id'] = self.table_id
        info['priority'] = self.priority
        info['match'] = self.match
        info['flags'] = 0
        return info

    def _get_port(self, port_str):
        try:
            port = int(port_str)
        except ValueError:
            eth_port = re.findall(r'\w+-eth(\d+)', port_str)
            if eth_port:
                port = int(eth_port[0])
            else:
                raise ValueError('Wrong port format', port_str)
        return port

    def _match_str(self):
        match_list = [
            '='.join([key, str(val)]) for key, val in self.match.items()]
        return ','.join(match_list)

    def __str__(self):
        match = self._match_str()
        info = 'dpid=%s, cookie=%s, table=%d, ' \
               'n_packets=%d, n_bytes=%d, ' \
               'priority=%d,%s'
        info = info % (self.dpid, self.cookie, self.table_id,
                       self.packet_count, self.byte_count,
                       self.priority, match)
        return info

    def __eq__(self, other):
        # TODO: Check actions
        return self.dpid == other.dpid and \
               self.cookie == other.cookie and \
               self.table_id == other.table_id and \
               self.priority == other.priority and \
               self.match == other.match


class DPCTL:
    def __init__(self, switches):
        self.switches = OrderedDict([(int(s.dpid, 16), s) for s in switches])

    def rule_num(self, count_zero=True):
        #rule_num = 0
        #for output in self._ofctl('dump-aggregate'):
        #    rule_num += int(re.findall(r'flow_count=(\d+)', output)[0])

        rule_num = 0
        for switch in self.switches.values():
            output = switch.dpctl('dump-aggregate', '-O OpenFlow13')
            rule_num += int(re.findall(r'flow_count=(\d+)', output)[0])

        #if not count_zero:
        #    zero_rule_num = 0
        #    for output in self._ofctl('dump-flows table=0'):
        #        zero_rule_num += len(output.splitlines())
        #    rule_num -= zero_rule_num

        if not count_zero:
            zero_rule_num = 0
            for switch in self.switches.values():
                output = switch.dpctl('dump-flows', 'table=0', '-O OpenFlow13')
                zero_rule_num += len(output.splitlines())
            rule_num -= zero_rule_num

        return rule_num

    def dump_flows(self):
        rules = []
        for switch in self.switches.values():
            output = switch.dpctl('dump-flows', '-O OpenFlow13')
            output = output.splitlines()
            for line in output:
                if '0x88cc' not in line and 'xid' not in line:
                    rule = Rule(switch.dpid, line)
                    rules.append(rule)
        return rules

    #@retry(exceptions=RuntimeError, tries=5)
    def get_rule(self, rule):
        switch = self.switches.get(rule.dpid)
        if not switch:
            raise RuntimeError('Non-existing switch ', rule.dpid)

        # Request rule
        output = switch.dpctl(
            'dump-flows', '\"'+rule.to_str()+'\"', '-O OpenFlow13')
        output = output.splitlines()
        #rules = [Rule(switch.dpid, o) for o in output.splitlines()]
        rules = []
        for line in output:
            if '0x88cc' not in line and 'xid' not in line:
                rule = Rule(switch.dpid, line)
                rules.append(rule)

        # Get rule
        # dpctl doesn't allow to specify priority
        rules = [r for r in rules if r.priority == rule.priority]
        if not rules:
            raise RuntimeError('Stat request failed')
        if len(rules) > 1:
            raise RuntimeError(
                'Stat request returned too many rules', rules)
        new_rule = rules[0]
        assert new_rule == rule
        return new_rule

    def _ofctl(self, cmd):
        #shell_str = 'for sw in s1 s2; do sudo ovs-ofctl dump-flows s1 -O OpenFlow13; echo; done'
        processes = []
        for switch in self.switches.values():
            process = switch.popen(
                'ovs-ofctl ' + cmd + ' ' + switch.name + ' -O OpenFlow13')
            processes.append(process)

        outputs = []
        for process in processes:
            output, _ = process.communicate()
            outputs.append(output)

        return outputs


class TrafficManager:
    def __init__(self, network):
        self.network = network
        self.flows = dict()
        self._last_id = 0
        self._last_port = 10000

    def network_load(self):
        flow_size_list = [f['bandwidth'] for f in self.flows.values()]
        return sum(flow_size_list)

    def add_random_flow(self, bandwidth):
        src, dst = random.sample(self.network.hosts, 2)
        if type(bandwidth) is int:
            flow = self.add_flow(src, dst, bandwidth)
        elif type(bandwidth) is tuple:
            bandwidth = random.randint(bandwidth[0], bandwidth[1])
            flow = self.add_flow(src, dst, bandwidth)
        else:
            raise ValueError('Wrong bandwidth type', type(bandwidth))
        return flow

    def delete_random_flow(self):
        flow_id = random.sample(self.flows, 1)[0]
        self.delete_flow(flow_id)

    def add_flow(self, src, dst, bandwidth):
        # Init iperf parameters
        ip = dst.intfs[0].ip
        port = self._last_port
        self._last_port += 1
        server = 'iperf3 -s -B %s -p %d >/dev/null &' % (ip, port)#2>&1
        client = 'iperf3 -c %s -p %d -b %s -t 3600 &' % (#>/dev/null 2>&1
            ip, port, bandwidth)

        # Run iperf
        #print 'Add flow', ip
        #dst_pid = self._pid(dst.cmd(server, verbose=True))
        #src_pid = self._pid(src.cmd(client, verbose=True))

        dst.cmd(server)
        src.cmd(client)

        dst_pid = 1
        src_pid = 1


        # Create flow entry
        flow = dict()
        flow['src'] = src
        flow['dst'] = dst
        flow['src_pid'] = src_pid
        flow['dst_pid'] = dst_pid
        flow['bandwidth'] = bandwidth
        self.flows[self._last_id] = flow
        self._last_id += 1
        return flow

    def delete_flow(self, flow_id):
        flow = self.flows[flow_id]
        flow['src'].cmd('sudo kill -9 %d' % flow['src_pid'])
        flow['dst'].cmd('sudo kill -9 %d' % flow['dst_pid'])
        del self.flows[flow_id]

    def _pid(self, bash_output):
        parsed = re.findall(r'\[\d+\]\s(\d+)', bash_output.strip())
        print bash_output
        if len(parsed) != 1:
            raise RuntimeError('iperf3 error', bash_output)
        return int(parsed[0])


class Controller(Ryu):
    def __init__(self, port=6633, rest_port=8080):
        args = '--observe-links'
        apps = 'ryu.app.simple_switch_13 ryu.app.gui_topology.gui_topology'
        ryu_args = args + ' ' + apps
        Ryu.__init__(self, 'ryu', ryu_args, port=port)
        self.url = 'http://localhost:%d' % rest_port

    #@retry(exceptions=RuntimeError, tries=5)
    def get_rule(self, rule):
        # Create REST request
        url = self.url + '/stats/flow/%d' % rule.dpid
        data = json.dumps(rule.to_dict())
        headers = {'Content-Type': 'application/json'}

        # Get rule from controller REST
        response = requests.get(url, data=data, headers=headers)
        if response.status_code != 200:
            raise RuntimeError(
                'GET /stats/flow/ {}'.format(response.status_code))
        if not response.text:
            raise RuntimeError('No rules found')

        # Get rule from the REST response
        response = json.loads(response.text)
        assert len(response.items()) == 1
        dpid, rule_info = response.items()[0]
        dpid = int(dpid)
        assert dpid == rule.dpid
        #assert len(rule_info) < 2, rule_info
        #assert len(rule_info) == 1, rule_info

        if not rule_info:
            raise RuntimeError('Stat request failed')
        if len(rule_info) > 1:
            raise RuntimeError(
                'Stat request returned too many rules', rule_info)

        new_rule = Rule(dpid, rule_info[0])
        assert new_rule == rule
        return new_rule


class OpenFlowTestbed:
    def __init__(self, topo, port=6653, controller_port=6653, servers=None):
        self.clear()
        self.controller = Controller(controller_port)
        if not servers:
            self.network = Mininet(
                topo=topo,
                controller=RemoteController(self.controller.name, port=port))
        else:
            self.network = MininetCluster(
                topo=topo, servers=servers,
                placement=SwitchBinPlacer,
                controller=RemoteController(self.controller.name, port=port))
        self.dpctl = DPCTL(self.network.switches)
        self.traffic_manager = TrafficManager(self.network)

    def __enter__(self):
        self._start()
        self._configure_network()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._stop()

    def switches(self):
        return [s.dpid for s in self.network.switches]

    def switch_num(self):
        return len(self.network.switches)

    def rules(self):
        return self.dpctl.dump_flows()

    def rule_num(self):
        return self.dpctl.rule_num(count_zero=True)

    def hosts(self):
        return self.network.hosts

    def add_flow(self, bandwidth):
        self.traffic_manager.add_random_flow(bandwidth=bandwidth)

    def delete_flow(self):
        self.traffic_manager.delete_random_flow()

    def flow_num(self):
        return len(self.traffic_manager.flows)

    def network_load(self):
        return self.traffic_manager.network_load()

    def _start(self):
        self.controller.start()
        self.network.start()

    def _stop(self):
        self.controller.stop()
        self.network.stop()

    def _configure_network(self):
        # Disable IPv6
        for node in self.network.hosts + self.network.switches:
            for intf in ['all', 'default', 'lo']:
                node.cmd('sysctl -w net.ipv6.conf.%s.disable_ipv6=1' % intf)

    def cli(self):
        CLI(self.network)

    @staticmethod
    def clear():
        os.popen('sudo pkill iperf3')
        os.popen('sudo mn -c > /dev/null 2>&1')
        os.popen('sudo pkill ryu')


class Testbed(OpenFlowTestbed):
    def __init__(self, topo, path='~/lib/compromutator/compromutator',
                 log_file='log.txt', load_time=5, servers=None):
        OpenFlowTestbed.__init__(
            self, topo=topo, port=6653, controller_port=6633, servers=servers)
        self.compromutator = Compromutator(
            path=path, controller='127.0.0.1', log_file=log_file)
        self.load_time = load_time

    def rules(self):
        rule_list = OpenFlowTestbed.rules(self)
        rule_list = [r for r in rule_list if r.table_id != 0]
        return rule_list

    def rule_num(self):
        return self.dpctl.rule_num(count_zero=False)

    @retry(exceptions=RuntimeError, tries=5)
    def get_counter(self, rule):
        # Get rules
        real_rule = self.dpctl.get_rule(rule)
        rule.table_id -= 1
        predicted_rule = self.controller.get_rule(rule)
        predicted_rule.table_id += 1
        real_rule_after = self.dpctl.get_rule(real_rule)
        assert real_rule == predicted_rule

        # Get counters
        # Real counter is a mean value before and after the prediction
        real = dict()
        real['packet_count'] = real_rule.packet_count
        real['packet_count'] += real_rule_after.packet_count
        real['packet_count'] //= 2
        real['byte_count'] = real_rule.byte_count
        real['byte_count'] += real_rule_after.byte_count
        real['byte_count'] //= 2

        predicted = dict()
        predicted['packet_count'] = predicted_rule.packet_count
        predicted['byte_count'] = predicted_rule.byte_count

        return real, predicted

    def _start(self):
        OpenFlowTestbed._start(self)
        self.compromutator.start()
        sleep(self.load_time)

    def _stop(self):
        self.compromutator.stop()
        OpenFlowTestbed._stop(self)

    @staticmethod
    def clear():
        OpenFlowTestbed.clear()
        os.popen('sudo pkill compromutator')


class DebugTestbed(Testbed):
    def __init__(self, topo, servers=None):
        OpenFlowTestbed.__init__(
            self, topo=topo, port=6653, controller_port=6633, servers=servers)

    def _start(self):
        OpenFlowTestbed._start(self)

    def _stop(self):
        OpenFlowTestbed._stop(self)

    @staticmethod
    def clear():
        OpenFlowTestbed.clear()


class DebugTopo(Topo):
    def __init__(self):
        Topo.__init__(self)

        h1 = self.addHost('h1', ip='10.0.0.1/24', mac='00:00:00:00:00:01')
        h2 = self.addHost('h2', ip='10.0.0.2/24', mac='00:00:00:00:00:02')

        s1 = self.addSwitch('s1', protocols='OpenFlow13', dpid='1')
        s2 = self.addSwitch('s2', protocols='OpenFlow13', dpid='2')

        self.addLink(h1, s1)
        self.addLink(h2, s2)
        self.addLink(s1, s2)


setLogLevel('error')
