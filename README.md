# Operating systems, UniTN [146065]
Project developed for the Operating System course followed during the bachelor's degree in Computer Science at the University of Trento.

## Information
Course professor: Bruno Crispo

Collaborators:
- Battistotti Tommaso
- Berlanda Leonardo 
- Farina Davide 

## Description
### Structure
The project consists of two main parts: 
1. the 'home' program simulates the entire home automation system,
2. the 'manualMode' program is used to interact with the *home* via external agents.

### Build and run
Makefile commands:
- "make": the action set by default is "make help";
- "make help": provides a set of guidelines for using the home automation system;
- "make build": creates the build and bin folders, respectively containing the object files and the compiled executables;
- "make clean": deletes the bin and builds folders.

Once the project has been built with the command above, it is possible to execute the 'home' program by running the "run.sh" script. Similarly, "manualMode.sh" is the script to run the agents to interact with the *home* from the extern.

### Summary
The main objective of the project is to apply the Inter Proces Communication (IPC) technologies presented during the course. These tools are used to manage communication between processes.

We decided to use **anonymous pipes** for each exchange of information between a parent process and its children, as we thought this was the most straightforward way to communicate. 
To manage the exchange of information with the external agents we used **FIFO** and **signals**, as a direct hierarchy between the processes (needed by the pipes) is not available in this setting.
Finally, to use all the techniques seen during the course, we used **message queues** for the link and unlink commands, adopted in the project.
