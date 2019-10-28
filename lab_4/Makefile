.PHONY: all
all: heat_diffusion

heat_diffusion: heat_diffusion.c
	gcc -O2 -o heat_diffusion heat_diffusion.c -lm -lOpenCL

.PHONY: run
run: heat_diffusion
	./heat_diffusion -n200 -d0.6 diffusion_100000_100

heat_diffusion.tar.gz: heat_diffusion.c Makefile
	tar -cvzf heat_diffusion.tar.gz heat_diffusion.c heat_diffusion.cl Makefile

.PHONY: test
test: clean heat_diffusion.tar.gz
	./check_submission.py ~/git-labs/lab_4/heat_diffusion.tar.gz

.PHONY: clean
clean:
	rm -rf heat_diffusion extracted/ heat_diffusion.tar.gz
