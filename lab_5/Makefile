.PHONY: all
all: heat_diffusion

heat_diffusion: heat_diffusion.c
	gcc -O2 -o heat_diffusion heat_diffusion.c -I/usr/include/openmpi-x86_64 -pthread -Wl,-rpath -Wl,/usr/lib64/openmpi/lib -Wl,--enable-new-dtags -L/usr/lib64/openmpi/lib -lmpi

run: heat_diffusion
	mpirun -n 1 heat_diffusion -n2 -d0.0333333333

heat_diffusion.tar.gz: heat_diffusion.c Makefile
	tar -cvzf heat_diffusion.tar.gz heat_diffusion.c Makefile

.PHONY: test
test: clean heat_diffusion.tar.gz
	/home/hpc2019/a5_grading/check_submission.py ~/git-labs/lab_5/heat_diffusion.tar.gz

.PHONY: clean
clean:
	rm -rf heat_diffusion extracted/ heat_diffusion.tar.gz
