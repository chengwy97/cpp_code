import struct
import time
import os
import sys
import argparse

FIELDS = [
    ("timestamp", "q"),
    ("a", "q"),
    ("b", "d"),
    ("c", "d"),
    ("d", "d"),
    ("e", "d"),
    ("f", "d"),
    ("g", "d"),
    ("h", "d"),
    ("i", "d"),
    ("j", "d"),
]

struct_fmt = "<" + "".join([field[1] for field in FIELDS])
struct_unpack = struct.Struct(struct_fmt)
struct_size = struct_unpack.size

def analyze_bin(bin_path):
    with open(bin_path, 'rb') as f:

        while True:
            data = f.read(struct_size)
            if not data:
                time.sleep(0.1)
                continue
            yield struct_unpack.unpack(data)

if __name__ == '__main__':
    # 只有一个参数即 bin 的路径
    if len(sys.argv) != 2:
        print("Usage: python analyze_bin.py <bin_path>")
        sys.exit(1)
    bin_path = sys.argv[1]
    for data in analyze_bin(bin_path):
        timestamp, a, b, c, d, e, f, g, h, i, j = data
        ms = int(timestamp % 1_000_000 / 1_000)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(timestamp / 1_000_000)) + f".{ms:03d}"
        print(timestamp, a, b, c, d, e, f, g, h, i, j)
