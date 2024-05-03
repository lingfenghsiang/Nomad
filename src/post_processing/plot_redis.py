import re
import os
import matplotlib.pyplot as plt
import numpy as np

redis_logs = ["redis.small.log", "redis.large.log", "redis.noevict.log"]

cases = ["case 1", "case 2", "case 3"]

redis_log_dir = ["redis-5.13.0-rc6nomad", "redis-5.13.0-rc6tpp"]

legends = ["nomad", "tpp"]

base_dir = "src/tmp/results"

regex = r"\[OVERALL\], Throughput\(ops\/sec\), ([0-9.]*)"


def get_redis_speed(log_name):
    with open(log_name, "r") as f:
        while True:
            line = f.readline()
            if not line:
                return 0.0
            searched = re.search(regex, line)
            if searched:
                return float(searched.groups()[0])

def calc_pos(total_nr, idx):
    width = 0.7 / total_nr
    offset = (idx - (total_nr - 1) / 2) * width
    return offset, width

def plot_redis():
    fig, ax = plt.subplots(1, 1)
    for idx, j in enumerate(redis_log_dir):
        data = []

        for i, case in zip(redis_logs, cases):

            log_name = os.path.join(base_dir, j, i)
            if(not os.path.exists(log_name)):
                print(log_name, "not found")
                exit(1)
            data.append(get_redis_speed(log_name))
        x, wid = calc_pos(len(legends), idx)
        ax.bar(np.arange(len(cases)) + x, np.array(data) / 1000.0, wid, label = legends[idx])
    ax.set_xticks(np.arange(len(cases)), cases)
    ax.set_ylabel('Throughput (kOps/s)',fontsize = 18)
    ax.legend()
    foo_fig = plt.gcf()
    foo_fig.savefig("src/post_processing/tmp/redis.png")

plot_redis()