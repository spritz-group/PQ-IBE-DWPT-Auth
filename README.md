# Privacy Preserving Post Quantum IBE Authentication Protocol for Electric Vehicle Dynamic Charging
This repository contains the simulation and analysis scripts for the **Privacy Preserving Post Quantum IBE Authentication Protocol for Electric Vehicle Dynamic Charging** research paper. 

## Run the ns-3 simulation
To run it, ns-3.38 is required. Once downloaded and installed, copy the pq-ibe.cc script inside the scratch/ folder in ns-3.38.  
```
$ cp your_path/pq-ibe.cc ns-3_path/ns-3.38/scratch/
```

Then, build and run the simulation.  
```
$./ns3 build
$./ns3 run scratch/pq-ibe
```
You can check the simulation's parameters, such as the number of pads, with:
```
$./ns3 run "scratch/pq-ibe --Help"
```

Additionally, a bash script is provided to run multiple tests with different numbers of pads and save the results in a folder and .txt iles for each simulation.
These files can be later used for further analysis.


## Numerical analysis
A simple python script performs this analysis and generates plots.  
It generates and tests the theoretical results of the performance analysis.

## Security Formal Analysis with Scyther
To run this script, you need Scyther tool (https://people.cispa.io/cas.cremers/scyther/).  
Just upload the script in the tool and run it. 
