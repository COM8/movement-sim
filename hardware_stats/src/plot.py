from matplotlib.axes import Axes
from typing import List, Tuple, Union
from datetime import datetime
import csv
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.animation import FuncAnimation

# plt.style.use('fivethirtyeight')

def parse_results(filePath: str) -> List[Tuple[datetime, float, float, float, float, float, float, float, float]]:
    print("Parsing data...")
    results: List[Tuple[str, float, float, float, float, float, float, float, float, float, float]] = list()
    try:
        with open(filePath) as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=';')
            next(csv_reader, None)
            for row in csv_reader:
                if len(row) == 9:
                    try:
                        results.append((datetime.strptime(row[0], "%H:%M:%S.%f"), float(row[1]), float(row[2]), float(row[3]), float(row[4]), float(row[5]), float(row[6]), float(row[7]), float(row[8])))
                        # print(results[-1][0])
                    except Exception as e:
                        print("Parsing failed: " + str(e))
    except Exception as e:
        print("Opening failed: " + str(e))
    return results

def plot_all_cpus(ax: Axes, dataBetter: List[List[Union[datetime, float]]]):
    ax.cla()
    ax.set_xticks(ax.get_xticks(), ax.get_xticklabels(), rotation = -90)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    ax.xaxis.set_major_locator(mdates.SecondLocator(interval=2))
    ax.set_xlim([min(dataBetter[0]), max(dataBetter[0])])
    ax.set_ylim([0, 200])

    ax.plot(dataBetter[0], dataBetter[1], linestyle="dotted", linewidth=1.0, label=f'CPU0 Package (max: {round(max(dataBetter[1]), 2)})')
    ax.plot(dataBetter[0], dataBetter[2], linestyle="dotted", linewidth=1.0, label=f'CPU0 Cores (max: {round(max(dataBetter[2]), 2)})')
    ax.plot(dataBetter[0], dataBetter[3], linewidth=1.0, label=f'CPU0 (max: {round(max(dataBetter[3]), 2)})')

    ax.plot(dataBetter[0], dataBetter[5], linestyle="dotted", linewidth=1.0, label=f'CPU1 Package (max: {round(max(dataBetter[4]), 2)})')
    ax.plot(dataBetter[0], dataBetter[6], linestyle="dotted", linewidth=1.0, label=f'CPU1 Cores (max: {round(max(dataBetter[5]), 2)})')
    ax.plot(dataBetter[0], dataBetter[7], linewidth=1.0, label=f'CPU1 (max: {round(max(dataBetter[6]), 2)})')
    ax.legend(loc='upper left')

    secondAx: Axes = ax.twinx()
    secondAx.cla()
    secondAx.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    secondAx.xaxis.set_major_locator(mdates.SecondLocator(interval=2))
    secondAx.set_ylim([0, 100])
    secondAx.set_xlim([min(dataBetter[0]), max(dataBetter[0])])
    secondAx.plot(dataBetter[0], dataBetter[4], linewidth=1.0, label=f'CPU0 Utilization (max: {round(max(dataBetter[4]), 2)}%)')
    secondAx.plot(dataBetter[0], dataBetter[8], linewidth=1.0, label=f'CPU1 Utilization (max: {round(max(dataBetter[4]), 2)}%)')
    secondAx.set_ylabel("Utilization")
    secondAx.legend(loc='upper right')

def plot_cpus(ax: Axes, dataBetter: List[List[Union[datetime, float]]]):
    ax.cla()
    ax.clear()
    ax.set_xticks(ax.get_xticks(), ax.get_xticklabels(), rotation = -90)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    ax.xaxis.set_major_locator(mdates.SecondLocator(interval=2))

    packageWatt: List[float] = [sum(value) for value in zip(dataBetter[1], dataBetter[5])]
    coreWatt: List[float] = [sum(value) for value in zip(dataBetter[2], dataBetter[6])]
    sumWatt: List[float] = [sum(value) for value in zip(dataBetter[3], dataBetter[7])]
    cpuUtilization: List[float] = [sum(value) / 2 for value in zip(dataBetter[4], dataBetter[8])]

    ax.plot(dataBetter[0], packageWatt, linestyle="dotted", linewidth=1.0, label=f'Package (max: {round(max(packageWatt), 2)})')
    ax.plot(dataBetter[0], coreWatt, linestyle="dotted", linewidth=1.0, label=f'Cores (max: {round(max(coreWatt), 2)})')
    ax.plot(dataBetter[0], sumWatt, linewidth=1.0, label=f'CPU (max: {round(max(sumWatt), 2)})')
    ax.set_xlim([min(dataBetter[0]), max(dataBetter[0])])
    ax.set_ylim([0, 400])
    ax.set_ylabel("Watt")
    ax.set_xlabel("Time")
    ax.legend(loc='upper left')

    secondAx: Axes = ax.twinx()
    secondAx.cla()
    secondAx.clear()
    secondAx.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    secondAx.xaxis.set_major_locator(mdates.SecondLocator(interval=2))
    secondAx.set_ylim([0, 100])
    secondAx.set_xlim([min(dataBetter[0]), max(dataBetter[0])])
    secondAx.plot(dataBetter[0], cpuUtilization, linewidth=1.0, label=f'Utilization (max: {round(max(cpuUtilization), 2)}%)')
    secondAx.set_ylabel("Utilization")
    secondAx.legend(loc='upper right')

def animate(i, ax):
    lastN: int = -250
    data = parse_results("build/src/data.txt")[lastN:]
    if len(data) <= 0:
        return
    dataBetter: List[List[Union[datetime, float]]] = list(zip(*data))

    plot_all_cpus(ax[0], dataBetter)
    plot_cpus(ax[1], dataBetter)


fig, ax = plt.subplots(2, 1, constrained_layout=True)
fig.suptitle("CPU performance statistics", fontsize=16)
ani = FuncAnimation(fig, animate, interval=250, fargs=(ax,))
plt.show()