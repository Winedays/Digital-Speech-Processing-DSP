#!/bin/bash 

if [ $# != 2 ] ; then 
  echo "usage: run_demo.sh <MACHINE_TYPE> <SRIPATH>"
  echo "e.g. run_demo.sh i686-m64 /ta/path/to/srilm-1.5.10"
  exit 1
fi

m_type=$1
sri_path=$(realpath $2)

# 5% Check if "make map" works
#echo "error code now:$?"
rm -f ZhuYin-Big5.map;
echo "make clean..."
make clean
cp bigram.lm.ta bigram.lm
cp trigram.lm.ta trigram.lm

echo "Checking compilation of mydisambig and mapping..."
make MACHINE_TYPE=$m_type SRIPATH=$sri_path
if [ "$?" -ne 0 ]; then
    echo "Compilation failure"
else
    echo "Compilation success."
fi

echo "make map";
make map;
if [ "$?" -ne 0 ]; then 
    echo "Cannot generate maps."; 
else
    echo "Map generated successfully.";
    mv ZhuYin-Big5.map ZhuYin-Big5.map.demo;
fi
cp ZhuYin-Big5.map.ta ZhuYin-Big5.map;

echo "Checking execution of mydisambig...";
rm -rf result2
mkdir result2
timeout 10m make MACHINE_TYPE=$m_type SRIPATH=$sri_path LM=bigram.lm run;
if [ "$?" -ne 0 ]; then
    echo "Execution failure";
else
    echo "Execution success.";
fi
rm ZhuYin-Big5.map;
cp ZhuYin-Big5.map.demo ZhuYin-Big5.map;

tar -zcvf demo.tar.gz ZhuYin-Big5.map result2/
