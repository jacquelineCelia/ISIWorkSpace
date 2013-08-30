#!/bin/bash


root_dir="/usr/users/chiaying/mosesdecoder/"
work_dir=$1

mkdir -p $work_dir/model/

$root_dir/scripts/training/giza2bal.pl -d "gzip -cd $2" -i "gzip -cd $3" | ../bin/symal -alignment="grow" -diagonal="yes" -final="yes" -both="yes" > $work_dir/model/aligned.grow-diag-final-and
