#!/usr/bin/env python

import os
import sys
import re
import pandas as pd
from collections import OrderedDict
from random import shuffle
from retry import retry
from tqdm import tqdm, trange

from sys import argv
from time import time, sleep
from mininet.topo import LinearTopo
from mininet.topolib import TreeTopo, TorusTopo

from testbed import OpenFlowTestbed, Testbed, DebugTestbed


class TestBase:
    def save_dataframe(self, dataframe, filename):
        if os.path.exists(filename):
            with open(filename, 'a') as f:
                dataframe.to_csv(f, index=False, header=False)
        else:
            with open(filename, 'w') as f:
                dataframe.to_csv(f, index=False)


class DelayTest(TestBase):
    def __init__(self, topologies, result_file, max_pings=500, run_times=1):
        self.topologies = topologies
        self.result_file = result_file
        self.max_pings = max_pings
        self.run_times = run_times
        if os.path.exists(self.result_file):
            os.remove(self.result_file)

    def run(self):
        delays = []
        for _ in tqdm(range(self.run_times), desc='Iterations'):
            for topo in tqdm(self.topologies, desc='Topologies', leave=False):
                try:
                    result = self.run_topo_test(topo)
                    delays.extend(result)
                except RuntimeError:
                    pass
        return pd.concat(delays, ignore_index=True)

    @retry(exceptions=RuntimeError, tries=2)
    def run_topo_test(self, topo):
        delays = []
        with Testbed(topo) as testbed:
            self.ping_all(testbed)

            # Restart compromutator and get measurements
            testbed.stop_compromutator()
            delay = testbed.pop_perf_results()
            self.save_dataframe(delay, self.result_file)
            delays.append(delay)

            testbed.start_compromutator()
        return delays

    @retry(exceptions=RuntimeError, tries=5, jitter=1)
    def ping_all(self, testbed):
        try:
            # Create host ping sequence
            ping_pairs = []
            hosts = testbed.network.hosts
            for i in range(len(hosts)):
                for j in range(i+1, len(hosts)):
                    ping_pairs.append((i, j))
            shuffle(ping_pairs)
            ping_pairs = ping_pairs[:self.max_pings]

            for pair in tqdm(ping_pairs, desc='Ping', leave=False):
                self.ping(testbed, src=hosts[pair[0]], dst=hosts[pair[1]])

        except RuntimeError as ex:
            testbed.stop_compromutator()
            testbed.start_compromutator()
            raise ex

    @retry(exceptions=RuntimeError, tries=5, jitter=0.2)
    def ping(self, testbed, src, dst):
        output = testbed.network.pingFull(hosts=[src, dst], timeout=3)[0]
        ping_output = output[2]
        if not ping_output[4]:
            raise RuntimeError('Ping failed')


class PredictionTest(TestBase):
    def __init__(self, topologies, result_file, flow_num,
                 bandwidth_list, delay=0, run_times=1):
        self.topologies = topologies
        self.result_file = result_file
        self.flow_num = flow_num
        self.bandwidth_list = bandwidth_list
        self.delay = delay
        self.run_times = run_times

    def run(self):
        predictions = []
        for _ in tqdm(range(self.run_times), desc='Iterations'):
            for topo in tqdm(self.topologies, desc='Topologies', leave=False):
                try:
                    result = self.run_topo_test(topo)
                    predictions.extend(result)
                except RuntimeError:
                    pass
        return pd.concat(predictions, ignore_index=True)

    @retry(exceptions=RuntimeError, tries=2)
    def run_topo_test(self, topo):
        predictions = []
        with Testbed(topo) as testbed:
            for bandwidth in tqdm(self.bandwidth_list, desc='Bandwidth', leave=False):
                for _ in tqdm(range(self.flow_num), desc='Flows', leave=False):
                    testbed.add_flow(bandwidth=bandwidth)

                sleep(1)
                prediction = self.predict_all(testbed, bandwidth)

                # Restart compromutator and get measurements
                testbed.stop_compromutator()
                self.save_dataframe(prediction, self.result_file)
                predictions.append(prediction)

                testbed.start_compromutator()
        return predictions

    @retry(exceptions=RuntimeError, tries=5)
    def predict_all(self, testbed, bandwidth):
        predictions = []
        try:
            # Get rule counter predictions
            rules = testbed.rules()
            rule_num = len(rules)
            shuffle(rules)
            for rule in tqdm(rules, desc='Predictions', leave=False):
                prediction = self.predict(rule, testbed, bandwidth, rule_num)
                predictions.append(prediction)

        except RuntimeError as ex:
            testbed.stop_compromutator()
            testbed.start_compromutator()
            raise ex

        return pd.DataFrame(predictions)

    @retry(exceptions=RuntimeError, tries=5)
    def predict(self, rule, testbed, bandwidth, rule_num):
        real, predicted = testbed.get_counter(rule)

        result = OrderedDict()
        result['pred_packets'] = predicted['packet_count']
        result['real_packets'] = real['packet_count']
        result['pred_bytes'] = predicted['byte_count']
        result['real_bytes'] = real['byte_count']
        result['switch_num'] = testbed.switch_num()
        result['rule_num'] = rule_num
        result['flow_num'] = testbed.flow_num()
        result['bandwidth'] = bandwidth
        #result['load'] = testbed.network_load()
        result['delay'] = self.delay
        return result


