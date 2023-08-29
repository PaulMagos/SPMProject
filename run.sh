
make cleanexe
make all
max=10
for i in `seq 1 $max`
do
make tests
done;
