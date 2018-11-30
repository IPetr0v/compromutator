#!/usr/bin/env python

import os
import glob
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def cut_by_item(dataframe, key, step):
    item_range = np.arange(0, dataframe[key].max(), step)
    grouped = dataframe.groupby(pd.cut(dataframe[key], item_range))
    keys = [key for key, _ in grouped]
    dataframes = [group for _, group in grouped]
    return keys, dataframes


def process_delays(delays):
    # By graph size
    delays = delays[delays['network_size'] % 5 == 0]
    rule_interval = 100
    max_rule_num = 100000#delays['graph_size'].max()
    delay_by_graph_size = delays.groupby(pd.cut(
        delays['graph_size'],
        np.arange(0, max_rule_num, rule_interval))
    )['duration'].median()
    #std_delay_by_graph_size = delays.groupby(pd.cut(
    #    delays['graph_size'],
    #    np.arange(0, max_rule_num, rule_interval))
    #)['duration'].std()

    first_quartile_delay_by_graph_size = delays.groupby(pd.cut(
        delays['graph_size'],
        np.arange(0, max_rule_num, rule_interval))
    )['duration'].quantile(0.25)
    third_quartile_delay_by_graph_size = delays.groupby(pd.cut(
        delays['graph_size'],
        np.arange(0, max_rule_num, rule_interval))
    )['duration'].quantile(0.75)

    keys = [(k.left + k.right)/2.0 for k in delay_by_graph_size.keys()]
    #keys = [k.left for k in delay_by_graph_size.keys()]
    data = pd.Series(delay_by_graph_size).reset_index(drop=True)
    #std = pd.Series(std_delay_by_graph_size).reset_index(drop=True)

    upper = pd.Series(third_quartile_delay_by_graph_size).reset_index(drop=True)
    lower = pd.Series(first_quartile_delay_by_graph_size).reset_index(drop=True)

    data = data.rolling(window=5, min_periods=1).mean()
    upper = upper.rolling(window=5, min_periods=1).mean()
    lower = lower.rolling(window=5, min_periods=1).mean()

    plt.plot(keys, data, 'green')
    plt.fill_between(keys, lower, upper, color='green', alpha='0.15')
    #plt.plot(keys, upper, 'red', alpha='0.25')
    #plt.plot(keys, lower, 'red', alpha='0.25')
    #plt.plot(keys, std, 'g')
    plt.xlabel('Rule number')
    plt.ylabel('Delay, ms')
    plt.title('Delay by rule number')
    plt.savefig(data_dir + '/plot/' + 'Delay by rule number.png', fmt='png')
    #plt.show()
    plt.gcf().clear()

    # By network size
    switch_interval = 10
    max_switch_num = 301#delays['network_size'].max()
    #delay_by_network_size = delays.groupby(
    #    ['network_size'])['duration'].mean()
    #first_quartile_by_network_size = delays.groupby(
    #    ['network_size'])['duration'].quantile(0.25)
    #third_quartile_by_network_size = delays.groupby(
    #    ['network_size'])['duration'].quantile(0.75)

    delay_by_network_size = delays.groupby(pd.cut(
        delays['network_size'],
        np.arange(0, max_switch_num, switch_interval))
    )['duration'].median()
    #std_by_network_size = delays.groupby(pd.cut(
    #    delays['network_size'],
    #    np.arange(0, max_switch_num, switch_interval))
    #)['duration'].std()
    first_quartile_by_network_size = delays.groupby(pd.cut(
        delays['network_size'],
        np.arange(0, max_switch_num, switch_interval))
    )['duration'].quantile(0.25)
    third_quartile_by_network_size = delays.groupby(pd.cut(
        delays['network_size'],
        np.arange(0, max_switch_num, switch_interval))
    )['duration'].quantile(0.75)

    keys = [(k.left + k.right)/2.0 for k in delay_by_network_size.keys()]

    median = pd.Series(delay_by_network_size).reset_index(drop=True)
    #std = pd.Series(std_by_network_size).reset_index(drop=True)
    upper = pd.Series(third_quartile_by_network_size).reset_index(drop=True)
    lower = pd.Series(first_quartile_by_network_size).reset_index(drop=True)

    #data = data.rolling(window=5, min_periods=1).mean()
    #upper = upper.rolling(window=5, min_periods=1).mean()
    #lower = lower.rolling(window=5, min_periods=1).mean()

    #plt.boxplot(data)
    #plt.plot(keys, median, yerr=std, color='blue')
    plt.errorbar(keys, median, yerr=[lower, upper],
                 color='blue', fmt='o', capsize=3, elinewidth=1)
    #plt.fill_between(keys, lower, upper, color='blue', alpha='0.15')
    plt.xlabel('Network size')
    plt.ylabel('Delay, ms')
    plt.title('Delay by network size')
    plt.savefig(data_dir + '/plot/' + 'Delay by network size.png', fmt='png')
    #plt.show()
    plt.gcf().clear()

    # By mean table size
    #delays_table = delays


