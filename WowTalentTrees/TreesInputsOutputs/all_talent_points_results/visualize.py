# -*- coding: utf-8 -*-
import sys
import os

import numpy as np
import matplotlib.pyplot as plt

def main():
    lines = read_file()
    talent_points = list(range(1, 43))
    times = extract_times(lines)
    comb_wo_switch, comb_w_switch = extract_combinations(lines)
    visualize(talent_points, times, comb_wo_switch, comb_w_switch)

def read_file():
    lines = []
    with open("./all_talent_points_results.txt", "r") as f:
        for line in f:
            lines.append(line)

    return lines

def extract_times(lines):
    times = []
    for line in lines[1::2]:
        times.append(float(line.split(" ")[3]))

    return np.asarray(times) / 1000.0

def extract_combinations(lines):
    comb_wo_switch = []
    comb_w_switch = []
    for line in lines[::2]:
        comb_wo_switch.append(int(line.split(" ")[-5]))
        comb_w_switch.append(int(line.strip().split(" ")[-1]))

    return np.asarray(comb_wo_switch), np.asarray(comb_w_switch)

def visualize(tal, t, cwos, cws):
    fig, ax = plt.subplots(2, 3, figsize=(16, 8))
    ax[0][0].plot(tal, t)
    ax[0][0].set_xlabel("# talent points")
    ax[0][0].set_ylabel("time [s]")
    ax[0][0].set_title(f"total time spent: {t.sum()} s")
    ax[0][1].plot(tal, cwos / 1000)
    ax[0][1].set_xlabel("# talent points")
    ax[0][1].set_ylabel("combinations [x1000]")
    ax[0][1].set_title("combinations w/o switch talents")
    ax[0][2].plot(tal, cws / 1000)
    ax[0][2].set_xlabel("# talent points")
    ax[0][2].set_ylabel("combinations [x1000]")
    ax[0][2].set_title("combinations w/ switch talents")
    ax[1][0].plot(tal, t / tal)
    ax[1][0].set_xlabel("# talent points")
    ax[1][0].set_ylabel("time [s]")
    ax[1][0].set_title("time spent per talent point")
    ax[1][1].plot(tal, cwos / t / 1000)
    ax[1][1].set_xlabel("# talent points")
    ax[1][1].set_ylabel("combinations [x1000]")
    ax[1][1].set_title("combinations per second w/o switch talents")
    ax[1][2].plot(tal, cws / t / 1000)
    ax[1][2].set_xlabel("# talent points")
    ax[1][2].set_ylabel("combinations [x1000]")
    ax[1][2].set_title("combinations per second w/ switch talents")
    fig.tight_layout()
    #plt.show()
    fig.savefig("all_talent_points_results.pdf")
    fig.savefig("all_talent_points_results.png")
        
if __name__ == "__main__":
    sys.exit(int(main() or 0))
