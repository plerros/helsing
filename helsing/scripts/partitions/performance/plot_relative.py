#!/bin/python3

"""
SPDX-License-Identifier: BSD-3-Clause
Copyright (c) 2025-2026 Pierro Zachareas
"""

import csv
import sys
import numpy as np
from scipy.interpolate import UnivariateSpline
from matplotlib import pyplot as plt
from matplotlib import gridspec as gridspec

# read the values from csv to list
filename = open(sys.argv[1], 'r')
file = csv.DictReader(filename, delimiter='\t')
outimage = sys.argv[1].removesuffix('.csv') + '.png'

class column:
	cast = object()
	data = []
	empty = True

	def __init__(self, cast):
		self.cast = cast
		self.data = []
		self.empty = True

	def append(self, value):
		self.data.append(self.cast(value))
		self.empty = False

	def setAll(self, array1D):
		self.data = array1D.copy()
		self.empty = (len(array1D) > 0)

	def get(self):
		return self.data

class plotData:
	columns = dict()

	def __init__(self):
		self.columns = dict([
			('base',             column(np.int_)),
			('method',           column(np.int_)),
			('multiplicand',     column(np.int_)),
			('product',          column(np.int_)),
			('mean1',            column(np.float64)),
			('stddev1',          column(np.float64)),
			('mean2',            column(np.float64)),
			('stddev2',          column(np.float64)),
			('mean_relative1',   column(np.float64)),
			('stddev_relative1', column(np.float64)),
			('mean_relative2',   column(np.float64)),
			('stddev_relative2', column(np.float64))
		])

	def append(self, row):
		for name, column in self.columns.items():
			if name in row:
				self.columns[name].append(row[name])
			else:
				self.columns[name].append(1) # placeholder

	def setAll(self, array2D):
		i = 0
		for name, column in self.columns.items():
			self.columns[name].setAll(array2D[i])
			i = i + 1

	def get(self, name):
		return self.columns[name].get()

	def export2D(self):
		ret = []
		for name, column in self.columns.items():
			ret.append(column.get())

		return ret

	def order(self):
		mean_ratio = np.divide(self.get('mean1'), self.get('mean2'))
		mean_ratio = np.subtract(mean_ratio, 1) # center on 0 so that results will have opposite sign
		mean_ratio = np.divide(1, mean_ratio) # move big values closer to center and small ones further away
		data2D = [mean_ratio] + self.export2D()
		data2D = list((list(t) for t in zip(*sorted(zip(*data2D)))))
		self.setAll(data2D[1:])

	def keep_base(self, base):
		# find the index of the items that we don't want to keep
		to_pop = []
		for idx, value in enumerate(self.get('base')):
			if value != base:
				to_pop.append(idx)

		# pop by descending order so that the index doesn't change
		to_pop.sort()
		to_pop = reversed(to_pop)
		for i in to_pop:
			for name, column in self.columns.items():
				self.columns[name].data.pop(i)

	def calc_relative(self):
		# variance and covariance
		# we assume they are independent; no need for covariance!
		var1 = np.multiply(self.get('stddev1'), self.get('stddev1'))
		var2 = np.multiply(self.get('stddev2'), self.get('stddev2'))

		# Calculate relative performance uplift
		self.columns['mean_relative1'].setAll([])
		self.columns['mean_relative2'].setAll([])
		self.columns['stddev_relative1'].setAll([])
		self.columns['stddev_relative2'].setAll([])

		for i in range(len(self.get('mean1'))):
			tmp = self.get('mean1')[i] / self.get('mean2')[i] - 1.0
			if (tmp > 0) and (not np.isinf(tmp)):
				self.columns['mean_relative1'].append(tmp)
				self.columns['stddev_relative1'].append(np.sqrt((var1[i] + var2[i]) / self.get('mean2')[i]))
			else:
				self.columns['mean_relative1'].append(0)
				self.columns['stddev_relative1'].append(0)

		for i in range(len(self.get('mean2'))):
			tmp = self.get('mean2')[i] / self.get('mean1')[i] - 1.0
			if (tmp > 0) and (not np.isinf(tmp)):
				self.columns['mean_relative2'].append(-tmp)
				self.columns['stddev_relative2'].append(np.sqrt((var1[i] + var2[i]) / self.get('mean1')[i]))
			else:
				self.columns['mean_relative2'].append(0)
				self.columns['stddev_relative2'].append(0)

	def length(self):
		return len(self.columns['base'].get())