def load(files):
    dataframe_list = []
    for dataframe_file in files:
        df = pd.read_csv(dataframe_file, index_col=None, header=0)
        dataframe_list.append(df)
    return pd.concat(dataframe_list, ignore_index=True)


if __name__ == '__main__':
    #tqdm.pandas()
    data_dir = './experiments'

    delay_files = [data_dir + '/delay.csv']#glob.glob(data_dir + '/delay*')
    #delay_files = ['./experiments/delay.csv'] #['./experiments/delay_5_100.csv']
    #prediction_files = glob.glob(data_dir + '/prediction*')

    delays = load(delay_files)
    process_delays(delays)

    #predictions = load(prediction_files)
    #process_predictions(predictions)

    # Prediction
    #result['pred_packets'] = predicted['packet_count']
    #result['real_packets'] = real['packet_count']
    #result['pred_bytes'] = predicted['byte_count']
    #result['real_bytes'] = real['byte_count']
    #result['switch_num'] = testbed.switch_num()
    #result['rule_num'] = rule_num
    #result['flow_num'] = testbed.flow_num()
    #result['load'] = testbed.network_load()




















def process_delays_new_old(delays):
    step = 90
    rule_interval = 250
    window = 100

    real_key_list, real_list = cut_by_item(delays, 'network_size', step)
    real_by_rule_list = [
        r.groupby(pd.cut(
            r['graph_size'],
            np.arange(0, r['graph_size'].max(), rule_interval))
        )['duration'].mean()
        for r in real_list
    ]

    for label, real_by_rule in zip(real_key_list, real_by_rule_list):
        real_keys = [(k.left + k.right)/2.0 for k in real_by_rule.keys()]
        keys = real_keys
        delay = pd.DataFrame(real_by_rule).rolling(window=window).mean()
        #delay = tested_by_rule - real_by_rule
        plt.plot(keys, delay, label=str(label.right))#, 'r')
        plt.legend()

    plt.xlabel('Rule number')
    plt.ylabel('Delay')
    plt.title('Delay by rule number')
    #plt.savefig(data_dir + '/plot/' + 'Delay by rule number.png', fmt='png')
    plt.show()
    plt.gcf().clear()


def process_delays_old(delays):
    step = 10
    rule_interval = 250
    window = 100

    real = delays.ix[delays['real'] == True]
    tested = delays.ix[delays['real'] == False]

    real_key_list, real_list = cut_by_item(real, 'network_size', step)
    real_by_rule_list = [
        r.groupby(pd.cut(
            r['graph_size'],
            np.arange(0, r['graph_size'].max(), rule_interval))
        )['RTT'].mean()
        for r in real_list
    ]

    tested_key_list, tested_list = cut_by_item(tested, 'network_size', step)
    tested_by_rule_list = [
        t.groupby(pd.cut(
            t['graph_size'],
            np.arange(0, t['graph_size'].max(), rule_interval))
        )['RTT'].mean()
        for t in tested_list
    ]

    for label, real_by_rule, tested_by_rule in zip(real_key_list, real_by_rule_list, tested_by_rule_list):
        real_keys = [(k.left + k.right)/2.0 for k in real_by_rule.keys()]
        tested_keys = [(k.left + k.right)/2.0 for k in tested_by_rule.keys()]
        keys = sorted(list(set(real_keys + tested_keys)))
        delay = pd.DataFrame(tested_by_rule - real_by_rule).rolling(window=window).mean()
        #delay = tested_by_rule - real_by_rule
        plt.plot(keys, delay, label=str(label.right))#, 'r')
        plt.legend()

    plt.xlabel('Rule number')
    plt.ylabel('RTT')
    plt.title('RTT by rule number')
    plt.savefig(data_dir + '/plot/' + 'RTT by rule number.png', fmt='png')
    plt.gcf().clear()


