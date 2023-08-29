CORES="2 4 8 6 10"

make cleanexe
make FTH=true all
for CORE in $CORES
do
    make THREADS=$CORE test5
done
