# Swim-mill: Concurrent Processing & Shared Memory Location 
Swim-mill is a simple concurrent multiprocess program that simulate a swim-mill to show the behavior of a fish swimming upstream.
- Concurrent multi-process program by forking multiple child processes and using shared memory allocation
- Synchronize and communication between multiple processes by using semaphores to control and gain access to critical resources

## Detailed Project Description:
A program that uses multiple processes to simulate a swim-mill to show the behavior of a fish swimming upstream. The swim mill is represented as a one-dimensional array such that each element indicates the distance from the shore. The fish will occupy one and only one of these elements. The presence of a fish is indicated by changing the integer in the specified element. The fish is represented by the character 'F'. Somewhere upstream, a pellet is dropped into the stream. The fish sees this pellet traveling towards it and moves sideways to enable its capture. If the pellet and the fish are in the same position at any point, the fish eats the pellet, or else it is possible for our fish to miss the pellet. 

### Swim-mill (coordinator)
The coordinator creates fish process, the pellet processes, and coordinates both processes with shared memory. There can be multiple pellet at any time. The swim_mill process will create a pellet at random intervals, with the pellet being dropped into the swim mill at a random distance from the fish.

### Pellet: 
Each pellet process drops a pellet at a random distance from the fish. The pellet will start moving towards the fish with the flow of the river. For simplicity, all the pellet moves in a straight line downstream.

### Fish
The fish sees the pellet as it is dropped and moves one unit left or right to start aligning itself with the pellet. As the pellet moves towards the fish and the fish is at the same distance from the shore as the pellet, it stops changing its sideways position and devours the pellet when they are coincident. The fish is restricted to move in the last row of the 2D array. After the fish eats the pellet, or if the pellet is missed, it  moves towards another pellet.

Multiple concurrent processes are managed by using semaphores, fork, exec, wait and exit in the shared memory allocated. Semaphore is implemented by using semget, semctl, semop, semat, and shmdt to communicate between processes and work together with shared memory. A signal handling is also implemented to terminate all processes and detach/remove shared memory and semaphores when terminated. The swim_mill process also sets a timer at the start of computation to 30 seconds. If computation has not finished by this time, swim_mill kills all the children and grandchildren and then exits. 

