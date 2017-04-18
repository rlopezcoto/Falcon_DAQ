#!/bin/bash
display_usage() { 
    echo "******Run Cosmic-ray data******" 
    echo "Script to take cosmic ray data and "
    echo "convert it to XCDF"
    echo "-d name of the FADC output data file"
    echo "-c for taking data of a given channel"
    echo "-m for taking data of that number of channels"
    echo "-t threshold on the differentiated, pole-zero corrected signal"
    echo "-f flag of the method to select the Single PE spectrum"
    echo "-x multiplicity of the multiplicity trigger"
    echo "-w width of the coincidence window"
    echo -e "\nUsage:\n$0 -d output_file -t trigger thr. -c/-m channels\n" 
} 


while getopts ':ht:m:c:f:x:w:d:' option; do
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
    x) MULTIPLICITY=$OPTARG 
       ;;
    w) WIDTH=$OPTARG 
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

# MULTIPLICITY and WIDTH selection
if [[ $MULTIPLICITY && ! $WIDTH ]] || [[ ! $MULTIPLICITY &&  $WIDTH ]]; then
    echo "You cannot select a multiplicity/width for the"
    echo "majority trigger without specifying the other"
    exit
fi


# MASK selection
if [[ $CHANNEL ]]; then
    TOTAL_MASK=$(echo "obase=16; 2^($CHANNEL-1)" | bc) # We give the mask of a channel
    if [[ $MULTIPLICITY || $WIDTH ]]; then
	echo "You cannot do a multiplicity trigger if you select only one channel"
	echo "Setting them to null"
	MULTIPLICITY=
	WIDTH=
    fi
elif [[ $MASK ]]; then
    TOTAL_MASK=$(echo "obase=16; 2^$MASK-1" | bc) # We give the mask of a series of channels
else
    echo "You did not select a channel or selected channel AND mask, type -h for options"
    exit
fi

# TRIGGER selection
if [[ ! $THR ]]; then
    echo "No trigger threshold selected"
    exit
fi

# FADC OUTPUT file
if [ -z $OFILE ]; then
    echo "OFILE $OFILE"
    if [[ $MASK ]]; then
	OFILE=/mnt/FALCON/data/air_shower_runs/data_run_D_${MASK}channels_thr${THR}.dat
	single_pe_files=$(ls /mnt/FALCON/data/single_pe_runs/single_pe_S_${MASK}channels*.txt)
	echo $single_pe_files
	set -- $single_pe_files
	SINGLE_PE=$1
    else
	OFILE=/mnt/FALCON/data/air_shower_runs/data_run_D_channel${CHANNEL}_thr${THR}.dat
	single_pe_files=$(ls /mnt/FALCON/data/single_pe_runs/single_pe_S_channel${CHANNEL}*.txt)
	echo $single_pe_files
	set -- $single_pe_files
	SINGLE_PE=$1
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

echo "Running Falcon with command:"
echo "/root/fadc/fc250b-2.0-server/efbprun /root/fadc/fc250b-2.0-server/test-fc250b -me 1000000 -ei eth1 -a 10 -d 2 -t 0 -athr $THR -am $TOTAL_MASK -es 50 -re 100 -o $OFILE -amaj $MULTIPLICITY,$WIDTH"

/root/fadc/fc250b-2.0-server/efbprun /root/fadc/fc250b-2.0-server/test-fc250b -me 1000000 -ei eth1 -a 10 -d 2 -t 0 -athr $THR -am $TOTAL_MASK -es 50 -re 100 -o $OFILE -amaj $MULTIPLICITY,$WIDTH


XCDF_OUTPUT=$(echo $OFILE | awk -F "." '{print $1}')
echo "Running Flash2Falcon command"
echo "Flash2Falcon -i $OFILE -o $XCDF_OUTPUT.xcdf -f $FLAG -c $SINGLE_PE"
Flash2Falcon -i $OFILE -o $XCDF_OUTPUT.xcdf -f $FLAG -c $SINGLE_PE