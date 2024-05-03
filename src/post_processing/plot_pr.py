import glob
import os
import re
import numpy as np
import matplotlib.pyplot as plt

regex = r"execution time ([0-9.]*) \(s\)"

def get_speed(log_name):
    with open(log_name, "r") as f:
        while True:
            line = f.readline()
            if not line:
                print("bad log")
                exit(1)
            searched = re.search(regex, line)
            if searched:
                return 1 / float(searched.groups()[0])

def plot_pr():
    pr_dir = glob.glob("src/tmp/results/pageranking*")
    cases = []
    data = []
    for i in pr_dir:
        cases.append(os.path.basename(os.path.normpath(i)))
        logname = os.path.join(i, "results-pr", "output.log")
        data.append(get_speed(logname))
    normalized_speed = np.array(data) / np.min(np.array(data))
    fig, ax = plt.subplots(1, 1)
    ax.bar(np.arange(len(cases)), normalized_speed)
    ax.set_xticks(np.arange(len(cases)), cases, rotation = 45)
    ax.set_ylabel('Normalized speed',fontsize = 18)
    foo_fig = plt.gcf()
    foo_fig.savefig("src/post_processing/tmp/pageranking.png", bbox_inches='tight')

plot_pr()