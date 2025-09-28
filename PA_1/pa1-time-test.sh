rm answer.txt
for i in 256 1000 10000 100000 1000000 10000000 100000000 1000000000
do
	file_size=$i
	echo $file_size >> answer.txt
	truncate -s $file_size BIMDC/test.bin
	{ time ./client -f test.bin > /dev/null 2>&1 ; } 2>> answer.txt
	echo "\n" >> answer.txt
	rm BIMDC/test.bin received/test.bin
done

