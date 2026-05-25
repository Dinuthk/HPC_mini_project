cd /home/yasiru/HPC_mini_project
nvcc -O3 -arch=sm_52 -lm -o cuda_load_balancer cuda_load_balancer.cu
./cuda_load_balancer