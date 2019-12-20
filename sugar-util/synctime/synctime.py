#!/usr/bin/env python

import re
import sys
from datetime import datetime
from datetime import timedelta

# logFilePath = "debug.log"
# outputFilePath = "output.csv"
logFilePath = sys.argv[1]
outputFilePath = sys.argv[1] + ".csv"

with open(logFilePath, "r") as logFile, open(outputFilePath, "w") as csvFile:
    lines = logFile.readlines()
    csvFile.write("date,elapsed time,height\n")
    startDatetime = None
    _td = timedelta(0)
    for line in lines:
        match = re.match("^(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}).* height=(\\d*) .*", line)
        if match:
            date = match.group(1)
            dt = datetime.strptime(date, "%Y-%m-%d %H:%M:%S")
            if startDatetime is None:
                startDatetime = dt
            td = dt - startDatetime
            if td >= _td:
                height = match.group(2)
                
                # convert td in seconds: bad conversion, but works...
                td = str(td)
                td_in_seconds = sum(x * int(t) for x, t in zip([3600, 60, 1], td.split(":")))
                
                # write CSV
                csvFile.write("{0},{1},{2}\n".format(date, td_in_seconds, height))
                
                # output every 10 minutes
                _td = _td + timedelta(minutes=10)
