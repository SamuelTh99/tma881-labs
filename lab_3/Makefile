.PHONY: all
all: cell_distances

cell_distances: cell_distances.c
	gcc -O2 -o cell_distances cell_distances.c -lm

run: cell_distances
	./cell_distances -t5

cell_distances.tar.gz: cell_distances.c Makefile
	tar -cvzf cell_distances.tar.gz cell_distances.c Makefile

.PHONY: test
test: cell_distances.tar.gz
	/home/hpc2019/a3_grading/check_submission.py ~/git-labs/lab_3/cell_distances.tar.gz

.PHONY: clean
clean:
	rm -rf cell_distances extracted/ cell_distances.tar.gz
