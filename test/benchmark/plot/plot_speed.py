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

plt.figure(figsize=(5.0, 3.5))

rects_spark = plt.bar(data.index - BAR_WIDTH - INTERVAL_WIDTH, data['spark'], BAR_WIDTH, color='#aaaaaa', label='Spark', zorder=3)
# rects_blaze = plt.bar(data.index, data['blaze'], BAR_WIDTH, color='#019427', label='Blaze')
# rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH, data['blaze_opt'], BAR_WIDTH, color='#016A1C', label='Blaze OPT')
rects_blaze = plt.bar(data.index, data['blaze'], BAR_WIDTH, color='#0071b5', label='Blaze', zorder=0)
rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH, data['blaze_opt'], BAR_WIDTH, color='#2091d5', label='Blaze Opt.', zorder=3)
# rects_blaze = plt.bar(data.index, data['blaze'], BAR_WIDTH, color='#fb8c00', label='Blaze')
# rects_blaze_opt = plt.bar(data.index + BAR_WIDTH + INTERVAL_WIDTH, data['blaze_opt'], BAR_WIDTH, color='#ef6c00', label='Blaze OPT')

plt.legend()
plt.xticks(data.index, ('2', '4', '8', '16'))
# plt.xticks(data.index - (BAR_WIDTH + INTERVAL_WIDTH) / 2, ('2', '4', '8', '16'))
plt.gca().yaxis.grid(zorder=2, linestyle='dotted')
plt.title('Wordcount Throughput (More is Better)')
plt.xlabel('Number of nodes')
plt.ylabel('Words per second (in millions)')
plt.savefig(exp + '_speed.eps', format='eps', dpi=300)
plt.show()
