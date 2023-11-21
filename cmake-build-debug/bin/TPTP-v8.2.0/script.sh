#!/bin/zsh
inFile=fof_results.txt
outFile=fof_results_2.txt
true > $outFile
for problem in $(cat $inFile | awk '$2!="None"{print $1}')
do
  res=$(../vampire_dbg_master_6828 --mode classify --show_preprocessing true -t 5m -m 2048 $problem)
  classification=$(echo $res | tail -1)
  echo $problem $classification
  echo $problem $classification >> $outFile
done