#!/usr/bin/env python3

# Convert OSP 32-bit float stereo recording dumps to WAV files.

import sys
import argparse
import struct

def float32_wav_header(byte_count, channels, sample_rate):
    wav_file = struct.pack('<ccccIccccccccIHHIIHH',
        b'R', b'I', b'F', b'F',
        byte_count + 0x2c - 8,  # header size
        b'W', b'A', b'V', b'E', b'f', b'm', b't', b' ',
        0x10,  # size of 'fmt ' header
        3,  # format 3 = floating-point PCM
        channels,  
        sample_rate,  # samples / second
        sample_rate * 4,  # bytes / second
        4,  # block alignment
        32)  # bits / sample
    wav_file += struct.pack('<ccccI', b'd', b'a', b't', b'a', byte_count)
    return wav_file

def convert(filename):
    with open(filename, mode='rb') as file:
        data = file.read()
        num_bytes = len(data)
        print(f"Read {num_bytes} bytes from {filename}")
        if num_bytes % (96*4):
            print("ERROR: length of data is not correct")
            sys.exit(1)

        header = float32_wav_header(num_bytes, 2, 48000)

        out_filename = filename + ".wav"
        print(f"Writing {out_filename}")
        with open(out_filename, 'wb') as out:
            out.write(header)
            out.write(data) 


def main(files):
    for f in files:
        convert(f)
    return 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='osp_convert', 
        description="Convert recording dumps to WAV files")
    parser.add_argument('files', metavar='filename', type=str, nargs='+',
                    help='The file(s) to be converted')
    args = parser.parse_args()
    sys.exit(main(args.files))
