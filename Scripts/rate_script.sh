#!/bin/bash
echo "Script to monitor the rate in a single channel"
echo "output is a file wih the rate per seconds and will be written in /mnt/FALCON/data/rates"
echo "LOADING FIRMWARE"
bash Load_firmware.sh

echo "To select channel use the channel mask: " 
echo "ch 1: 1"
echo "ch 2: 2"
echo "ch 3: 4"
echo "ch 4: 8"
echo "ch 5: 10"
echo "ch 6: 20"
echo "ch 7: 40"
echo "ch 8: 80"
echo "Select the channel (followed by enter): "
read channel
echo "Enter the threshold, 20 should be good for single pe (followed by enter): "
read thr 
echo "output file name: " 
read filename
echo "Running rate scan with channel mask" $channel "and threshold " $thr " and output will be written in  " $filename
FALCON_PATH=/root/fadc/fc250b-2.0-server
echo "Running command $FALCON_PATH/efbprun $FALCON_PATH/test-fc250b -ei eth1 -a 10 -d 2 -t 0 -athr $thr -am $channel -es 4 -re 100"
echo "hit enter to stop"
$FALCON_PATH/efbprun $FALCON_PATH/test-fc250b -ei eth1 -a 10 -d 2 -t 0 -athr $thr -am $channel -es 4 -re 100 2> tmp.txt
cat tmp.txt |grep dead > tmp2.txt; awk {'print $13'} tmp2.txt > /mnt/FALCON/data/rates/$filename
rm -f tmp.txt tmp2.txt
echo "rates script is ready"
echo "ouput written in /mnt/FALCON/data/rates/$filename

  