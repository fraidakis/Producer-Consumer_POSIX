#!/bin/bash

#SBATCH --job-name=bitonic
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --time=00:01:00
#SBATCH --partition=rome
#SBATCH --mem=1G
#SBATCH --output=temporary.txt
#!SBATCH --exclusive
#!SBATCH --test-only


module load gcc

cd results/
bash results.sh




# for q in {1..20}
# do
#     make run p=1 q=$q
#     echo ""
# done


# p=1

# make run p=$p q=1
# make run p=$p q=2
# make run p=$p q=4
# make run p=$p q=8
# make run p=$p q=16
# make run p=$p q=32
# make run p=$p q=64
