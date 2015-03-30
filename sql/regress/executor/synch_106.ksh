loopcnt=0
while [ "$loopcnt" -lt "36000" ]
do 
  if [ -f ./test106_synch ]; then
    exit
  fi
  sleep .1 
  let "loopcnt = $loopcnt + 1"
done
