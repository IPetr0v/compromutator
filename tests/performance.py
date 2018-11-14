#!/usr/bin/env python

import os
import re
import pandas as pd
from collections import OrderedDict
from random import shuffle
from retry import retry
from tqdm import tqdm, trange

from sys import argv
import matplotlib.pyplot as plt
from time import time, sleep
from mininet.topo import LinearTopo
from mininet.topolib import TreeTopo, TorusTopo

from testbed import OpenFlowTestbed, Testbed, DebugTestbed


class PingDelayTest:
    def __init__(self, topo):
        self.topo = topo

    @retry(exceptions=RuntimeError, tries=5)
    def run(self):
        with OpenFlowTestbed(self.topo) as real_testbed:
            real_results = self.pingall(real_testbed, real=True)
        with Testbed(self.topo) as testbed:
            results = self.pingall(testbed, real=False)
        return pd.DataFrame(results + real_results)

    def pingall(self, testbed, real=False):
        results = []

        # Create host ping sequence
        ping_pairs = []
        hosts = testbed.network.hosts
        for i in range(len(hosts)):
            for j in range(i+1, len(hosts)):
                ping_pairs.append((i, j))
        shuffle(ping_pairs)

        for pair in tqdm(ping_pairs, desc='Ping', leave=False):
            # Run ping test
            # TODO: remove sleep and fix rule installation
            sleep(1)
            result = self.ping(
                testbed, src=hosts[pair[0]], dst=hosts[pair[1]], real=real)
            results.append(result)
        return results

    def ping(self, testbed, src, dst, real=False):
        # Get first ping since only it influences rule installation
        output = testbed.network.pingFull(hosts=[src, dst], timeout=0.5)[0]
        ping_output = output[2]

        # Get max RTT
        result = OrderedDict()
        result['RTT'] = ping_output[4]
        result['switch_num'] = testbed.switch_num()
        result['rule_num'] = testbed.rule_num()
        result['real'] = real

        if not result['RTT']:
            raise RuntimeError(
                'Ping failed on %d switches' % result['switch_num'])
        return result


class PredictionTest:
    def __init__(self, topo, flow_num, bandwidth, request_num=1):
        self.topo = topo
        self.flow_num = flow_num
        self.bandwidth = bandwidth
        #self.delay = delay
        self.request_num = request_num

        # TODO: Make consistency test

    @retry(exceptions=RuntimeError, tries=5)
    def run(self):
        results = []
        with Testbed(self.topo) as testbed:
            for _ in tqdm(range(self.flow_num), desc='Flows', leave=False):
                testbed.add_flow(bandwidth=self.bandwidth)
                sleep(1)

                # Get rule counter predictions
                for _ in tqdm(range(self.request_num),
                              desc='Requests', leave=False):
                    rules = testbed.rules()
                    shuffle(rules)
                    for rule in rules:
                        results.append(self.predict(rule, testbed))
        return pd.DataFrame(results)

    def predict(self, rule, testbed):
        real, predicted = testbed.get_counter(rule)

        # DEBUG
        #if predicted > real + 10:
        #    os.popen('sudo pkill iperf3')
        #    print rule.to_dict()
        #    testbed.cli()

        #print 'packets: %d/%d | bytes: %d/%d' % (
        #    predicted['packet_count'], real['packet_count'],
        #    predicted['byte_count'], real['byte_count'])

        result = OrderedDict()
        result['pred_packets'] = predicted['packet_count']
        result['real_packets'] = real['packet_count']
        result['pred_bytes'] = predicted['byte_count']
        result['real_bytes'] = real['byte_count']
        result['switch_num'] = testbed.switch_num()
        result['rule_num'] = testbed.rule_num()
        result['flow_num'] = testbed.flow_num()
        result['load'] = testbed.network_load()
        #result['delay'] = self.delay
        return result


class PerformanceTest:
    def __init__(self, result_dir, run_times=1):
        self.result_dir = result_dir
        self.run_times = run_times
        self.topologies = [LinearTopo(n, 1) for n in range(5, 106, 5)]
        self.topologies = [LinearTopo(1, 2)] + self.topologies
        self.bandwidth_list = [b*1000000 for b in [10, 100, 1000]]

    def run_delay_tests(self):
        path = os.path.join(self.result_dir, 'delay.csv')
        results = []
        for _ in tqdm(range(self.run_times), desc='Iterations'):
            for topo in tqdm(self.topologies, desc='Topologies', leave=False):
                result = PingDelayTest(topo).run()
                results.append(result)

                # Save intermediate results
                data = pd.concat(results, ignore_index=True)
                data.to_csv(path, index=False)
        print 'Saved to', path
        return results

    def run_prediction_tests(self):
        path = os.path.join(self.result_dir, 'prediction.csv')
        results = []
        for _ in tqdm(range(self.run_times), desc='Iterations'):
            for topo in tqdm(self.topologies, desc='Topologies', leave=False):
                for bandwidth in tqdm(self.bandwidth_list,
                                      desc='Bandwidth', leave=False):
                    result = PredictionTest(
                        topo, flow_num=len(topo.hosts()),
                        bandwidth=bandwidth, request_num=1).run()
                    results.append(result)

                    # Save intermediate results
                    data = pd.concat(results, ignore_index=True)
                    data.to_csv(path, index=False)
        print 'Saved to', path
        return results

if __name__ == '__main__':
    test = PerformanceTest(result_dir='./experiments', run_times=1)

    #print '--- Delay Test --'
    #delay = test.run_delay_tests()

    print '--- Prediction Test --'
    prediction = test.run_prediction_tests()

    print 'delay', delay
    print 'prediction', prediction
