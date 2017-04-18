#!/bin/bash
# Script to load the firmware to the FADC cards
# ********** Usage ***********
# ./Load_firmware loads the default version in FADC_VERSION
# ./Load_firmware $version loads $version
display_usage() {
    echo "Script to load the firmware to the FADC cards"
    echo "./Load_firmware loads the default version in FADC_VERSION"
    echo "./Load_firmware \$version loads the \$version you give"
}

# if less than two arguments supplied, display usage                                                                                                                                                        
if [  $# -lt 1 ]; then
    display_usage
    FADC_VERSION="2.0"
else
    FADC_VERSION=$1
fi

# check whether user had supplied -h or --help . If yes display usage
if [[ ( $# == "--help") ||  $# == "-h" ]]
    then
    display_usage
    exit 0
    fi

FADC_PATH=/root/fadc/fc250b-$FADC_VERSION-server

# Ethernet address of the server (ifconfig)
ETH=eth1

for i in {0..2}; do
    # First we check the firmware that is installed in the cards
    CURRENT_VERSION=$($FADC_PATH/fwl-fc250b -ei $ETH -et efb$i -s 10  2>&1 | grep fc250b | gawk -F "fc250b-" '{print $2}' | gawk  '{print $1}')
    # eth1 is the port of the server where we have the FADC connected
    # 10 is the port of the fadc card
    # efb$i is the ethernet address of the card

    if [[ $CURRENT_VERSION == $FADC_VERSION ]]; then
	echo "Correct version of the Firmware loaded"
    else
    # Load firmware for the card
    $FADC_PATH/fwl-fc250b -ei $ETH -et efb$i -in $FADC_PATH/Firmware/adc0.bin -ps 2 -b 10 
    fi
done
