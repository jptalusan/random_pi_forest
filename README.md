# random_pi_forest
Trying to setup distributed random forest for Pi
needs git, cmake, make and g++ with c++11

Slave Node: Generates a random forest from the training data

Master:
Sends training data to the slave nodes after splitting up
Also runs the random forests from the different nodes and compiles
them to generate the classification.

Requirements:
    - Libmosquitto-dev, libmosquittopp-dev, libssl-dev, cmake
        - sudo apt-get install libmosquitto-dev libmosquittopp-dev libssl-dev cmake -y
    - BATMAN Mesh network: https://jpinjpblog.wordpress.com/2017/12/06/setting-up-b-a-t-m-a-n-mesh-on-rpi3/ 
    
Steps for building:
    - cd distRF
    - mkdir build && cd build
    - cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=..
    - cd ../bin

    for master node:
        cd master_node
        ./master_node

    for slav node:
        cd slave_node
        ./slave_node

    *Modify confisg.json first and add cleaned.csv to the folder (data to send)*
    Configs.json contains the mqtt_broker address and nodename as well as the training parameters for the RF

Make sure you have all the requirements or else it will not build
The same pi could be used as the master or slave or both if needed.
