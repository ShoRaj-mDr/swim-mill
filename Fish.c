#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>
#include <math.h>

#define SIZE 10

// Global Variables Initialization
// row and col will determine the size of the shared memory
int row = 10, col = 10;

// Fish 'F' is represented with hex 0x46
unsigned char fishy = 0x46;
// Food (Pellet) is represented with hex 0x80
unsigned char foody = 0x80;

/**
 * p = semwait. decrement the count of semaphore
 * 		wait to enter the critial region
 * 
 * v = semsignal. increment the count of semaphore
 * 		signal the process to enter the critical region
 * 
 * sembuf: sepcifies a semaphore operations 
 * 
 * */
struct sembuf p = {0, -1, SEM_UNDO};
struct sembuf v = {0, +1, SEM_UNDO};


/**
 * Declare a union for the purpose of sharing the same memory location of semaphore operations
 * ALso, get the number of semaphores in a set. Only 1 semaphore operaton at a time.
 */
union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

/**
 * 	Prints the shared memory in a two-dimensional format 
 * 	Fish is represented with hex 0x46 (F)
 * 	Food is represented with hex 0x80
*/
void printCoordinator(char ptr[][col]){
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            printf("[%c\t]", ptr[i][j]);
        }
        printf("\n");
    }
}

/**
 *  A method that calculates the distance between the Food and the Fish.
 *  distance varaible is the distance between food n fish (only column)
 *  distance can be negative-left and positive-right
 */
int distance_Pellet_Food(char ptr[][col]){

    // x and y coordinate for food and fish location
    int locationFood_x;
    int locationFood_y;
    int locationFish_x;
    int locationFish_y;

    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            // get the location of the fish
            if(ptr[i][j] == 0x46){          
                locationFish_x = i;
                locationFish_y = j;
            }
            // get the location of pellet
            if(ptr[i][j] == -0x80 || ptr[i][j] == 0x80){    
                // if(locationFood_y < j)
                    locationFood_y = j;
                    locationFood_x = i;
            }
        }
    }

    // calculate the distance betwen fish and pellet
    // distance calculated according to the column-
    // distance between fish and pellet
    int distance = locationFood_y - locationFish_y;  

    printf("Target Food Location: [%d][%d]\n", locationFood_x, locationFood_y);
    printf("Fish Location: [%d][%d]\n", locationFish_x, locationFish_y);
    printf("Distance betn Fish and Food: %d\n", distance);

    return distance;
}

/**
 * This is which handles fish movements
 * if distance is negative, the Fish will move one place left
 * if distance is positive, the Fish will more one place right
 */
void moveFish(char ptr[][col]){

    int distance = distance_Pellet_Food(ptr);

    for(int i=0; i<row; i++){
        for(int j=0 ; j<col; j++){

            if(ptr[i][j] == 0x46){

                if(distance > 0){
                    ptr[i][j+1] = 0x46;
                    ptr[i][j] = 0x20;
                    break;
                }
                else if(distance < 0){
                    ptr[i][j-1] = 0x46;
                    ptr[i][j] = 0x20;
                    break;
                }
                else if(distance = 0){
                    ptr[i][j] = 0x46;
                    break;
                }
            }

        }
    }   // for loop end
}

/**
 * A method that will handle interrupt (^C) & delete sema & shared mem.
 * Uses a signal that will kill all the child processor after interupt
*/
void interruptHandler(int sig){
	printf("An interrupt has been invoked in Fish processor id: %d\n",(int)getpid());
	fprintf(stderr, "%s", "Error!!\n");
	kill(0, SIGKILL);
}

/**
 * execv: execv(args2[0], args2);
 * argc is the argument count
 * argv is the array character pointers to the argument 
 * */
int main(int argc, char*argv[]){


    // variables for shared memory
    key_t SHM_key;
    int shmid;
    char (*SHM_ptr)[col]; 

    // handles interrupts, Kills the processor and child
    signal(SIGINT, interruptHandler);

    // shared mem with the same key and id as previous created shm mem
    SHM_key = ftok(".", 'x');

    // 0666 - means that it is able to access the previous created shared mem 
    shmid = shmget(SHM_key, sizeof(char[row][col]), 0666);


	/**
	 * Creating a new semaphore set using the semget. returns a sem_id when it is successful
	 * the key is used same as the shared memory key
     * Only 1 elements needs to be passed in the semaphore array
	 */
    int sem_id = semget(SHM_key, 1, 0666);
	if(sem_id < 0){       
		perror("Error: semget");
		exit(15);
	}

	// attaching a shared mem to the segment and get a pointer to it
    SHM_ptr = shmat(shmid, NULL, 0);

    /**
     * To enter the critial region, use p to decrement the counter value.
     * Once in critical region, the Fish processor is the only processor to be able
     * to work with the shared memory at a time.
     * */
    if(semop(sem_id, &p, 1) < 0){ 
        perror("semop p"); 
        exit(30);
    }  

    moveFish(SHM_ptr);     // move fish
    sleep(1);              //  
    shmdt((void*)SHM_ptr); // detach memory

    /**
     * To release the critial region, use v to increment the counter value.
     * Once the critial region is release, the other processor is free to work
     * */
    if(semop(sem_id, &v, 1) < 0){
        perror("semop p"); 
        exit(31);
    }

    return 0;
}

