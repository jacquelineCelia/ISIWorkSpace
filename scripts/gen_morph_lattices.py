#!/usr/bin/python
import os
import sys
import string
import re
import tempfile, subprocess, hashlib, codecs
from optparse import OptionParser

__MAX_FILES = 500

def get_words(text):
   counter = 0
   words = list()
   ftext = open(text)
   line = ftext.readline()
   while line:
      tokens = line.strip().split()
      for t in tokens:
         if t not in words:
            words.append(t)
      counter += 1
      if counter % 1000 == 0:
         print '.'
      line = ftext.readline()
   ftext.close()
   return words

def hashed_path(parent, name, suffix):
   h = hashlib.md5(name).hexdigest()
   dir_num = int(h, 16) % __MAX_FILES
   if not parent.endswith(os.sep): parent += os.sep
   return parent + str(dir_num) + os.path.sep + h + suffix

def make_path_dirs(path):
   path_dir = os.path.dirname(path)
   if not os.path.exists(path_dir):
      os.makedirs(path_dir)

def remove_eps(l):
   while l.count("") != 0:
      l.remove("")
   return l

def generate_txt_fst(fn_morph, fn_text):
   init_state_id = 0
   final_state_id = 1
   state_counter = 2
   ftxt = open(fn_text, 'w')
   fin = open(fn_morph)
   
   substrings = list()
   line = fin.readline()
   substrings = list()
   while line:
      striped_line = line.strip()
      if striped_line != "": 
         [w, path] = line.strip().split('\t')
         tokens = path.split('+')
         tokens = remove_eps(tokens)
	 for i in range(len(tokens)):
	    substring = tokens[i]
	    substrings.append(tokens[i])
	    if i == 0 and i == len(tokens) - 1:
	       ftxt.write("0 1 " + substring + " " + substring + '\n')
            elif i == 0:
	       ftxt.write("0 " + str(state_counter) + " " + substring + " " + substring + '\n')
	    elif i == len(tokens) - 1:
	       ftxt.write(str(state_counter - 1) + " 1 " + substring + " " + substring + '\n')
	    else:
	       ftxt.write(str(state_counter - 1) + " " + str(state_counter) + " " + substring + " " + substring + '\n')
            state_counter += 1
      line = fin.readline()
   ftxt.write("1\n")

   filabel = open(fn_text + ".ilabel", 'w')
   folabel = open(fn_text + ".olabel", 'w')

   filabel.write("<eps> 0\n")
   folabel.write("<eps> 0\n")
   substring_counter = 1
   unique_substrings = set(substrings)
   for s in unique_substrings:
      filabel.write(s + ' ' + str(substring_counter) + '\n')
      folabel.write(s + ' ' + str(substring_counter) + '\n')
      substring_counter += 1

   fin.close()
   ftxt.close()
   filabel.close()
   folabel.close()

def process_word(w, morph_dir, out_dir):
   morph_filename = hashed_path(morph_dir, w, ".mph")
   print morph_filename
   text_filename = hashed_path(out_dir, w, ".txt")
   fst_filename = hashed_path(out_dir, w, ".fst")
   min_fst_filename = hashed_path(out_dir, w, ".min.fst")
   det_fst_filename = hashed_path(out_dir, w, ".det.fst")

   text_filename_isyms = text_filename + '.ilabel'
   text_filename_osyms = text_filename + '.olabel'

   make_path_dirs(text_filename)
   generate_txt_fst(morph_filename, text_filename)

   cmd = 'fstcompile --isymbols=' + text_filename_isyms + \
         ' --osymbols=' + text_filename_osyms + \
         ' --keep_isymbols --keep_osymbols ' + \
         text_filename + ' ' + fst_filename 
   subprocess.Popen(cmd, shell=True).wait()
   cmd = 'fstdeterminize ' + fst_filename + ' ' + det_fst_filename 
   subprocess.Popen(cmd, shell=True).wait()
   cmd = 'fstminimize ' + det_fst_filename + ' ' + min_fst_filename 
   subprocess.Popen(cmd, shell=True).wait()
   cmd = 'rm ' + fst_filename + ' ' + det_fst_filename
   subprocess.Popen(cmd, shell=True).wait()
   cmd = 'mv ' + min_fst_filename + ' ' + fst_filename
   subprocess.Popen(cmd, shell=True).wait()

def main():
   parser = OptionParser()
   parser.add_option("--text", dest = "text", help = "text data in the African langauge")
   parser.add_option("--analysis_dir", dest = "paths_dir", help ="directory of where the analyses are")
   parser.add_option("--output_dir", dest = "out_dir", help = "directory of output morpheme lattices")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   words = get_words(options.text)
   for w in words:
      process_word(w, options.paths_dir, options.out_dir)

if __name__ == "__main__":
   main()

