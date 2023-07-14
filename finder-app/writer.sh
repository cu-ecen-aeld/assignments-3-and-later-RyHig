#!/bin/sh

usage="Usage: $0 /path/to/file text_to_write"

#if [[ ( $@ == "--help") || ($@ == "-h") ]] then
#    echo $usage
#    echo "Flags:"
#    printf "\t--help, -h\t\tHelp.\n"
#    exit 0 
#fi

if [ $# -ne 2 ]
then
    echo "$0 needs exactly 2 arguments, you provided $#"
    echo $usage
    exit 1
fi

writefile=$1
writestr=$2

mkdir -p `dirname $writefile`
echo $writestr > $writefile

if [ -e $writefile ]
then
    echo "$writefile sucessfully created."
    if [ `cat $writefile` == $writestr ]
    then
        echo "$writestr successfully written to $writefile"
    else
        echo "The contents of $writestr does not match $writefile"
        exit 1
    fi
else
    echo "file could not be created"
    exit 1
fi

exit 0
