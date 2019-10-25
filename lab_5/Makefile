.PHONY: all
all: heat_diffusion

heat_diffusion: heat_diffusion.c
	gcc -O3 -o heat_diffusion heat_diffusion.c -I/usr/include/openmpi-x86_64 -pthread -Wl,-rpath -Wl,/usr/lib64/openmpi/lib -Wl,--enable-new-dtags -L/usr/lib64/openmpi/lib -lmpi

.PHONY: run
run: heat_diffusion
	mpirun -n 2 heat_diffusion -d0.0333333 -n2

.PHONY: run1
run1: heat_diffusion
	mpirun -n 1 heat_diffusion -d0.01 -n100000 diffusion_100_100

.PHONY: run10
run10: heat_diffusion
	mpirun -n 10 heat_diffusion -d0.01 -n100000 diffusion_100_100

heat_diffusion.tar.gz: heat_diffusion.c Makefile
	tar -cvzf heat_diffusion.tar.gz heat_diffusion.c Makefile

.PHONY: test
test: clean heat_diffusion.tar.gz
	/home/hpc2019/a5_grading/check_submission.py ~/git-labs/lab_5/heat_diffusion.tar.gz

.PHONY: clean
clean:
	rm -rf heat_diffusion extracted/ heat_diffusion.tar.gz
