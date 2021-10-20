#!/usr/bin/env python3

import sys
import time
import datetime
import argparse

# Print the system temperature periodically

def main(interval):
    if interval is not None:
        interval = int(interval)

    while True:
        CurrentTime = datetime.datetime.now()

        with open(r"/sys/class/thermal/thermal_zone0/temp") as f:
            CurrentTemp = f.readline()

        print(f"{CurrentTime.strftime('%H:%M:%S')}\t{float(CurrentTemp) / 1000} ℃")

        if interval is None:
            return 0
        time.sleep(interval)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='temp', 
        description="Print system temperature periodically.")
    parser.add_argument('-t', help='Time interval in seconds. Default is None.')
    args = parser.parse_args()
    sys.exit(main(args.t))
