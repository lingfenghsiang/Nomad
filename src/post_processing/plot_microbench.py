import matplotlib.pyplot as plt
import pandas as pd
import os

microbench_logs = ["zipfan_hottest_10G.read.log.csv" ,  "zipfan_hottest_13.5G.read.log.csv" ,  "zipfan_hottest_27G.read.log.csv",
 "zipfan_hottest_10G.write.log.csv",  "zipfan_hottest_13.5G.write.log.csv" , "zipfan_hottest_27G.write.log.csv"]

test_name = ["small-read", "medium-read", "large-read", "small-write", "medium-write", "large-write"]

microbench_log_dir = ["microbench_memtis" , "microbench_nomad" , "microbench_tpp"]

legends = ["memtis", "nomad", "tpp"]

base_dir = "src/post_processing/tmp"

def plot_microbench_bw(log_names, legends, graph_name):
    steady = []
    transient = []
    for i in log_names:
        df = pd.read_csv(i, index_col=0)
        transient.append(df.loc[1, "Bandwidth(MB/s)"])
        steady.append(df.loc[4, "Bandwidth(MB/s)"])
    fig, (ax0, ax1) = plt.subplots(1, 2)
    ax0.bar(range(len(transient)) , transient)
    ax0.set_xticks(range(len(transient)), legends)
    ax1.bar(range(len(steady)) , steady)
    ax1.set_xticks(range(len(steady)), legends)
    ax0.sharey(ax1)
    ax0.set_ylabel('Bandwidth (MB/s)',fontsize = 18)
    ax0.set_title("migration in progress")
    ax1.set_title("migration stable")
    foo_fig = plt.gcf()
    foo_fig.savefig(graph_name)
    

for log_name, graph_name in zip(microbench_logs, test_name) :
    logs = []
    for dir in microbench_log_dir:
        log_to_be_ploted = os.path.join(base_dir, dir, log_name)
        if(not os.path.exists(log_to_be_ploted)):
            print(log_to_be_ploted, "not found")
            exit(1)
        logs.append(log_to_be_ploted)
    plot_microbench_bw(logs, legends, os.path.join(base_dir,"microbench" + graph_name + ".png"))
    