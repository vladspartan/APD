all:
	gcc -o serial_neoptimizat serial_neoptimizat.c
	gcc -o serial_optimizat serial_optimizat.c
	gcc -fopenmp -o paralel_neoptimizat paralel_neoptimizat.c
	gcc -fopenmp -o paralel_optimizat paralel_optimizat.c
sn:
	gcc -o serial_neoptimizat serial_neoptimizat.c
so:
	gcc -o serial_optimizat serial_optimizat.c
pn:
	gcc -fopenmp -o paralel_neoptimizat paralel_neoptimizat.c
po:
	gcc -fopenmp -o paralel_optimizat paralel_optimizat.c

clean:
	rm paralel_optimizat paralel_neoptimizat serial_optimizat serial_neoptimizat
