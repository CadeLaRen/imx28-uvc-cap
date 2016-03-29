#!/bin/sh
CPUREV=$(cat /proc/cpuinfo | grep Revision | awk '{print $3}' | awk '{print substr($0,1,2)}')
FILEVG=/usr/lib/libOpenVG.so
FILEVG3D=/usr/lib/libOpenVG_3D.so
FILEVG355=/usr/lib/libOpenVG_355.so
if  [ -e $FILEVG3D ] && [ -e $FILEVG355 ]
then
  if  [ $CPUREV == "61" ] || [ $CPUREV == "63" ] || [ $CPUREV == "60" ] && [ -e  $FILEVG ]
  then
        rm -f $FILEVG
  fi
  if [ $CPUREV == "61" ]
  then
        ln -s $FILEVG3D $FILEVG
  fi
  if [ $CPUREV == "63" ]
  then
        ln -s $FILEVG355 $FILEVG
  fi
  if [ $CPUREV == "60" ]
  then
        ln -s $FILEVG355 $FILEVG
  fi
fi
