import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

with open("E:/Games/astrodyn_concept_3/standalone/dev/encelladus_thetys_landscape.csv") as f:
    df = pd.read_csv(f)
fig, ax = plt.subplots(subplot_kw={'projection': '3d'})
ax.scatter(df['departure'], df['arrival'], df['dv'])
ax.set_zlim(0, 2000)
#plt.contour(df['departure'], df['arrival'], df['dv'])
plt.show()