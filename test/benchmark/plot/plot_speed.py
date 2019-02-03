import argparse
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

parser = argparse.ArgumentParser()
parser.add_argument('exp')

args = parser.parse_args()
print(args)

exp = args.exp
data = pd.read_csv(exp + '.csv')
print(data)

BAR_WIDTH = 0.25
INTERVAL_WIDTH = 0.02
CAP_WIDTH = 2

FACTOR = {
    'wordcount': 347,
    'pagerank': 230,
    'em': 28,
    'kmeans': 400,
    'nn': 200,
}

TITLE = {
    'wordcount': 'Wordcount',
    'pagerank': 'Pagerank',
    'em': 'Expectation Maximization (GMM)',
    'kmeans': 'K-Means',
    'nn': 'Nearest 100 Neighbors'
}

MEASURE = {
    'wordcount': 'words / second (in millions)',
    'pagerank': 'links / second / iteration (in millions)',
    'em': 'points / second / iteration (in millions)',
    'kmeans': 'points / second / iteration (in millions)',
    'nn': 'points / second (in millions)'
}

SPARK_LEGEND = {
    'wordcount': 'Spark',
    'pagerank': 'Spark (GraphX)',
    'em': 'Spark (MLlib)',
    'kmeans': 'Spark (MLlib)',
    'nn': 'Spark'
}

plt.figure(figsize=(5.0, 3.5))
plt.gca().yaxis.grid(linestyle='dotted', color='#555555')
plt.tight_layout(pad=2.0)

factor = FACTOR[exp]
for item in ['spark', 'blaze', 'blaze_opt']:
    data[item] = factor / data[item]
rects_spark = plt.bar(data.index - BAR_WIDTH - INTERVAL_WIDTH,
                      data['spark'], BAR_WIDTH, color='#c5c5c5', label=SPARK_LEGEND[exp], yerr=data['spark'] * 0.2, capsize=CAP_WIDTH)
# rects_blaze = plt.bar(data.index, data['blaze'], BAR_WIDTH, color='#019427', label='Blaze')
# rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH, data['blaze_opt'], BAR_WIDTH, color='#016A1C', label='Blaze OPT')
rects_blaze = plt.bar(
    data.index, data['blaze'], BAR_WIDTH, color='#94cddb', label='Blaze', yerr=data['blaze'] * 0.10, capsize=CAP_WIDTH)
    # data.index, data['blaze'], BAR_WIDTH, color='#0071b5', label='Blaze', yerr=data['blaze'] * 0.10, capsize=CAP_WIDTH)
# plt.errorbar(data.index, factor / data['blaze'], factor / data['blaze'] * 0.2, ecolor='#666666', fmt='none')
rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH,
                          data['blaze_opt'], BAR_WIDTH, color='#68aec4', label='Blaze TCM', yerr=data['blaze_opt'] * 0.05, capsize=CAP_WIDTH)
                          # data['blaze_opt'], BAR_WIDTH, color='#2091d5', label='Blaze TCM', yerr=data['blaze_opt'] * 0.05, capsize=CAP_WIDTH)
# rects_blaze = plt.bar(data.index, data['blaze'], BAR_WIDTH, color='#fb8c00', label='Blaze')
# rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH, data['blaze_opt'], BAR_WIDTH, color='#ef6c00', label='Blaze OPT')

plt.legend()
plt.xticks(data.index, ('2', '4', '8', '16'))
# plt.xticks(data.index - (BAR_WIDTH + INTERVAL_WIDTH) / 2, ('2', '4', '8', '16'))
plt.title(TITLE[exp] + ' Performance')
plt.xlabel('number of nodes')
plt.ylabel(MEASURE[exp])
plt.savefig(exp + '_speed.eps', format='eps', dpi=300)
plt.show()
