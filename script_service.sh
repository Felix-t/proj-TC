download_link="https://tabarly.cstb.fr/api/std/v1/config/mac=b8:27:eb:69:90:d0/"
WorkingDirectory="/home/pi/TC_Eolienne/"
program_path="sudo bin/thermocouples -d /dev/ttyUSB0 -f 1"

echo $program_path
$program_path -c config/config_service2 &
pid=$!
echo $pid

while true; do
	curl -H 'Content-Type: text/plain' -o config/config_download $download_link
	if [ "$?" -eq "0" ] ; then
		cmp config/config_download config/current_config
		if [ "$?" -ne "0" ] ; then
			if [ -n "$pid" ] ; then
				kill -15 $pid
			fi
			`$program_path -c config/config_service2` &
			pid=$!
			echo "Changed config file"
		else
			echo "not changed"
		fi
		mv -u -f config/config_download config/current_config
	fi
	filen=( `find Send/ -regex 'Send/.*data_[0-9]+\.csv\.gz'` )

	if [ -n "$filen" ] ; then
		curl -F file=@/home/pi/TC_Eolienne/$filen https://tabarly.cstb.fr/api/std/v1/data/mac=b8:27:eb:69:90:d0/

		if [ "$?" -eq "0" ] ; then
			rm -f $filen
		fi
	fi
	sleep 60
done

exit 0
