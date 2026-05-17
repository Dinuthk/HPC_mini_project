# HPC_mini_project

main.c => mpicc main.c -o main -pthread -fopenmp   /  mpirun -np 3 ./main

main.cpp => nvcc -Xcompiler -fopenmp -Xcompiler -pthread -I/usr/lib/x86_64-linux-gnu/openmpi/include -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi main.cu -o main   =/  mpirun -np 3 ./main


/project_root
├── /src
│   ├── main.c             # MPI Initialization and routing to Master or Worker
│   ├── master.c           # Master Node logic (scheduling, tracking)
│   ├── worker.c           # Worker Node logic (queue management, main loop)
│   ├── listener.c         # Pthread networking logic (the background thread)
│   └── compute.c          # OpenMP CPU loops (the heavy math)
├── /include
│   ├── common.h           # Structs (Trade, TradeQueue), Tags, and shared globals
│   ├── master.h           # Declarations for master.c
│   ├── worker.h           # Declarations for worker.c
│   ├── listener.h         # Declarations for listener.c
│   └── compute.h          # Declarations for compute.c
└── Makefile               # Critical for compiling the Beowulf cluster code

Final run comand 

# No balancer — will take a LONG time (single thread, 600k tasks)
cd /mnt/d/Github_work_place/HPC_mini_project/no_balancer
make clean && make && ./bin/no_balancer

# HPC — will finish much faster and exit when done
cd /mnt/d/Github_work_place/HPC_mini_project/hpc  
make clean && make && mpirun --allow-run-as-root -np 3 ./bin/load_balancer
