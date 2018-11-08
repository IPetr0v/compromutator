#!/usr/bin/env python

from time import sleep
import unittest

from mininet.cli import CLI
from mininet.topo import LinearTopo

from stand import TestStand


class TestBase(unittest.TestCase):
    TOPO = None

    def setUp(self):
        self.stand = TestStand(topo=self.TOPO)
        self.stand.start()
        sleep(1)

    def tearDown(self):
        self.stand.stop()

    @classmethod
    def setUpClass(cls):
        TestStand.clear()

    @classmethod
    def tearDownClass(cls):
        TestStand.clear()


class BasicTest(TestBase):
    TOPO = LinearTopo(2, 1)

    def test_pingall(self):
        #CLI(self.stand.network)
        drop_rate = self.stand.network.pingAll()
        self.assertEqual(0.0, drop_rate)

    # TODO: iperf deletes link between switches (check lldp)
    def test_iperf(self):
        drop_rate = self.stand.network.pingAll()
        sleep(1)
        rates = self.stand.network.iperf()
        self.assertLess(0.0, float(rates[0].split(' ')[0]))
        self.assertLess(0.0, float(rates[1].split(' ')[0]))


if __name__ == '__main__':
    unittest.main()
    #stand = TestStand(topo=LinearTopo(2, 1))
    #stand.start()
    #CLI(stand.network)
