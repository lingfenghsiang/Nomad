import sys
import argparse
def generate_hosttest(output_file:str, offset: int):
    mapping = {}
    order = []

    for line in sys.stdin:
        num = int(line)
        order.append(num)
        if not num in mapping:
            mapping[num] =[0, 0]
        else:
            mapping[num][0] += 1

    counter = len(mapping) - 1 + offset
    # the offset of the next working set page index
    print(counter + 1)
    

    # hottest
    for k in sorted(mapping.items(), key= lambda a : a[1][0]):
        k[1][1] = counter
        counter -= 1
    with open(output_file, 'wb') as file:
        for i in order:
            file.write((mapping[i][1]).to_bytes(8, byteorder='little'))

def generate_first_touch(output_file:str, offset: int):
    mapping = {}
    order = []

    counter = 0 + offset
    for line in sys.stdin:
        num = int(line)
        num = str(num)
        order.append(num)
        if not num in mapping:
            mapping[num] = counter
            counter += 1
    # the offset of the next working set page index
    print(counter)
       
    with open(output_file, 'wb') as file:
        for i in order:
            file.write((mapping[i]).to_bytes(8, byteorder='little'))

parser = argparse.ArgumentParser(description='Process arguements.')
parser.add_argument('-f', '--file', default="out.bin")
parser.add_argument('-o', '--offset', type=int, default=0)
parser.add_argument('-p', '--pattern', type=int, default=0)
args = parser.parse_args()
if(args.pattern == 0):
    generate_hosttest(args.file, args.offset)
elif(args.pattern == 1):
    generate_first_touch(args.file, args.offset)