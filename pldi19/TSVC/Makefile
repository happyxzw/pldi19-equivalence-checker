
OPTS=-std=c99 -msse4.2

#default: others

all:
	gcc $(OPTS) -O1 clean.c -c -o baseline.o
	gcc $(OPTS) -O3 clean.c -c -o gcc.o
	clang $(OPTS) -O3 clean.c -c -o llvm.o
	ld baseline.o -T static_segments.ld -o baseline.bin 2>/dev/null
	ld gcc.o -T static_segments.ld -o gcc.bin 2>/dev/null
	ld llvm.o -T static_segments_llvm.ld -o llvm.bin 2>/dev/null
	stoke extract -i baseline.bin -o baseline
	stoke extract -i gcc.bin -o gcc
	stoke extract -i llvm.bin -o llvm

others:
	gcc $(OPTS) -O3 -c others.c -o others-gcc.o
	clang $(OPTS) -O3 -c others.c -o others-llvm.o

.force:

pdfs: .force
	rm -rf pdfs
	./make_pdfs.rb

tcgen:
	rm -rf testcases
	mkdir -p testcases
	tcgen_tsvc --num_testcases 8 > testcases/8
	# quad
	tcgen_tsvc --num_testcases 8 > testcases/8quad
	tcgen_tsvc --num_testcases 8 >> testcases/8quad
	tcgen_tsvc --num_testcases 8 >> testcases/8quad
	tcgen_tsvc --num_testcases 8 >> testcases/8quad
	# 96
	tcgen_tsvc --num_testcases 8 > testcases/96
	tcgen_tsvc --num_testcases 8 >> testcases/96
	tcgen_tsvc --num_testcases 8 >> testcases/96
	tcgen_tsvc --num_testcases 8 >> testcases/96
	tcgen_tsvc --num_testcases 16 >> testcases/96
	tcgen_tsvc --num_testcases 16 >> testcases/96
	tcgen_tsvc --num_testcases 16 >> testcases/96
	tcgen_tsvc --num_testcases 16 >> testcases/96
	# 128
	tcgen_tsvc --num_testcases 4 > testcases/128
	tcgen_tsvc --num_testcases 16 >> testcases/128
	tcgen_tsvc --num_testcases 8 >> testcases/128
	tcgen_tsvc --num_testcases 8 >> testcases/128
	tcgen_tsvc --num_testcases 8 >> testcases/128
	tcgen_tsvc --num_testcases 8 >> testcases/128
	tcgen_tsvc --num_testcases 12 >> testcases/128
	tcgen_tsvc --num_testcases 12 >> testcases/128
	tcgen_tsvc --num_testcases 12 >> testcases/128
	tcgen_tsvc --num_testcases 12 >> testcases/128
	tcgen_tsvc --num_testcases 16 >> testcases/128
	tcgen_tsvc --num_testcases 4 >> testcases/128
	tcgen_tsvc --num_testcases 4 >> testcases/128
	tcgen_tsvc --num_testcases 4 >> testcases/128
	# 192
	tcgen_tsvc --num_testcases 8 > testcases/192
	tcgen_tsvc --num_testcases 8 >> testcases/192
	tcgen_tsvc --num_testcases 8 >> testcases/192
	tcgen_tsvc --num_testcases 8 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 12 >> testcases/192
	tcgen_tsvc --num_testcases 16 >> testcases/192
	tcgen_tsvc --num_testcases 16 >> testcases/192
	tcgen_tsvc --num_testcases 16 >> testcases/192
	tcgen_tsvc --num_testcases 16 >> testcases/192
	# sixteen
	tcgen_tsvc --num_testcases 16 > testcases/16
	#256
	tcgen_tsvc --num_testcases 16 > testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256
	tcgen_tsvc --num_testcases 16 >> testcases/256

clean:
	rm -f *.o *.bin 
	rm -rf llvm gcc baseline
	rm -rf testcases/*
	
plots:
	gnuplot search.gnuplot
	gnuplot performance.gnuplot
	gnuplot search-gcc.gnuplot
	gnuplot performance-gcc.gnuplot
