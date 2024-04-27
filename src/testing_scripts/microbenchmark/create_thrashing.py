import argparse
def create_thrashing(output_file, page_num):
    with open(output_file, 'wb') as file:
        for i in range(page_num):
            file.write(i.to_bytes(8, byteorder='little'))

parser = argparse.ArgumentParser(description='Process arguements.')
parser.add_argument('-f', '--file', default="out.bin")
# size in GB
parser.add_argument('-s', '--size',  type=int, default=0)
args = parser.parse_args()

create_thrashing(args.file, args.size * 1024 * 1024 * 1024 // 4096)
