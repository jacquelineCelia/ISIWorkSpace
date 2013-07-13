#!/usr/bin/python
import os
import subprocess, hashlib
from optparse import OptionParser

__MAX_FILES = 500

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

def process_a_sentence(line, input_dir, output_dir):
   tokens = line.split()
   ilabels = list()
   for t in tokens:
      path = hashed_path(input_dir, t, ".txt.ilabel")
      flabel = open(path)
      l = flabel.readline()
      while l:
         label = l.split()[0]
         if label not in ilabels and label != "<eps>":
            ilabels.append(label)
         l = flabel.readline()
      flabel.close()
   ilabels_path = hashed_path(output_dir, line, ".txt.ilabel")
   make_path_dirs(ilabels_path)
   print len(ilabels)
   fout = open(ilabels_path, 'w')
   fout.write("<eps> 0\n")
   for idx, t in enumerate(ilabels):
      fout.write(t + " " + str(idx + 1) + "\n")
   fout.close()

   olabels_path = hashed_path(output_dir, line, ".txt.olabel")
   cmd = "cp " + ilabels_path + " " + olabels_path
   subprocess.Popen(cmd, shell=True).wait()

   rl_paths = list()

   for t in tokens:
      word_fst_path = hashed_path(input_dir, t, ".fst")
      relabeld_fst_path = hashed_path(input_dir, t, ".rl.fst")
      rl_paths.append(relabeld_fst_path)
      cmd = "fstrelabel --relabel_isymbols=" + ilabels_path + " --relabel_osymbols=" + olabels_path + " " + word_fst_path + " " + relabeld_fst_path
      subprocess.Popen(cmd, shell=True).wait()

   tmp_path = hashed_path(output_dir, line, ".tmp.fst")
   if len(rl_paths) == 1:
      cmd = "cp " + rl_paths[0] + " " + tmp_path
      subprocess.Popen(cmd, shell=True).wait()
   elif len(rl_paths) == 2:
      cmd = "fstconcat " + rl_paths[0] + " " + rl_paths[1] + " " + tmp_path
      subprocess.Popen(cmd, shell=True).wait()
   else:
      cmd = "fstconcat " + rl_paths[0] + " " + rl_paths[1] + " | " 
      for i in range(2, len(rl_paths) - 1):
         cmd += " fstconcat - " + rl_paths[i] + " | "
      cmd += "fstconcat - " + rl_paths[len(rl_paths) - 1] + " " + tmp_path
      subprocess.Popen(cmd, shell=True).wait()

   output_path = hashed_path(output_dir, line, ".fst")

   cmd = "fstrmepsilon " + tmp_path + " " + output_path
   subprocess.Popen(cmd, shell=True).wait()
   
   cmd = "rm " + tmp_path
   subprocess.Popen(cmd, shell=True).wait()

   for f in set(rl_paths):
      cmd = "rm " + f
      subprocess.Popen(cmd, shell=True).wait()

def GenInputPath(line, dirs):
   tokens = line.split('|||')
   path = hashed_path(dirs, tokens[0].strip(), ".fst")
   ilabel_path = hashed_path(dirs, tokens[0].strip(), ".txt.ilabel")
   counter = 0
   fin = open(ilabel_path)
   line = fin.readline()
   while line:
      counter += 1
      line = fin.readline()
   fin.close()
   if counter < 200 and len(tokens[1].split()) < 200:
      return path + ' ||| ' + tokens[1]
   else:
      return ""

def main():
   parser = OptionParser()
   parser.add_option("--text", dest = "bitext", help = "text data in both langauge, separated by |||")
   parser.add_option("--dir", dest = "dirs", help = "directory of sentence lattices")
   parser.add_option("--output", dest = "output", help = "output file")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   fout = open(options.output, 'w')
   fin = open(options.bitext)
   line = fin.readline()
   while line:
      path = GenInputPath(line, options.dirs)
      if path != "":
         fout.write(path)
      line =fin.readline()

   fin.close()
   fout.close()

if __name__ == "__main__":
   main()






