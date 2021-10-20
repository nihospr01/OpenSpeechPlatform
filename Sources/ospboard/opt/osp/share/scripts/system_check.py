#!/usr/bin/env python3

import sys
import time
import datetime
import os
import psutil


def main():
    
    CurrentTime = datetime.datetime.now()

    with open(r"/sys/class/thermal/thermal_zone0/temp") as f:
        CurrentTemp0 = f.readline()
    with open(r"/sys/class/thermal/thermal_zone1/temp") as f:
        CurrentTemp1 = f.readline()
    freq = []
    for i in range(4):
        with open(f"/sys/devices/system/cpu/cpu{i}/cpufreq/cpuinfo_cur_freq") as f:
            freq.append(f.readline())

    with open(r"/sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state") as f:
        time_in_state = f.read()

    print(f"\n{CurrentTime.strftime('%H:%M:%S')}\t CPU0-1: {float(CurrentTemp0) / 1000} ℃\t\tCPU2-3: {float(CurrentTemp1) / 1000} ℃")


    cpu = psutil.cpu_times_percent(percpu=True)
    time.sleep(1)
    cpu = psutil.cpu_times_percent(percpu=True)

    print(f"\nCPU busy (%)   (1-4) : {100-cpu[0].idle:.2f} {100-cpu[1].idle:.2f} {100-cpu[2].idle:.2f} {100-cpu[3].idle:.2f}")
    print(f"\nCPU freq (kHz) (1-4) : {int(freq[0])/1000}  {int(freq[1])/1000}  {int(freq[2])/1000}  {int(freq[3])/1000}")

    print("\nTIME IN STATE\n-------------\nkHz  Percent\n-------------")
    total = 0
    for t in time_in_state.split('\n'):
        if t:
            freq, per = t.split()
            total += int(per)

    for t in time_in_state.split('\n'):
        if t:
            freq, per = t.split()
            freq = int(int(freq)/1000)
            per = int(int(per) / total * 100)
            print(f"{freq}   {per}")

    print("\nOSP Status")
    os.system('ps -T -p `pgrep OSP` -o cpuid,cls,pri,pcpu,lwp,comm')

    diskfree = psutil.disk_usage('/').percent
    print(f"\nDiskfree: {diskfree}%")

    print("\nCharge Log\n----------")
    with open(r"/var/log/charge.log") as f:
        print(f.read())

if __name__ == '__main__':
    sys.exit(main())
