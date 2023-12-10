#!/bin/sh
inFile=cnf_
outFile=cnf_classification.txt


echo Problem \\t Status \\t Fragment \\t Fragment_sk > $outFile
while read line
do
  #problem=$(echo $line | awk -v prb=$line[0] '{print "Problems/" substr(prb, 1, 3) "/" prb ".p"}')
 # stat=$line[1]

#  echo line: $line
  arrIN=(${line})
  problem=${arrIN[0]}
  stat=${arrIN[1]}
  prb=$(echo $problem | cut -c 1-3)
  path=Problems/$prb/$problem.p


#  echo problem: $problem
#  echo $stat
#  echo $prb
#  echo $path
#  echo

  classification=$(../vampire_dbg_master_6832 --mode classify -t 5m -m 8000 $path | tail -1)
  #classification_sk=$(../vampire_dbg_master_6829 --mode classify_sk -t 5m -m 2048 $path | tail -1)
  classification_sk=$classification

#  echo res: $res
#  echo#
#  res_sk=$(../vampire_dbg_master_6829 --mode classify_sk -t 5m -m 2048 $path)
#  classification_sk=$(echo $res_sk | tail -1)
#
  echo $problem \\t $stat \\t $classification \\t $classification_sk
  echo $problem \\t $stat \\t $classification \\t $classification_sk >> $outFile
done<$inFile
