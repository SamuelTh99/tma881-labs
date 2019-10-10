.PHONY: all
all: newton

newton: newton.c
	gcc -O2 -o newton newton.c -lm -lpthread

.PHONY: images
images: newton
	for d in {1..9}; do \
		echo "d = $$d" ;\
		./newton -t4 -l1000 $$d ;\
	done

newton.tar.gz: newton.c Makefile
	tar -cvzf newton.tar.gz newton.c Makefile

.PHONY: test
test: newton.tar.gz
	/home/hpc2019/a2_grading/check_submission.py ~/git-labs/lab_2/newton.tar.gz

.PHONY: clean
clean:
	rm -rf newton extracted/ newton.tar.gz
