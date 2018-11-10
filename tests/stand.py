import re
import os
import subprocess

from mininet.node import RemoteController, Ryu
from mininet.net import Mininet
from mininet.topo import LinearTopo


class Rule:
    """OpenFlow forwarding rule"""

    def __init__(self, dpctl_output):
        #print dpctl_output
        self.cookie = re.findall(r'(cookie)=(\w+)', dpctl_output)[0][1]
        #self.duration =
        self.table = int(re.findall(r'(table)=(\d+)', dpctl_output)[0][1])
        self.n_packets = int(re.findall(r'(n_packets)=(\d+)', dpctl_output)[0][1])
        self.n_bytes = int(re.findall(r'(n_bytes)=(\d+)', dpctl_output)[0][1])
        #self.match =
        #self.actions =

class Dpctl:
    def __init__(self, switch):
        self.switch = switch

    def dump_flows(self):
        # TODO: use OpenFlow 1.3
        output = self.switch.dpctl('dump-flows', '-O OpenFlow13').splitlines()
        rules = [Rule(o) for o in output if '0x88cc' not in o]
        return rules

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


class Controller(Ryu):
    def __init__(self, port=6633):
        args = '--observe-links'
        apps = 'ryu.app.simple_switch_13 ryu.app.gui_topology.gui_topology'
        ryu_args = args + ' ' + apps
        Ryu.__init__(self, 'ryu', ryu_args, port=port)


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

    def __del__(self):
        self.stop()

    def switch_num(self):
        return len(self.network.switches)

    def rule_num(self, count_zero=False):
        num = 0
        for switch in self.network.switches:
            rules = Dpctl(switch).dump_flows()
            if not count_zero:
                rules = [r for r in rules if r.table != 0]
            num += len(rules)
        return num

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

    @staticmethod
    def clear():
        os.popen('sudo mn -c > /dev/null 2>&1')
        os.popen('sudo pkill compromutator')
        os.popen('sudo pkill ryu')
