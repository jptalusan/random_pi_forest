inotifywait -q -m -r -e create -e moved_to /home/pi/RandomForest_for_smart_home_data/data |
    while read path action file; do
        echo "'$file' appeared in directory '$path'"
        sleep 2s
        cd /home/pi/RandomForest_for_smart_home_data
        ./rf_exe
    done
