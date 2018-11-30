#!/usr/bin/env python

import sys
import glob
import pandas as pd


def load(files):
    dataframe_list = []
    for dataframe_file in files:
        df = pd.read_csv(dataframe_file, index_col=None, header=0)
        dataframe_list.append(df)
    if len(dataframe_list) > 1:
        return pd.concat(dataframe_list, ignore_index=True)
    else:
        return dataframe_list[0]


if __name__ == '__main__':
    #for arg in sys.argv[1:]:
    #    print arg

    # Load files
    data_dir = './experiments'
    delay_files = glob.glob(data_dir + '/delay*')
    print 'Merging', delay_files
    delays = load(delay_files)

    # Preprocess dataframe
    delays = delays[delays['duration'] > 2]
    #delays = delays[delays['network_size'] != 300]
    delays = delays.sort_values(['graph_size'])

    # Save dataframe
    with open('experiments/delay.csv', 'w') as f:
        delays.to_csv(f, index=False)
