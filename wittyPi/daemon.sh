#!/bin/bash
# file: daemon.sh
#
# This script should be auto started, to support WittyPi hardware
#

# check if sudo is used
if [ "$(id -u)" != 0 ]; then
  echo 'Sorry, you need to run this script with sudo'
  exit 1
fi

# get current directory
cur_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# utilities
. "$cur_dir/utilities.sh"

log 'Witty Pi 2 daemon (v2.52) is started.'

# halt by GPIO-4 (BCM naming)
halt_pin=4

# make sure the halt pin is input with internal pull up
gpio -g mode $halt_pin up
gpio -g mode $halt_pin in

# LED on GPIO-17 (BCM naming)
led_pin=17

# wait for RTC ready
sleep 2

# if RTC presents
has_rtc=is_rtc_connected

if $has_rtc ; then
  # disable square wave and enable alarms
  i2c_write 0x01 0x68 0x0E 0x07

  byte_F=$(i2c_read 0x01 0x68 0x0F)

  # if woke up by alarm B (shutdown), turn it off immediately
  if [ $((($byte_F&0x1) == 0)) == '1' ] && [ $((($byte_F&0x2) != 0)) == '1' ] ; then
    log 'Seems I was unexpectedly woken up by shutdown alarm, must go back to sleep...'
    do_shutdown $halt_pin $led_pin
  fi

  # clear alarm flags
  clear_alarm_flags $byte_F
else
  log 'Witty Pi is not connected, skip I2C communications...'
fi

# synchronize time
if $has_rtc ; then
  "$cur_dir/syncTime.sh" &
else
  log 'Witty Pi is not connected, skip synchronizing time...'
fi

# wait for system time update
sleep 3

# run schedule script
if $has_rtc ; then
  "$cur_dir/runScript.sh" >> "$cur_dir/schedule.log" &
else
  log 'Witty Pi is not connected, skip schedule script...'
fi

# delay until GPIO pin state gets stable
counter=0
while [ $counter -lt 5 ]; do  # increase this value if it needs more time
  if [ $(gpio -g read $halt_pin) == '1' ] ; then
    counter=$(($counter+1))
  else
    counter=0
  fi
  sleep 1
done

# run extra tasks in background
"$cur_dir/extraTasks.sh" >> "$cur_dir/wittyPi.log" 2>&1 &

# wait for GPIO-4 (BCM naming) falling, or alarm B (shutdown)
log 'Pending for incoming shutdown command...'
while true; do
  gpio -g wfi $halt_pin falling
  if $has_rtc ; then
    byte_F=$(i2c_read 0x01 0x68 0x0F)
    if [ $((($byte_F&0x1) != 0)) == '1' ] && [ $((($byte_F&0x2) == 0)) == '1' ] ; then
      # alarm A (startup) occurs, clear flags and ignore
      log 'Startup alarm occurs in ON state, ignored'
      clear_alarm_flags
    else
      break;
    fi
  else
    # power switch can still work without RTC
    break;
  fi
done
log 'Shutdown command is received...'

do_shutdown $halt_pin $led_pin
