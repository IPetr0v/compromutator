#!/usr/bin/env python

import sys
from time import sleep

from mininet.cli import CLI
from mininet.log import setLogLevel, info, error
from mininet.net import Mininet
from mininet.link import Intf, TCLink
from mininet.topolib import Topo
from mininet.util import quietRun
from mininet.node import RemoteController, Ryu
from mininet.topo import LinearTopo, SingleSwitchTopo
from mininet.topolib import TreeTopo, TorusTopo

from testbed import OpenFlowTestbed, Testbed, DebugTestbed


#class TestTopo(Topo):
#    """Test topology"""
#
#    def __init__(self, size):
#        """Create test topology"""
#        Topo.__init__(self)
#        switches = []
#
#        h1 = self.addHost('h1', ip='10.0.0.1/24', mac='00:00:00:00:00:01')
#        h2 = self.addHost('h2', ip='10.0.0.2/24', mac='00:00:00:00:00:02')
#
#        for i in range(size):
#            idx = i + 1
#            switch = self.addSwitch(
#                's%i' % idx, protocols='OpenFlow13', dpid='%i' % idx)
#            switches.append(switch)
#
#        self.addLink(switches[0], h1, cls=TCLink, bw=10)
#        self.addLink(switches[-1], h2, cls=TCLink, bw=10)
#
#        for i in range(size-1):
#            self.addLink(switches[i], switches[i+1], cls=TCLink, bw=10)
#
#        if size > 1:
#            root = self.addSwitch(
#                'root', protocols='OpenFlow13', dpid=str(size+1))
#            self.addLink(root, switches[size/2], cls=TCLink, bw=10)
#            self.addLink(h3, root, cls=TCLink, bw=10)
#        else:
#            self.addLink(h3, switches[0], cls=TCLink, bw=10)
# TestTopo(10) LinearTopo(2, 1, lopts={'cls':TCLink, 'bw':10}) SingleSwitchTopo(2)

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


if __name__ == '__main__':
    setLogLevel('info')

    # Create test stand
    testbed = DebugTestbed(topo=LinearTopo(50, 1))

    # Emulate network
    with testbed:
        testbed.cli()
