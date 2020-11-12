#!/bin/bash
#Script by Gustavo & Miguel 12-11-2020

for input in ${1}/*.txt
do  
    for numThreads in $(seq 1 ${3}) 
    do
        inputfile=$(basename "${input}" .txt)
        outputfile=$(basename "${inputfile}-${numThreads}" .txt)
        echo InputFile=${inputfile}.txt NumThreads=${numThreads}
        ./tecnicofs "${1}/${inputfile}.txt" "${2}/${outputfile}.txt" ${numThreads} >  ${2}/temp.txt
        cat ${2}/temp.txt | grep "TecnicoFS completed in" ${2}/temp.txt
        rm ${2}/temp.txt
    done
done
