#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys


def convert(from_file, to_file):
    with open(from_file) as in_f:
        lines = in_f.readlines()
        with open(to_file, 'w') as out_f:
            out_f.write('"')
            for line in lines:
                if line.strip():
                    out_line = line.replace('"', '\\"')
                    out_line = out_line.replace('\n', '\\\n')
                    out_f.write(out_line)
            out_f.write('\\r\\n\\r\\n";\n')


def main():
    if len(sys.argv) != 3:
        print("Usage: {} HTML_FILE OUT_FILE".format(sys.argv[0]))
        exit(-1)
    else:
        convert(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
