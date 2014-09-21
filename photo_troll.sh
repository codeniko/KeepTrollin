#!/bin/bash

for file in ~/{,Pictures,Downloads}/*; do 
	e=`file "${file}" | cut -d ' ' -f 2`
	if [ "${e}" == "GIF" ] || [ "${e}" == "JPEG" ] || [ "${e}" == "PNG" ]; then 
		if [ "${1}" == "flip" ]; then
			mogrify -flip "${file}" 
		else
			mogrify -blur 4 "${file}"
		fi
	fi
done
