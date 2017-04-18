#!/bin/bash

display_usage() { 
    echo "******Run SinglePE******" 
    echo "Script to take a single PE run with the FADC system"
    echo "store it and extract the constants"
    echo "-d name of the FADC output data file"
    echo "-c for taking data of a given channel"
    echo "-m for taking data of that number of channels"
    echo "-t threshold on the differentiated, pole-zero corrected signal"
    echo "-f flag of the method to run the Single PE spectrum"
    echo "-s Single PE output"
    echo -e "\nUsage:\n$0 -d output_file -t trigger thr. -c/-m channels\n" 
} 


while getopts ':ht:m:c:f:s:d:' option; do
  case "$option" in
    h) display_usage
       exit
       ;;
    c) CHANNEL=$OPTARG 
       ;;
    m) MASK=$OPTARG 
       ;;
    t) THR=$OPTARG 
       ;;
    d) OFILE=$OPTARG 
       ;;
    f) FLAG=$OPTARG 
       ;;
    s) SINGLE_PE_OUTPUT=$OPTARG 
       ;;
    :) printf "missing argument for -%s\n" "$OPTARG" >&2
       echo "$usage" >&2
       exit 1
       ;;
   \?) printf "illegal option: -%s\n" "$OPTARG" >&2
       echo "$usage" >&2
       exit 1
       ;;
  esac
done


# MASK selection
if [[ $CHANNEL ]]; then
    TOTAL_MASK=$(echo "obase=16; 2^($CHANNEL-1)" | bc) # We give the mask of a channel
elif [[ $MASK ]]; then
    TOTAL_MASK=$(echo "obase=16; 2^$MASK-1" | bc) # We give the mask of a series of channels
else
    echo "You did not select a channel or selected channel AND mask"
    exit
fi

# TRIGGER selection
if [[ ! $THR ]]; then
    echo "No trigger threshold selected"
    exit
fi

# FADC OUTPUT file
if [ -z $OFILE ]; then
    if [[ $MASK ]]; then
	OFILE=/mnt/FALCON/data/single_pe_runs/single_pe_S_${MASK}channels_thr${THR}.dat
    else
	OFILE=/mnt/FALCON/data/single_pe_runs/single_pe_S_channel${CHANNEL}_thr${THR}.dat
    fi
    echo "No output file given"
    echo "Using default output $OFILE"
fi

# FLAG selection
if [[ ! $FLAG ]]; then
    echo "No flag selected: running it with flag=1"
    FLAG=1
fi


bash Load_firmware.sh

echo "Running FALCON command"
echo "/root/fadc/fc250b-2.0-server/efbprun /root/fadc/fc250b-2.0-server/test-fc250b -me 1000000 -ei eth1 -a 10 -d 2 -t 0 -athr $THR -am $TOTAL_MASK -es 50 -re 100 -o $OFILE"
echo "Press enter to stop, otherwise wait untill 1e6 events have been taken"

/root/fadc/fc250b-2.0-server/efbprun /root/fadc/fc250b-2.0-server/test-fc250b -me 1000000 -ei eth1 -a 10 -d 2 -t 0 -athr $THR -am $TOTAL_MASK -es 50 -re 100 -o $OFILE

# OFILE is the input to run the single PE spectrum
if [[ $SINGLE_PE_OUTPUT ]]; then
    single_pe -i $OFILE -f $FLAG -n $SINGLE_PE_OUTPUT
    echo "Running single PE spectrum:"
    echo "single_pe -i $OFILE -f $FLAG -n $SINGLE_PE_OUTPUT"
else
    SINGLE_PE_OUTPUT=$(echo $OFILE | awk -F "." '{print $1}')
    echo "Running single PE spectrum:"
    echo "single_pe -i $OFILE -f $FLAG -n $SINGLE_PE_OUTPUT"
    single_pe -i $OFILE -f $FLAG -n $SINGLE_PE_OUTPUT

fi

