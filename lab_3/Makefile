.PHONY: all
all: cell_distances

cell_distances: cell_distances.c
	gcc -O3 -fopenmp -o cell_distances cell_distances.c -lm -lgomp

.PHONY: mac
mac: cell_distances.c
	gcc-9 -O3 -fopenmp -o cell_distances cell_distances.c -lm -lgomp

omp_test: omp_test.c
	gcc-9 -O2 -fopenmp -o omp_test omp_test.c -lm -lgomp

run: cell_distances
	./cell_distances -t5

cell_distances.tar.gz: cell_distances.c Makefile
	tar -cvzf cell_distances.tar.gz cell_distances.c Makefile

.PHONY: test
test: clean cell_distances.tar.gz
	/home/hpc2019/a3_grading/check_submission.py ~/git-labs/lab_3/cell_distances.tar.gz

.PHONY: clean
clean:
	rm -rf cell_distances distances/ extracted/ cell_distances.tar.gz
