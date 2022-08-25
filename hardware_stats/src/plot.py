from matplotlib.axes import Axes
from typing import List, Tuple, Union, Dict
from datetime import datetime
import csv
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.animation import FuncAnimation

# nvidia-smi --query-gpu=timestamp,name,utilization.gpu,power.draw --format=csv -lms 100 -f dataGpu.txt

def parse_cpu_results(filePath: str) -> List[Tuple[datetime, float, float, float, float, float, float, float, float]]:
    print("Parsing CPU data...")
    results: List[Tuple[datetime, float, float, float, float, float, float, float, float]] = list()
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

def parse_gpu_results(filePath: str) -> List[Tuple[datetime, float, float, float, float, float, float, float, float]]:
    print("Parsing GPU data...")
    resultsDict: Dict[str, List[Tuple[datetime, float, float]]] = dict()
    try:
        with open(filePath) as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=',')
            next(csv_reader, None)
            for row in csv_reader:
                if len(row) == 4:
                    try:
                        tp: datetime = datetime.strptime(row[0], "%Y/%m/%d %H:%M:%S.%f")
                        name: str = row[1]
                        if not name in resultsDict:
                            resultsDict[name] = list()
                        resultsDict[name].append((tp, float(row[2].replace(" %", "")), float(row[3].replace(" W", ""))))
                    except Exception as e:
                        print("Parsing failed: " + str(e))
    except Exception as e:
        print("Opening failed: " + str(e))
    return list(resultsDict.values())

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

def plot_all_gpus(ax: Axes, dataBetter: List[List[Union[datetime, float]]]):
    ax.cla()
    ax.set_xticks(ax.get_xticks(), ax.get_xticklabels(), rotation = -90)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    ax.xaxis.set_major_locator(mdates.SecondLocator(interval=2))
    ax.set_xlim([min(dataBetter[0][0]), max(dataBetter[0][0])])
    ax.set_ylim([0, 200])

    ax.plot(dataBetter[0][0], dataBetter[0][2], linewidth=1.0, label=f'GPU0 (max: {round(max(dataBetter[0][2]), 2)})')
    ax.plot(dataBetter[1][0], dataBetter[1][2], linewidth=1.0, label=f'GPU1 (max: {round(max(dataBetter[1][2]), 2)})')
    ax.legend(loc='upper left')

    secondAx: Axes = ax.twinx()
    secondAx.cla()
    secondAx.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    secondAx.xaxis.set_major_locator(mdates.SecondLocator(interval=2))
    secondAx.set_ylim([0, 100])
    secondAx.set_xlim([min(dataBetter[0][0]), max(dataBetter[0][0])])
    secondAx.plot(dataBetter[0][0], dataBetter[0][1], linewidth=1.0, label=f'GPU0 Utilization (max: {round(max(dataBetter[0][1]), 2)}%)')
    secondAx.plot(dataBetter[1][0], dataBetter[0][1], linewidth=1.0, label=f'GPU1 Utilization (max: {round(max(dataBetter[1][1]), 2)}%)')
    secondAx.set_ylabel("Utilization")
    secondAx.legend(loc='upper right')

def plot_gpus(ax: Axes, dataBetter: List[List[Union[datetime, float]]]):
    ax.cla()
    ax.clear()
    ax.set_xticks(ax.get_xticks(), ax.get_xticklabels(), rotation = -90)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    ax.xaxis.set_major_locator(mdates.SecondLocator(interval=2))

    sumWatt: List[float] = [sum(value) for value in zip(dataBetter[0][2], dataBetter[1][2])]
    gpuUtilization: List[float] = [sum(value) / 2 for value in zip(dataBetter[0][1], dataBetter[1][1])]

    ax.plot(dataBetter[0][0], sumWatt, linewidth=1.0, label=f'GPU (max: {round(max(sumWatt), 2)})')
    ax.set_xlim([min(dataBetter[0][0]), max(dataBetter[0][0])])
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
    secondAx.set_xlim([min(dataBetter[0][0]), max(dataBetter[0][0])])
    secondAx.plot(dataBetter[0][0], gpuUtilization, linewidth=1.0, label=f'Utilization (max: {round(max(gpuUtilization), 2)}%)')
    secondAx.set_ylabel("Utilization")
    secondAx.legend(loc='upper right')

def animate(i, ax):
    lastN: int = -250
    dataCpu = parse_cpu_results("build/src/dataCpu.txt")[lastN:]
    if len(dataCpu) > 0:
        dataCpuBetter: List[List[Union[datetime, float]]] = list(zip(*dataCpu))
        plot_all_cpus(ax[0][0], dataCpuBetter)
        plot_cpus(ax[0][1], dataCpuBetter)

    dataGpu = parse_gpu_results("build/src/dataGpu.txt")
    if len(dataGpu) == 2 and len(dataGpu[0]) > 0 and len(dataGpu[0]) == len(dataGpu[1]):
        dataGpuBetter: Tuple[List[List[Union[datetime, float]]], List[List[Union[datetime, float]]]] = (list(zip(*(dataGpu[0][lastN:]))), list(zip(*(dataGpu[1][lastN:]))))
        plot_all_gpus(ax[1][0], dataGpuBetter)
        plot_gpus(ax[1][1], dataGpuBetter)


fig, ax = plt.subplots(2, 2, constrained_layout=True)
fig.suptitle("CPU performance statistics", fontsize=16)
ani = FuncAnimation(fig, animate, interval=250, fargs=(ax,))
plt.show()