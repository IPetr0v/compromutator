#!/usr/bin/env python

import os
import subprocess
from time import sleep
import unittest

from mininet.node import RemoteController, Ryu
from mininet.net import Mininet
from mininet.topo import LinearTopo


class TestBase(unittest.TestCase):
    def setUp(self):
        self.topo = LinearTopo(2, 1)
        self.network = Mininet(
            topo=self.topo,
            controller=self.__get_controller(6666))
            #controller=RemoteController()
        #self.network.addController('ryu', )
        self.network.start()
        sleep(1)
        self.compromutator = self.__create_compromutator()
        #os.system('./build/compromutator')
        sleep(1)

    def tearDown(self):
        self.compromutator.kill()
        self.network.stop()

    @classmethod
    def setUpClass(cls):
        TestBase.__clear_mininet()
        os.popen('sudo pkill compromutator')
        os.popen('sudo pkill ryu')

    @classmethod
    def tearDownClass(cls):
        TestBase.__clear_mininet()
        os.popen('sudo pkill compromutator')
        os.popen('sudo pkill ryu')

    @staticmethod
    def __clear_mininet():
        cmd = 'sudo mn -c > /dev/null 2>&1'
        os.popen(cmd)

    def __run_controller(self):
        ryu = ''
        ryu += 'export PYTHONPATH=$PYTHONPATH:./third_party/ryu; '
        ryu += './third_party/ryu/bin/ryu-manager '
        ryu += 'ryu.app.simple_switch_13 '
        ryu += '--observe-links --ofp-tcp-listen-port 6666 '
        cmd = 'bash -c "%s"' % ryu
        controller = subprocess.Popen(
            cmd.split(),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        return controller

    def __get_controller(self, port=6653):
        apps = 'ryu.app.simple_switch_13 ryu.app.gui_topology.gui_topology'
        args = '%s --observe-links --ofp-tcp-listen-port %i' % (apps, port)
        controller = Ryu('ryu', args)
        return controller

    def __create_compromutator(self):
        cmd = './build/compromutator'
        compromutator = subprocess.Popen(
            cmd.split(),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=True)
        return compromutator


class BasicTest(TestBase):
    def test_pingall(self):
        drop_rate = self.network.pingAll()
        self.assertEqual(0.0, drop_rate)

    # TODO: iperf deletes link between switches (check lldp)
    #def test_iperf(self):
    #    rates = self.network.iperf()
    #    self.assertLess(0.0, float(rates[0].split(' ')[0]))
    #    self.assertLess(0.0, float(rates[1].split(' ')[0]))


if __name__ == '__main__':
    unittest.main()

