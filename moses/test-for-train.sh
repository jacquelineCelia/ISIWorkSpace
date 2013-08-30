#!/bin/bash

cd "/data/sls/scratch/chiaying/mt/correct_swahili"

pwd

root_dir="/usr/users/chiaying/mosesdecoder/"
data_dir="/data/sls/scratch/chiaying/mt/data/"
work_dir=$1

cd $work_dir

$root_dir/bin/moses -recover-input-path true -f mert-work/moses.ini -inputtype 2 < $data_dir/weighted.lattice.$2.train.plf > translated.en 2> translated.out

$root_dir/scripts/generic/multi-bleu.perl -lc $data_dir/clean.train.eng < translated.en 1>> translated.out
