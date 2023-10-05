#!/bin/bash

echo "Starting ns3 simulation with n pads, says 200"

for i in {1..200}
do
    echo "Simulation with ${i} Pads"
    ./ns3 run "scratch/pq-ibe --nPads=${i}" &> pq-ibe-results/tracefile_${i}_simulation.txt
done