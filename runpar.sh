#!/bin/bash
commfile=$1
n=$2
ctr=0
pids=()
while IFS='' read -r line || [[ -n "$line" ]]; do
    ctr=$((ctr+1))
    echo "Text read from file: $line"
    cmd=$line
    eval $cmd &
    #pid=$! 
    #pids+=($pid)
    if ! ((ctr < n)); then
       wait -n
       #for apid in ${pids[@]};
       #do
       #   echo "waiting for $apid at $ctr"
       #   wait $apid
       #done
       #pids=()
    fi
done < "$commfile"
