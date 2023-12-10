#!/bin/sh
inFile=fof_classification.txt
mode=1b
# One_Binding, Conjunctive_Binding
fragment="Conjunctive_Binding"
prefix="fof_"

while read line
do
  arrIN=(${line})
  problem=${arrIN[0]}
  stat=${arrIN[1]}
  frag=${arrIN[2]}


  # if fragment is not a substring of frag, skip
  if [[ ! $frag == *"$fragment"* ]]; then
    continue
  fi

  prb=$(echo $problem | cut -c 1-3)
  path=Problems/$prb/$problem.p

  echo Launching $problem
  res=$(../vampire_dbg_master_6832 --mode $mode -sa otter --input_syntax tptp -t 5m -m 8000 --show_preprocessing false -p off -tstat on $path)
  #echo "$res"
  mkdir -p "$prefix$mode/$fragment"
  echo "$res" > "$prefix$mode/$fragment/$problem.out"
  echo "End of $problem"
done<$inFile