def process_delays_oldoldold(delays):
    real = delays.ix[delays['real'] == True]
    tested = delays.ix[delays['real'] == False]

    # TODO: cut and average by different switch numbers
    real = real.groupby(pd.cut(
        real['switch_num'],
        np.arange(0, real['switch_num'].max(), 10))
    )['RTT'].mean()
    tested = tested.groupby(pd.cut(
        tested['switch_num'],
        np.arange(0, tested['switch_num'].max(), 10))
    )['RTT'].mean()

    real_by_rule = real.groupby(pd.cut(
        real['rule_num'],
        np.arange(0, real['rule_num'].max(), 100))
    )['RTT'].mean()
    tested_by_rule = tested.groupby(pd.cut(
        tested['rule_num'],
        np.arange(0, tested['rule_num'].max(), 100))
    )['RTT'].mean()

    #real_by_rule = real.groupby(['rule_num'])['RTT'].agg(
    #    lambda x: x.unique().sum()/x.nunique())
    #tested_by_rule = tested.groupby(['rule_num'])['RTT'].agg(
    #    lambda x: x.unique().sum()/x.nunique())

    keys = [(k.left + k.right)/2.0 for k in real_by_rule.keys()]
    #keys = list(set(list(real_by_rule.keys()) + list(tested_by_rule.keys())))
    #print keys

    df1 = pd.DataFrame(tested_by_rule).rolling(window=100).mean()
    df2 = pd.DataFrame(real_by_rule).rolling(window=100).mean()
    #plt.plot(keys, tested_by_rule, 'r')
    #plt.plot(keys, real_by_rule, 'b')
    plt.plot(keys, df1, 'r')
    plt.plot(keys, df2, 'b')
    plt.xlabel('Rule number')
    plt.ylabel('RTT')
    plt.title('RTT by rule number')
    plt.savefig(data_dir + 'RTT by rule number.png', fmt='png')
    plt.gcf().clear()

    df = pd.DataFrame(np.abs(real_by_rule - tested_by_rule))#.rolling(window=100).mean()
    #plt.plot(keys, np.abs(real_by_rule - tested_by_rule), 'g')
    plt.plot(keys, df, 'g')
    plt.xlabel('Rule number')
    plt.ylabel('Processing delay')
    plt.title('Processing delay by rule number')
    plt.savefig(data_dir + 'Processing delay by rule number.png', fmt='png')
    plt.gcf().clear()

    #real_by_switch = real.groupby(pd.cut(
    #    real['switch_num'],
    #    np.arange(0, real['rule_num'].max(), 1))
    #)['RTT'].mean()
    #tested_by_switch = tested.groupby(pd.cut(
    #    tested['switch_num'],
    #    np.arange(0, tested['rule_num'].max(), 1))
    #)['RTT'].mean()

    real_by_switch = real.groupby(['switch_num'])['RTT'].agg(
        lambda x: x.unique().sum()/x.nunique())
    tested_by_switch = tested.groupby(['switch_num'])['RTT'].agg(
        lambda x: x.unique().sum()/x.nunique())

    keys = [k for k in real_by_switch.keys()]

    plt.plot(keys, tested_by_switch, 'r')
    plt.plot(keys, real_by_switch, 'b')
    plt.xlabel('Switch number')
    plt.ylabel('RTT')
    plt.title('RTT by Switch number')
    plt.savefig(data_dir + 'RTT by Switch number.png', fmt='png')
    plt.gcf().clear()

    plt.plot(keys, np.abs(real_by_switch - tested_by_switch), 'g')
    plt.xlabel('Switch number')
    plt.ylabel('Processing delay')
    plt.title('Processing delay by Switch number')
    plt.savefig(data_dir + 'Processing delay by Switch number.png', fmt='png')
    plt.gcf().clear()


def process_predictions(predictions):
    predictions['error'] = np.abs(
        predictions['real_packets'] - predictions['pred_packets'])

    error_by_switch = predictions.groupby(['switch_num'])['error'].agg(
        lambda x: x.unique().sum()/x.nunique())
    error_by_rule_num = predictions.groupby(pd.cut(
        predictions['rule_num'],
        np.arange(0, predictions['rule_num'].max(), 100))
    )['error'].mean()
    error_by_flow_num = predictions.groupby(['flow_num'])['error'].agg(
        lambda x: x.unique().sum()/x.nunique())
    error_by_load = predictions.groupby(['load'])['error'].agg(
        lambda x: x.unique().sum()/x.nunique())

    keys = [k for k in error_by_switch.keys()]
    plt.plot(keys, error_by_switch, 'r')
    plt.xlabel('Switch number')
    plt.ylabel('Error')
    plt.title('Error by switch number')
    plt.savefig(data_dir + 'Error by switch number.png', fmt='png')
    plt.gcf().clear()

    keys = [(k.left + k.right)/2.0 for k in error_by_rule_num.keys()]
    plt.plot(keys, error_by_rule_num, 'r')
    plt.xlabel('Rule number')
    plt.ylabel('Error')
    plt.title('Error by rule number')
    plt.savefig(data_dir + 'Error by rule number.png', fmt='png')
    plt.gcf().clear()

    keys = [k for k in error_by_flow_num.keys()]
    plt.plot(keys, error_by_flow_num, 'r')
    plt.xlabel('Flow number')
    plt.ylabel('Error')
    plt.title('Error by flow number')
    plt.savefig(data_dir + 'Error by flow number.png', fmt='png')
    plt.gcf().clear()

    keys = [k for k in error_by_load.keys()]
    plt.plot(keys, error_by_load, 'r')
    plt.xlabel('Load')
    plt.ylabel('Error')
    plt.title('Error by load')
    plt.savefig(data_dir + 'Error by load.png', fmt='png')
    plt.gcf().clear()