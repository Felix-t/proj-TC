download_link="https://tabarly.cstb.fr/api/std/v1/config/mac=b8:27:eb:69:90:d0/"
WorkingDirectory="/home/pi/TC_Eolienne/"
program_path="/home/pi/TC_Eolienne/bin/thermocouples -d \"/dev/ttyUSB0\""

while true; do
	curl -o config/config_download $download_link
	if [ "$?" -eq "0" ] ; then
		cmp config/config_download config/current_config
		if [ "$?" -ne "0" ] ; then
			if [ -n "$pid" ] ; then
				kill -15 $pid
			fi
			`$program_path -c config/config_download` &
			pid=$!
			echo "Changed config file"
		else
			echo "not changed"
		fi
		mv config/config_download config/current_config
	fi
	filen=( `find Send/ -regex 'Send/test_data_[0-9]+\.csv\.gz'` )

	curl -F file=@/home/pi/TC_Eolienne/$filen https://tabarly.cstb.fr/api/std/v1/data/mac=b8:27:eb:69:90:d0/
	if [ "$?" -eq "0" ] ; then
		rm $filen
	fi
	sleep 60
done

exit 0
