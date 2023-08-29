CORES="2 4 8 16 32 64"

make cleanexe
make FTH=true all
for CORE in $CORES
do
    make THREADS=$CORE test7
done
