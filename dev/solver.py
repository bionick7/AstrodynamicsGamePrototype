from math import *

import numpy as np
import matplotlib.pyplot as plt
import scipy.optimize

def f1(x, K):
    alpha = 2*asin(x)
    beta = 2*asin(K*x)
    return x**-3 * sqrt(alpha - sin(alpha) - beta + sin(beta))

def f2(x, K):
    alpha = 2*asin(x)
    beta = 2*asin(K*x)
    return x**-3 * sqrt(alpha - sin(alpha) + beta - sin(beta))

N = 1024
KK = np.linspace(1e-8, 1 - 1e-8, N)
yy = np.zeros((2, N))

outp1 = f"double simplified_lambert_minimum_tabulation_case_0 [SIMPLIFIED_LAMBERT_MINIMUM_TABULATION_SIZE] = " "{"
outp2 = f"double simplified_lambert_minimum_tabulation_case_2 [SIMPLIFIED_LAMBERT_MINIMUM_TABULATION_SIZE] = " "{"

for i in range(N):
    yy[0, i] = scipy.optimize.minimize_scalar(lambda x: f1(x, KK[i]), bounds=(0.0, 1.0)).x
    yy[1, i] = scipy.optimize.minimize_scalar(lambda x: f2(x, KK[i]), bounds=(0.0, 1.0)).x
    outp1 += f"{yy[0, i]:8.7f}, "
    outp2 += f"{yy[1, i]:8.7f}, "

outp1 = outp1[:-2] + "};"
outp2 = outp2[:-2] + "};"

print(f"#define SIMPLIFIED_LAMBERT_MINIMUM_TABULATION_SIZE {N}")
print(outp1)
print(outp2)

plt.plot(KK, yy[0], label="f1")
plt.plot(KK, yy[1], label="f2")
plt.legend()
plt.show()