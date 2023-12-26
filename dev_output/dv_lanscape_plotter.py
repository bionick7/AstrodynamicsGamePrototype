import numpy as np
#import matplotlib
#matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt

import os

def main():
    #print(os.getcwd())
    with open("dev_output/encelladus_thetys_landscape.csv", "r") as f:
        lines = f.readlines()[1:]

    departure_t = np.zeros(len(lines))
    arrival_t = np.zeros(len(lines))
    dvs = np.zeros(len(lines))

    for i, l in enumerate(lines):
        departure_t[i], arrival_t[i], dvs[i] = [float(x) for x in l[:-1].split(", ")]

    #departure_t = departure_t[dvs < 10000]
    #arrival_t = arrival_t[dvs < 10000]
    #dvs = dvs[dvs < 10000]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(departure_t, arrival_t, dvs)
    plt.show()

main()