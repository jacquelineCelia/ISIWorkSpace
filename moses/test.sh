#!/bin/bash

root_dir="$MOSESROOT/mosesdecoder/"
data_dir="$ROOT/swahili/data/"
work_dir=$1

cd $work_dir

$root_dir/bin/moses -f mert-work/moses.ini -inputtype 2 < $data_dir/weighted.lattice.$2.test.swa > translated.en 2> translated.out

$root_dir/scripts/generic/multi-bleu.perl -lc $data_dir/clean.test.eng < translated.en 1>> translated.out
