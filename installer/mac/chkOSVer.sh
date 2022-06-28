#!/bin/bash

# http://stackoverflow.com/questions/4023830/how-compare-two-strings-in-dot-separated-version-format-in-bash
vercomp () 
{
    if [[ $1 == $2 ]]
    then
        return 0
    fi
    local IFS=.
    local i ver1=($1) ver2=($2)
    # fill empty fields in ver1 with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
    do
        ver1[i]=0
    done
    for ((i=0; i<${#ver1[@]}; i++))
    do
        if [[ -z ${ver2[i]} ]]
        then
            # fill empty fields in ver2 with zeros
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]}))
        then
            return 1
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]}))
        then
            return 2
        fi
    done
    return 0
}

#-------------------------------------------------------------------
osVer="$(sw_vers -productVersion)"
minVer="10.12.0"

echo "current OS version: ${osVer}"
echo "minimum OS Version: $minVer"


vercomp ${osVer} $minVer

case $? in
        0) 
		op='='
		echo "result: os version equal minimum OS Version"
		exit 0
		;;
        1) 
		op='>'
		echo "result: os version bigger than minimum OS Version"
		exit 0
		;;
        2) 
		op='<'
		echo "result: os version smaller than minimum OS Version"
		exit -1
		;;
esac

exit -1
