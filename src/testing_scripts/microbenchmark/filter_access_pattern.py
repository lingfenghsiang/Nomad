import sys
import re

regex = r"user([0-9]*)"
for line in sys.stdin:
    tmp = re.findall(regex, line)
    if(tmp):
        print(tmp[1])

# print(re.findall(regex, "READ usertable user7697331399106995587 ")[1])