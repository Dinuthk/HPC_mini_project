# HPC_mini_project

main.c => mpicc main.c -o main -pthread -fopenmp   /  mpirun -np 3 ./main

main.cpp => nvcc -Xcompiler -fopenmp -Xcompiler -pthread -I/usr/lib/x86_64-linux-gnu/openmpi/include -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi main.cu -o main   =/  mpirun -np 3 ./main