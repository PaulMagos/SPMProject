
SECONDS=0;
max=10
for i in `seq 1 $max`
do
make THREADS=32 tests
done;
echo "$SECONDS"
