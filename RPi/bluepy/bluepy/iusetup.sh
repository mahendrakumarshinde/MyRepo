#!/bin/bash
sudo hciconfig hci0 down
sudo hciconfig hci0 up
sudo hcitool lescan > scan.txt
