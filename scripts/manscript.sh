#!/bin/bash
ip_address=$2
out=$(sudo batctl o | wc -l)
ifBatExists=$(ip a show bat0 up | wc -l)
force=$1
echo "$ifBatExists"
if [ "$ifBatExists" -gt 1 ] && [ "$out" -gt 2 ] && [ -z "$force" ]; then
    echo "Already setup batman!"
else
    echo "starting batman"
    sudo modprobe batman-adv
    echo "Sleeping 5 seconds!!"
    sleep 5s
    echo "Setting wlan down"
    sudo ip link set wlan0 down

    sudo ifconfig wlan0 mtu 1532
    sudo iwconfig wlan0 mode ad-hoc
    sudo iwconfig wlan0 essid my-mesh-network
    sudo iwconfig wlan0 ap any
    sudo iwconfig wlan0 channel 1
    echo "Sleeping for 2 seconds"
    sleep 2s

    echo "Setting wlan0 up"
    sudo ip link set wlan0 up

    echo "Sleeping for 2 seconds"
    sleep 2s

    sudo batctl if add wlan0
    echo "Sleeping for 2 seconds"
    sleep 2s

    echo "Setting bat0 up"
    sudo ifconfig bat0 up

    echo "Sleeping for 5 seconds"
    sleep 5s

    sudo ifconfig bat0 "$2"/16
    echo "Done!!!"
fi
