import os
import re
import numpy as np
import matplotlib.pyplot as plt


Algorithms = ["INTUITIVE", "INTERLEAVE", "PIPELINE", "OMP"]
dir = "./profile"
p = re.compile('gaussian_([A-Z]+)_nt([0-9]+)')
data = dict()
plt.rcParams['figure.constrained_layout.use'] = True
for root, dirs, files in os.walk(dir):
    for filename in files:
        m = p.match(filename)
        algo = m.groups()[0]
        num_thread = int(m.groups()[1])
        if algo not in data.keys():
            data[algo] = {}
        if num_thread not in data[algo].keys():
            data[algo][num_thread] = {}
        f = open(os.path.join(dir, filename))
        raw_data = f.read()
        # cycles = float(re.search(r'DPU cycles\s*=\s*([0-9\.e\+]*)', raw_data).groups()[0])
        dpu_time = float(re.search(r'time spent: ([0-9\.]*)', raw_data).groups()[0])
        data[algo][num_thread] = dpu_time

x_data = np.array([1, 2, 4, 8, 16, 32, 64, 128])
x_ticks = np.array([1, 2, 3, 4, 5, 6, 7, 8])
colors = ['#0089FF', '#FF7700', '#009900']
width = 0.2
# plt.title('普通高斯消元法的三种并行')
plt.ylabel('Time(ms)')
plt.xlabel('Number of thread')
plt.grid(linestyle='--', alpha=0.5)
plt.xticks(x_ticks, x_data)
for t in x_ticks:
    # for first in range(len(Algorithms)):
    plt.bar(t+width, data[Algorithms[3]][x_data[t-1]], width=width, color = colors[0])
# plt.legend(Algorithms)
# plt.bar(0, cpu_speed, width=0.5)
            
plt.savefig("partitioning hash join.jpg")


# x_data = np.arange(1, 25, dtype=int)
# xi = list(range(1, len(x_data), 2))
# yi = list(range(0, 50, 10))

# for i, tp in enumerate(['INT32', 'INT64', 'FLOAT', 'DOUBLE']):
#     plt.subplot(2,2,i+1)
#     plt.title('{} (1 DPU)'.format(tp), fontsize = 'x-small')
#     plt.ylabel('Arithmetic Throughput (MOPS)', fontsize = 'x-small')
#     plt.xlabel('#Tasklets', fontsize = 'x-small')
#     plt.grid(linestyle='--', alpha=0.5)
#     plt.xticks(xi)
#     plt.plot(x_data, data[tp]['ADD'][1:], 'ro-', label='ADD')
#     plt.plot(x_data, data[tp]['SUB'][1:], 'y^-', label='SUB')
#     plt.plot(x_data, data[tp]['MUL'][1:], 'bs-', label='MUL')
#     plt.plot(x_data, data[tp]['DIV'][1:], 'gx-', label='DIV')
#     plt.legend(loc = (0.7, 0.45), fontsize = 'x-small')
# plt.savefig("interleaved_throughput.pdf")
