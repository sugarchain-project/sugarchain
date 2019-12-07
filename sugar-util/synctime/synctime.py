import re
from datetime import datetime
from datetime import timedelta

logFilePath = "debug.log"
outputFilePath = "output.csv"

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
                csvFile.write("{0},{1},{2}\n".format(date, td, height))
                # output every 10 minutes
                _td = _td + timedelta(minutes=10)
