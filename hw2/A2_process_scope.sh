#! /bin/bash

# usage: a2_process_scope.sh  number_of_runs_integer
# so from the shell prompt:  $ ./a2_process_scope.sh 10
# will run the multi threaded process named   pctproc
# 10 times and report how many of the runs ended in deadlock

if [ -z $1 ]
then
echo "USAGE: a2_process_scope.sh  number_of_runs_integer"
echo "Try the command again"
echo " "
fi

echo " "
echo "The configuration is for $1 LOOPS"
echo " "

local1=$1
lpcnt=1
count=0
while test $local1 -ne 0
do
echo " "
echo "Working on LOOP $lpcnt"
echo " "
./pctproc 
pid=`ps | grep pctproc | awk '{print $1}'`
# echo JOB AND PID IS $pid
sleep 10
pid=`ps | grep 'pctproc' | awk '{print $1}'`
# if  ps | grep 'pctproc' > /dev/null
if [ -n "$pid" ]
then
echo PID REMAINING IS $pid
echo " "
echo "DEADLOCK DETECTED ON LOOP $lpcnt"
sleep 1
echo " "
count=`expr $count + 1`
echo "#### KILLING ALL THREADS"
# /bin/kill pctproc 2> /dev/null
kill $pid
fi
local1=`expr $local1 - 1`
lpcnt=`expr $lpcnt + 1`
sleep 1
done
echo "        $1 LOOPS and $count DEADLOCK(S)"
echo "        $1 LOOPS and $count DEADLOCK(S)" >> proc_DL
echo
