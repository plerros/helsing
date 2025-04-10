#!/bin/python3

"""
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025 Pierro Zachareas
"""

import csv
import sys
import numpy as np
from scipy.interpolate import UnivariateSpline
from matplotlib import pyplot as plt

# Read the values from csv to list
filename = open(sys.argv[1], 'r')
file = csv.DictReader(filename, delimiter='\t')
outimage = sys.argv[1].removesuffix('.csv') + '.png'

csv_base = []
csv_method = []
csv_multiplicand = []
csv_product = []
csv_mean1 = []
csv_stddev1 = []
csv_mean2 = []
csv_stddev2 = []
#csv_distance = []

for col in file:
	csv_base.append(col['base'])
	csv_method.append(col['method'])
	csv_multiplicand.append(col['multiplicand'])
	csv_product.append(col['product'])
	csv_mean1.append(col['mean1'])
	csv_stddev1.append(col['stddev1'])
	csv_mean2.append(col['mean2'])
	csv_stddev2.append(col['stddev2'])
	#csv_distance.append(col['distance'])

csv_base = list(np.int_(csv_base))
csv_method = list(np.int_(csv_method))
csv_multiplicand = list(np.int_(csv_multiplicand))
csv_product = list(np.int_(csv_product))
csv_mean1 = list(np.float_(csv_mean1))
csv_stddev1 = list(np.float_(csv_stddev1))
csv_mean2 = list(np.float_(csv_mean2))
csv_stddev2 = list(np.float_(csv_stddev2))
#csv_distance = list(np.float_(csv_distance))

csv_lines = len(csv_base)

# Re-arrange data in preferred way
mean_ratio = np.divide(csv_mean1, csv_mean2)
mean_ratio = np.subtract(mean_ratio, 1) #center on 0 so that results will have opposite sign
mean_ratio = np.divide(1, mean_ratio) #move big values closer to center and small ones further away
#mean_ratio = np.fmax(np.divide(csv_mean1, csv_mean2),  np.divide(csv_mean2, csv_mean1))
mean_ratio, csv_base, csv_method, csv_multiplicand, csv_product, csv_mean1, csv_stddev1, csv_mean2, csv_stddev2 = (list(t) for t in zip(*sorted(zip(mean_ratio, csv_base, csv_method, csv_multiplicand, csv_product, csv_mean1, csv_stddev1, csv_mean2, csv_stddev2))))

# variance and covariance
# we assume they are independent; no need for covariance!
var1 = np.multiply(csv_stddev1, csv_stddev1)
var2 = np.multiply(csv_stddev2, csv_stddev2)

# Calculate relative performance uplift
mean_relative1 = csv_mean1.copy()
mean_relative2 = csv_mean2.copy()
stddev_relative1 = csv_stddev1.copy()
stddev_relative2 = csv_stddev2.copy()

for i in range(len(csv_mean1)):
	tmp = csv_mean1[i] / csv_mean2[i] - 1.0
	if (tmp > 0) and (not np.isinf(tmp)):
		mean_relative1[i] = tmp
		stddev_relative1[i] = np.sqrt((var1[i] + var2[i]) / csv_mean2[i])
	else:
		mean_relative1[i] = 0
		stddev_relative1[i] = 0


for i in range(len(csv_mean2)):
	tmp = csv_mean2[i] / csv_mean1[i] - 1.0
	if (tmp > 0) and (not np.isinf(tmp)):
		mean_relative2[i] = -tmp
		stddev_relative2[i] = np.sqrt((var1[i] + var2[i]) / csv_mean1[i])
	else:
		mean_relative2[i] = 0
		stddev_relative2[i] = 0

# Plot data
width = 0.4
fig, ax = plt.subplots(figsize=(csv_lines / 6, 7))
p1 = ax.bar(range(csv_lines), mean_relative1, width, yerr=stddev_relative1, label='#1 percent faster')
p2 = ax.bar(range(csv_lines), mean_relative2, width, yerr=stddev_relative2, label='#2 percent faster')

# Add percentile uplift to the bars
mean_rel_str1 = []
for i in mean_relative1:
	tmp = int(abs(i) * 100)
	if tmp > 0:
		mean_rel_str1.append(str(tmp).join(' %'))
	else:
		mean_rel_str1.append(" ")

mean_rel_str2 = []
for i in mean_relative2:
	tmp = int(abs(i) * 100)
	if tmp > 0:
		mean_rel_str2.append(str(tmp).join(' %'))
	else:
		mean_rel_str2.append(" ")

plt.bar_label(p1, labels=mean_rel_str1, color='black', label_type='edge', rotation='vertical', padding=3)
plt.bar_label(p2, labels=mean_rel_str2, color='black', label_type='edge', rotation='vertical', padding=3)

# Add base, method, mulitplicand and product to bars
labelpos = [] # invisible bar that helps with label positioning
labeltext1 = []
labeltext2 = []
for base, method, mult, prod, mean1, std1, mean2, std2 in zip(csv_base, csv_method, csv_multiplicand, csv_product, csv_mean1, csv_stddev1, csv_mean2, csv_stddev2):
	if (mean1 > mean2):
		labeltext1.append(' '.join((str(base), str(method), str(mult), str(prod))))
		labeltext2.append("")
		labelpos.append(-0.001)
	else:
		labeltext1.append("")
		labeltext2.append(' '.join((str(base), str(method), str(mult), str(prod))))
		labelpos.append(0.001)

p3 = ax.bar(range(csv_lines), labelpos, width, visible=False)

plt.bar_label(p3, labels=labeltext1, color='lightgrey', label_type='edge', rotation='vertical', padding=5)
plt.bar_label(p3, labels=labeltext2, color='lightgrey', label_type='edge', rotation='vertical', padding=5)

ax.set(yticklabels=[])  # remove the tick labels
ax.tick_params(left=False)  # remove the ticks
ax.axis('off')

plt.tight_layout()

plt.savefig(outimage)
#plt.show()
