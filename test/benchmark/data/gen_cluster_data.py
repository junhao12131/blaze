import math
import random
import argparse



def main():
    # N_POINTS = 50
    N_POINTS = 2000000
    # N_POINTS = 5000000
    N_CENTERS = 5
    CENTERS = []
    for i in range(N_CENTERS):
        CENTERS.append([i, i, i])

    # output = open('kmeans_data_small.txt', 'w')
    # output = open('kmeans_data_small.txt', 'w')
    output = open('cluster_data.txt', 'w')
    ic = 0
    for i in range(N_POINTS):
        if ic == N_CENTERS:
            ic = 0
        for j in range(3):
            xj = random.gauss(CENTERS[ic][j], 0.2)
            # xj = random.gauss(CENTERS[ic][j], 0.6)
            output.write('%.2f ' % xj)
        output.write('\n')
        ic += 1


main()
