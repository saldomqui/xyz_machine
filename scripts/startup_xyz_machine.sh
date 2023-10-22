#!/bin/bash 


if [ "$#" == 1 ] && [ "$1" == "laptop" ]; then
  echo "Launching XYZ machine software for laptop setup"
  cd ~/thorvald_ws
else
  echo "Launching XYZ machine software for raspberry setup"
  cd ~/software/catkin_ws
fi
./devel/lib/xyz_machine/xyz_machine -f ./src/xyz_machine/cfg/config.yaml 




