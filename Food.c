#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/sem.h>
#include <time.h>

#define SIZE 10

// Global Variables Initialization
// row and col will determine the size of the shared memory
int row = 10, col = 10;

// Fish 'F' is represented with hex 0x46
unsigned char fishy = 0x46;
// Food (Pellet) is represented with hex 0x80
unsigned char foody = 0x80;

// random x and y position integer variables that is retrieved from swim_mill execv function
int position_x, position_y;

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
 * p = semwait. decrement the count of semaphore
 * 		wait to enter the critial region
 * 
 * v = semsignal. increment the count of semaphore
 * 		signal the process to enter the critical region
 * 
 * sembuf: sepcifies a semaphore operations 
 * */
struct sembuf p = {0, -1, SEM_UNDO};
struct sembuf v = {0, +1, SEM_UNDO};

/*
	Prints the shared memory in a two-dimensional format 
	Fish is represented with hex 0x46 (F)
	Food is represented with hex 0x80
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
 * creating a food at a random row & col in the shared memory
 * */
void createFood(char ptr[][col], int postion_x, int position_y){
    for(int i = 0; i < row; i++ ){
        for(int j = 0; j < col; j++){
            ptr[position_x][position_y] = foody;
        }
    }
}

/** 
 * A method to destroy food after it passes the end of mem
 * if food is destroyed if it goes below the last row
 * */
void destroyFood(char ptr[][col]){
    for(int i = 0; i <= row; i++){
        for(int j = 0; j <= col; j++){
            if(ptr[row][col] == -0x80 || ptr[row][col] == 0x80){
                printf("\nPellet Process id: %d\n", getpid());
                printf("Pellet Destoryed. \n");
            }
        }
    }
}

/*
	A method that will handle interrupt (^C) & delete sema & shared mem.
	Uses a signal that will kill all the child processor
	after interupt
*/
void interruptHandler(int sig){
	printf("An interrupt has been invoked in Food processor id: %d\n",(int)getpid());
	fprintf(stderr, "%s", "Error!!\n");
	kill(0, SIGKILL);
}

int main(int argc, char **argv){

    /*
        initialize shared memory variables
        key_t from <sys/types.h> key_t type to initialize shared mem key.
	*/
    key_t SHM_key;
    int shmid;
    char (*SHM_ptr)[col]; 

	// handles interrupts, Kills the processor and childern
    signal(SIGINT, interruptHandler);

    // parse rand x position of the pellet 
    position_x = atoi(argv[1]); 
    // parse rand y position of the pellet 
    position_y = atoi(argv[2]); 

    // attaching shared memory to the previous created shared memory
    SHM_key = ftok(".", 'x');

    // 0666 - means that it is able to access the previous created shared mem
    shmid = shmget(SHM_key, sizeof(char[row][col]), 0666);

    /**
	 * Creating a new semaphore set using the semget. returns a sem_id when it is successful
	 * the key is used same as the shared memory key. 
     * Only 1 elements needs to be passed in the array
	 */
	int sem_id = semget(SHM_key, 1, 0666);

    // return an error upon fail
	if(sem_id < 0){             
		perror("Error: semget");
		exit(11);
	}

	// attaching a shared mem to the segment and get a pointer to it
    // attaching to shared mem using shmat
    SHM_ptr = shmat(shmid, NULL, 0);

    /**
     * To enter the critial region, use p to decrement the counter value.
     * Once entered the critical region, the pellet processor is able to run.
     * The food processor puts a pellet in a random location and moves all the pellet
     * on step down.
     * sem operation performs positive, negative or zero
     * */
    if(semop(sem_id, &p, 1) < 0){
        perror("semop p");
        exit(20);
    }               
    sleep(1);
    createFood(SHM_ptr, position_x, position_y);

    /**
     * Moving the each row one step down in shared memory
     * if the pellet goes beyond the array row, then the pellet destoryed is printed
     * if the pellet is one step above Fish, then the pellet is eaten by the fish.
     * */
    for(int i = row-1; i>=0; i--){
        for(int j = col-1; j>=0; j--){

            // the pellet is negative because, when a pellet moves one step down it changes
            // from 0x80 to -0x80. When a pellet is created at first, it will have a hex value
            // of 0x80. the empty array has a hex value of 0x20. So, when the pellet(0x80) moves
            // one step down to the location of an empty space(0x20). which makes it turn negative 
            if(SHM_ptr[i][j] == -0x80){

                // if a pellet goes below the array. Destroy the pellet
                if(i+1 >= row){     
                    printf("\nPellet Destroyed [%d][%d]\n", i, j); 
                    printf("Pellet Process id: %d\n", getpid());
                }
                    
                // when a pellet is right above the fish, the pellet is eaten
                if(SHM_ptr[i+1][j] == 'F'){
                    printf("\nPellet Eaten [%d] [%d]\n", i,j);
                    printf("Eat Process id: %d\n", getpid());
                // SHM_ptr[i+1][j] |= 0x80;     // bitwise OR logic should to change the bits back to 'F' in binary
                    SHM_ptr[i+1][j] = 'F';
                //     // SHM_ptr[i+1][j] = SHM_ptr[i][j];
                    SHM_ptr[i][j] = 0x20;
                //     // sleep(1);
                //     break;
                }
                else{
                    SHM_ptr[i+1][j] = SHM_ptr[i][j];
                    SHM_ptr[i][j] = 0x20; 
                }

            }

        }
    }

    /**
     * To release the critial region, use v to increment the counter value.
     * After the food processor is done executing, release the critical region so other processor
     * can work
     * */
    if(semop(sem_id, &v, 1) < 0){
        perror("semop p"); 
        exit(21);
    }

    // detach shared memory
    shmdt((void*)SHM_ptr);    
    return 0;
}
