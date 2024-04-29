#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

file_1="/work/files/client_files/$1"
file_2="/work/files/server_files/$1"


if [ ! -f "$file_1" ]; then
    echo "Error: File $file_1 does not exist."
    exit 1
fi

if [ ! -f "$file_2" ]; then
    echo "Error: File $file_2 does not exist."
    exit 1
fi

diff_output=$(diff $file_1 $file_2)

if [ $? -eq 0 ]; then
    echo "Files are identical."
else
    echo "Files are different."
    echo "Differences:"
    echo "$diff_output"
fi
