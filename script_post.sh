#!/bin/bash
b=(1 2 3)
filen=( `find Send/ -regex 'Send/test_data_[0-9]+\.csv\.gz'` )

curl -F file=@/home/pi/TC_Eolienne/$filen https://tabarly.cstb.fr/api/std/v1/data/mac=b8:27:eb:69:90:d0/
