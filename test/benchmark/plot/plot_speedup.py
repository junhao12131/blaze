import argparse
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import MultipleLocator, FormatStrFormatter

parser = argparse.ArgumentParser()

args = parser.parse_args()
print(args)

exps = ['wordcount', 'wordcount', 'wordcount', 'wordcount', 'wordcount']
FULLNAME = {
    'wordcount': 'Word Count'
}
speedups_blaze = {}
speedups_spark = {}
for exp in exps:
    data = pd.read_csv(exp + '.csv')
    speedups_blaze[exp] = (data['blaze'] / data['spark'][0] * 2.0).values
    speedups_blaze[exp] = np.insert(speedups_blaze[exp], 0, 0)
    speedups_spark[exp] = (data['spark'] / data['spark'][0] * 2.0).values
    speedups_spark[exp] = np.insert(speedups_spark[exp], 0, 0)

plt.figure(figsize=(5.5, 4.0))
index = np.array([0, 2, 4, 8, 16])
lines = []
legends = []
for exp in exps:
    line, = plt.plot(index, speedups_blaze[exp], '-', color='grey', zorder=1)
    lines.append(line)
    legends.append(FULLNAME[exp] + 'Blaze')
    line, = plt.plot(index, speedups_spark[exp], '--', color='grey', zorder=1)
    # lines.append(line)
    # legends.append(FULLNAME[exp] + 'Spark')

plt.legend(lines, legends)
# plt.xticks(index, ('2', '4', '8', '16'))
# plt.yticks(np.arange(0, 25, 2))
plt.grid(zorder=0, linestyle='dotted')
plt.title('Blaze Speedup')
plt.xlim(2, 16)
plt.ylim(0, 25)
plt.gca().yaxis.set_minor_locator(MultipleLocator(1))
plt.xlabel('Number of nodes')
plt.ylabel('Speedup (Spark 2 nodes speed = 2.0)')
plt.savefig(exp + '_speedup.eps', format='eps', dpi=300)
plt.show()