dataAll = plotData()
bases = set()

for row in file:
	dataAll.append(row)
	bases.add(np.int_(row['base']))

subplot_count = len(bases)
dataAll.order()
dataAll.calc_relative()

# Plot data
width = 0.4

fig = plt.figure(figsize=(dataAll.length() / 6, 2 * 7))
gs_outer = gridspec.GridSpec(2, 1, figure=fig)

gs1 = gridspec.GridSpecFromSubplotSpec(1, 1, subplot_spec=gs_outer[0,0])
ax1 = fig.add_subplot(gs1[0,0])
gs2 = gridspec.GridSpecFromSubplotSpec(1, subplot_count, subplot_spec=gs_outer[1,0])
ax2 = []
for idx in range(0, subplot_count):
	ax2.append(fig.add_subplot(gs2[0,idx]))

def plot_axis(ax, data):
	p1 = ax.bar(range(data.length()), data.get('mean_relative1'), width, yerr=data.get('stddev_relative1'), label='#1 percent faster')
	p2 = ax.bar(range(data.length()), data.get('mean_relative2'), width, yerr=data.get('stddev_relative2'), label='#2 percent faster')

	# Add percentile uplift to the bars
	mean_rel_str1 = []
	for i in data.get('mean_relative1'):
		tmp = int(abs(i) * 100)
		if tmp > 0:
			mean_rel_str1.append(str(tmp).join(' %'))
		else:
			mean_rel_str1.append(" ")

	mean_rel_str2 = []
	for i in data.get('mean_relative2'):
		tmp = int(abs(i) * 100)
		if tmp > 0:
			mean_rel_str2.append(str(tmp).join(' %'))
		else:
			mean_rel_str2.append(" ")

	ax.bar_label(p1, labels=mean_rel_str1, color='black', label_type='edge', rotation='vertical', padding=3)
	ax.bar_label(p2, labels=mean_rel_str2, color='black', label_type='edge', rotation='vertical', padding=3)

	# Add base, method, mulitplicand and product to bars
	labelpos = [] # invisible bar that helps with label positioning
	labeltext1 = []
	labeltext2 = []

	for base, method, mult, prod, mean1, mean2 in zip(data.get('base'), data.get('method'), data.get('multiplicand'), data.get('product'), data.get('mean1'), data.get('mean2')):
		if (mean1 > mean2):
			labeltext1.append(' '.join((str(base), str(method), str(mult), str(prod))))
			labeltext2.append("")
			labelpos.append(-0.001)
		else:
			labeltext1.append("")
			labeltext2.append(' '.join((str(base), str(method), str(mult), str(prod))))
			labelpos.append(0.001)

	p3 = ax.bar(range(data.length()), labelpos, width, visible=False)

	ax.bar_label(p3, labels=labeltext1, color='lightgrey', label_type='edge', rotation='vertical', padding=5)
	ax.bar_label(p3, labels=labeltext2, color='lightgrey', label_type='edge', rotation='vertical', padding=5)

	ax.set(yticklabels=[])  # remove the tick labels
	ax.tick_params(left=False)  # remove the ticks
	ax.axis('off')

plot_axis(ax1, dataAll)

for idx, base in enumerate(sorted(bases)):
	data = plotData()
	data.setAll(dataAll.export2D())
	data.keep_base(base)
	plot_axis(ax2[idx], data)

plt.tight_layout()

plt.savefig(outimage)
#plt.show()
