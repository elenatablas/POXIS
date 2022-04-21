import random
import argparse
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("numbytes", help="número de bytes aleatorios generados", type=int)
args = parser.parse_args()

random.seed(42)
for i in range(args.numbytes):
    a = random.randint(0, 255)
    os.write(1, a.to_bytes(1,sys.byteorder))
 
