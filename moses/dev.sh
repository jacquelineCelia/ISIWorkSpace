#!/bin/bash

root_dir="$MOSESROOT/mosesdecoder/"
work_dir=$1
data_dir="$ROOT/swahili/data/"

cd $work_dir

$root_dir/scripts/training/mert-moses.pl $data_dir/weighted.lattice.$2.dev.swa $data_dir/clean.dev.eng $root_dir/bin/moses binarised-model/moses.ini --no-filter-phrase-table --inputtype 2 -mertdir=/usr/users/chiaying/mosesdecoder/bin/ &> mert.out
