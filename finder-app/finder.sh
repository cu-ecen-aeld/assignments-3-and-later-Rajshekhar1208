#!/bin/bash


filesdir=$1

searchstr=$2


if [ ! -n $1 ]
then
	echo "file path not specified"
	exit 1
fi


if [ ! -n $2 ]
then
        echo "string not specified"
	exit 1
fi

X=$(find "$filesdir" -type f | wc -l)

Y=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo The number of files are $X and the number of matching lines are $Y

exit 0
	
