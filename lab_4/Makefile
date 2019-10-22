.PHONY: all
all: heat_diffusion

heat_diffusion: heat_diffusion.c
	gcc -O3 -o heat_diffusion heat_diffusion.c -lm 

run: heat_diffusion
	./heat_diffusion -n20 -d0.02

heat_diffusion.tar.gz: heat_diffusion.c Makefile
	tar -cvzf heat_diffusion.tar.gz heat_diffusion.c Makefile

.PHONY: test
test: clean heat_diffusion.tar.gz
	/home/hpc2019/a4_grading/check_submission.py ~/git-labs/lab_4/heat_diffusion.tar.gz

.PHONY: clean
clean:
	rm -rf heat_diffusion extracted/ heat_diffusion.tar.gz
