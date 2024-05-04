import random
import argparse

def write_num_to_file(output_file, page_num_list, iter: int):
    for i in range(iter):
        with open(output_file, 'ab') as file:
            for i in page_num_list:
                file.write(i.to_bytes(8, byteorder='little'))

def generatenum(seq: bool, num:int, offset: int):
    mylist = list(range(offset, num + offset))
    print("max num is", num + offset - 1)
    if(not seq):
        random.shuffle(mylist)
    return mylist

parser = argparse.ArgumentParser(description='Process arguements.')
parser.add_argument('-f', '--file', default="out.bin")
# size in page number
parser.add_argument('-s', '--size',  type=int, default=0)
# 0 for sequential, 1 for random
parser.add_argument('-p', '--pattern',  type=int, default=0)
parser.add_argument('-o', '--offset',  type=int, default=0)
# repeat time
parser.add_argument('-i', '--iter',  type=int, default=1)
args = parser.parse_args()

if args.pattern == 0:
    seq = True
else:
    seq = False

write_num_to_file(args.file, generatenum(seq, args.size, args.offset), args.iter)