class PerformanceTest:
    def __init__(self, result_dir, switch_num=5, run_times=1):
        self.result_dir = result_dir
        self.run_times = run_times
        if switch_num < 5:
            switch_num = 5
        self.topologies = [LinearTopo(n, 1) for n in range(
            switch_num, switch_num + 101, 5)]
        self.bandwidth_list = [b*1000000 for b in [10, 100, 1000]]

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

    def run_experimental(self):
        # TODO: delete this

        if os.path.exists('experiments/prediction.csv'):
            os.remove('experiments/prediction.csv')
        if os.path.exists('experiments/delay.csv'):
            os.remove('experiments/delay.csv')

        prediction_dfs = []
        delay_dfs = []
        for _ in tqdm(range(self.run_times), desc='Iterations'):
            for topo in tqdm(self.topologies, desc='Topologies', leave=False):
                with Testbed(topo) as testbed:
                    self.flow_num = (testbed.switch_num()*testbed.switch_num())/2 - 1
                    if self.flow_num > 1000:
                        self.flow_num = 1000

                    for bandwidth in tqdm(self.bandwidth_list, desc='Bandwidth', leave=False):
                        predictions = []
                        for _ in tqdm(range(self.flow_num), desc='Flows', leave=False):
                            testbed.add_flow(bandwidth=bandwidth)
                            sleep(0.05)

                            # Get rule counter predictions
                            rules = testbed.rules()
                            rule_num = len(rules)
                            shuffle(rules)
                            rules = rules[:1000]
                            for rule in tqdm(rules, desc='Predictions', leave=False):
                                predictions.append(self.predict(rule, testbed, rule_num))

                        # Restart compromutator and get measurements
                        testbed.stop_compromutator()
                        prediction = pd.DataFrame(predictions)
                        delay = testbed.pop_perf_results()

                        if os.path.exists('experiments/prediction.csv'):
                            with open('experiments/prediction.csv', 'a') as f:
                                prediction.to_csv(f, index=False, header=False)
                        else:
                            with open('experiments/prediction.csv', 'w') as f:
                                prediction.to_csv(f, index=False)

                        if os.path.exists('experiments/delay.csv'):
                            with open('experiments/delay.csv', 'a') as f:
                                delay.to_csv(f, index=False, header=False)
                        else:
                            with open('experiments/delay.csv', 'w') as f:
                                delay.to_csv(f, index=False)

                        prediction_dfs.append(prediction)
                        delay_dfs.append(delay)

                        testbed.start_compromutator()

        return pd.concat(prediction_dfs, ignore_index=True), \
               pd.concat(delay_dfs, ignore_index=True)

    def predict(self, rule, testbed, rule_num):
        real, predicted = testbed.get_counter(rule)

        result = OrderedDict()
        result['pred_packets'] = predicted['packet_count']
        result['real_packets'] = real['packet_count']
        result['pred_bytes'] = predicted['byte_count']
        result['real_bytes'] = real['byte_count']
        result['switch_num'] = testbed.switch_num()
        result['rule_num'] = rule_num
        result['flow_num'] = testbed.flow_num()
        result['load'] = testbed.network_load()
        return result
