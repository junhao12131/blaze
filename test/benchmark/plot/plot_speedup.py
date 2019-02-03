import argparse
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import MultipleLocator, FormatStrFormatter

parser = argparse.ArgumentParser()

args = parser.parse_args()
print(args)

exps = ['wordcount', 'pagerank', 'kmeans', 'em', 'nn']
FULLNAME = {
    'wordcount': 'Word Frequency Count',
    'pagerank': 'PageRank',
    'kmeans': 'K-Means',
    'em': 'Expectation Maximization (GMM)',
    'nn': 'Nearest 100 Neighbors',
}
speedups_blaze = {}
speedups_spark = {}
for exp in exps:
    data = pd.read_csv(exp + '.csv')
    speedups_blaze[exp] = (data['blaze'][0] / data['blaze'] * 2.0).values
    # speedups_blaze[exp] = np.insert(speedups_blaze[exp], 0, 0)

plt.figure(figsize=(5.5, 4.0))
index = np.array([2, 4, 8, 16])
# index = np.array([0, 2, 4, 8, 16])
lines = []
legends = []
for exp in exps:
    line = plt.errorbar(index, speedups_blaze[exp], yerr=speedups_blaze[exp] * 0.05, capsize=3)
    lines.append(line)
    legends.append(FULLNAME[exp])
    # line, = plt.plot(index, speedups_spark[exp], '--', color='grey', zorder=1)
    # lines.append(line)
    # legends.append(FULLNAME[exp] + 'Spark')

coord = np.arange(0, 20)
line, = plt.plot(coord, coord, '--', color='grey')
legends.append('Linear')

plt.legend(lines, legends)
plt.tight_layout(pad=2.0)
# plt.xticks(index, ('2', '4', '8', '16'))
# plt.yticks(np.arange(0, 25, 2))
plt.grid(zorder=0, linestyle='dotted')
plt.title('Speedup')
plt.xlim(0, 17)
plt.ylim(0, 25)
plt.gca().yaxis.set_minor_locator(MultipleLocator(1))
plt.xlabel('number of nodes')
plt.ylabel('speedup')
plt.savefig(exp + '_speedup.eps', format='eps', dpi=300)
plt.show()
