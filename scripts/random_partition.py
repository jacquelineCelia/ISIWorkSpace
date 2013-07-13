#!/usr/bin/python
import os
import sys
import string
import random
from optparse import OptionParser

def LoadFile(infile):
   utts = list()
   fin = open(infile)
   line = fin.readline()
   while line:
      utts.append(line)
      line = fin.readline()
   fin.close()
   return utts

def main():
   parser = OptionParser()
   parser.add_option("--train", dest = "train", help = "number of training data", type = int)
   parser.add_option("--test", dest = "test", help = "number of test data", type = int)
   parser.add_option("--dev", dest = "dev", help = "number of dev data", type = int)
   parser.add_option("--prefix", dest = "prefix", help = "prefix of the output data. output will be prefix.train and prefix.test and prefix.dev")
   parser.add_option("--input", dest = "infile", help = "input file")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")
   
   utts = LoadFile(options.infile)
   if options.train + options.test + options.dev > len(utts):
      print "sum of the training num and the test num is larger than the provided data!"
   else:
      random.shuffle(utts)
      ftrain = open(options.prefix + '.train', 'w')
      ftest = open(options.prefix + '.test', 'w')
      fdev = open(options.prefix + '.dev', 'w')

      offset = 0
      for i in range(options.train):
         ftrain.write(utts[i])
      offset = options.train
      for i in range(offset, offset + options.test):
         ftest.write(utts[i])
      offset += options.test
      for i in range(offset, offset + options.dev):
         fdev.write(utts[i])

      ftrain.close()
      ftest.close()
      fdev.close()

if __name__ == "__main__":
   main()
