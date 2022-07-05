#! /bin/sh

capacity=$(cat /sys/class/power_supply/BAT0/capacity)

echo $capacity

case $capacity in
  "50") 
    notify-send -t 3000 "Battery Warning" "Battery at 50%"
    ;; 
  "20") 
    notify-send -t 3000 "Battery Warning" "Battery at 20%" 
    ;;
  "5") 
    notify-send -t 3000 "BATTERY CRITICALLY LOW" "Battery at 5%" 
    ;;
esac
