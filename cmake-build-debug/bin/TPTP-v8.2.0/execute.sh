#!/bin/sh
inFile=fof_classification.txt
mode=vampire
# One_Binding, Conjunctive_Binding
fragment="Conjunctive_Binding"
prefix="fof_"
suffix="_no_subterm_sort_det"

for fragment in "One_Binding" "Conjunctive_Binding"
do
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
    # ../vampire_dbg_master_6839 --mode vampire -p off --show_preprocessing false -t 5m -m 100000000 -tstat off -av off -tha off -updr off -fs off -fsr off
    res=$(../vampire_dbg_master_6839 --mode $mode -sa otter --input_syntax tptp -t 5m -m 8000 --show_preprocessing false -p off -tstat on -av off -tha off -updr off -fs off -fsr off $path)
    #echo "$res"
    mkdir -p "$prefix$mode$suffix/$fragment"
    echo "$res" > "$prefix$mode$suffix/$fragment/$problem.out"
    echo "End of $problem"
  done<$inFile
done

for i in {1..10}
do
  echo "\007"
  sleep 1
done