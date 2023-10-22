#!/bin/bash 

if [ "$#" == 2 ]; then
  if [ "$1" == "laptop" ]; then
    echo "Launching software for laptop setup"
    export CATKIN_WS=~/thorvald_ws
  else
    echo "Launching software for raspberry setup"
    export CATKIN_WS=~/software_catkin_ws
  fi

  cd $CATKIN_WS
  
  export PLATFORM="$1"
  export APP="$2"

  session="xyz-machine"

  tmux new-session -d -s $session

  tmux rename-window -t $session:0 'machine_control'
  tmux new-window -t $session:1 
  tmux rename-window -t $session:1 $2

  tmux send-keys -t $session:0 "${CATKIN_WS}/devel/lib/xyz_machine/xyz_machine -f ${CATKIN_WS}/src/xyz_machine/cfg/config.yaml"  C-m
  tmux send-keys -t $session:1 "${CATKIN_WS}/devel/lib/thermal_webcam/thermal_webcam -f ${CATKIN_WS}/src/thermal_webcam/cfg/config_${APP}_${PLATFORM}.yaml -s ${HOME}/Desktop/snapshots/${APP}"  C-m

  tmux attach-session -t $session
else
  echo "ERROR: Wrong number of arguments. Use: startup_tmux.sh <laptop/raspberry> <thermal_webcam/microscope>"
fi

