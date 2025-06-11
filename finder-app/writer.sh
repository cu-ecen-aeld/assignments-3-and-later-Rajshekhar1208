#!/bin/bash


writefile=$1

writestr=$2

if [ ! -n "$writefile" ]
then 
	echo "file path not specified"
	exit 1
fi


if [ ! -n "$writestr" ]
then
        echo "string not specified"
	exit 1
fi

mkdir -p "$(dirname "$writefile")" || { 
	echo "Could not create directory" 
	exit 1 
}

echo "$writestr" > "$writefile" || {

	echo "could not write to file"

        exit 1 
}

exit 0
