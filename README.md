# random_pi_forest
Trying to setup distributed random forest for Pi

Slave Node: Generates a random forest from the training data
make
./src/rf_exe

Master:
Sends training data to the slave nodes after splitting up
Also runs the random forests from the different nodes and compiles
them to generate the classification.

Two files:
Distributed: Uses threads to parse and distribute training data
Classifier: Uses Random forest from nodes to classify data

Watcher (on slave node): Watches for the training data from master and 
starts random forest generation
Running watcher, change notifier path to point to ./src/rf_exe
nohup ../watcher/notifier.sh >/dev/null 2>&1 &


