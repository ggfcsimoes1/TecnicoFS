#!/bin/bash
#Script by Gustavo & Miguel 12-11-2020

#Verifying arguments
if [ ! -d "${1}" ] || [ ! -d "${2}" ] || [ "${3}" -lt 0 ]
then
    echo Invalid arguments
    echo Example usage:
    echo ./runTests.sh directory_with_tests directory int_greater_than_0 
    exit
fi

#Executing Tests 
for input in ${1}/*.txt
do  
    for numThreads in $(seq 1 ${3}) 
    do
        inputfile=$(basename "${input}" .txt)
        outputfile=$(basename "${inputfile}-${numThreads}" .txt)
        echo InputFile=${inputfile}.txt NumThreads=${numThreads}
        ./tecnicofs "${1}/${inputfile}.txt" "${2}/${outputfile}.txt" ${numThreads} | grep "TecnicoFS completed in"
    done
done
