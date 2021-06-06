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

# define SIZE 10

/*
    Global Variables Initialization
    row and col will determine the size of the shared memory
    fishy and foody represent food and pellet hex values.
    they are unsigned to make the hex values always postive
*/
int row = 10, col = 10;
unsigned char fishy = 0x46;
unsigned char foody = 0x80;

/*
	Creates an empty two-dimensional shared memory
	Inital memory creation will fill the shared mem with integer(00)
	
	Parameters: char ptr[row][col] : SHM_ptr 
	Takes in an array and fills it with 0x20 which is an empty space
*/
void createCoordinator(char ptr[][col]){
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            ptr[i][j] = 0x20;
        }
    }
}

/*
	Creates a Fish, after creating a shared memory.
	Fish is represented as char 'F' (0x46)
	Fish is added in the middle column, last row

	Parameter: char ptr[row][col] = SHM_ptr
*/
void createFish(char ptr[][col]){
    ptr[row-1][4] = fishy;
}

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

/*
	A method that will handle interrupt (^C) & delete sema & shared mem.
	Uses a signal that will kill all the child processor
	after interupt
*/
void interruptHandler(int sig){
	printf("An interrupt has been invoked in processor id %d\n",(int)getpid());
	fprintf(stderr, "%s", "Error!!\n");
	kill(0, SIGKILL);
}

/**
 * Declare a union for the purpose of sharing the same memory location of semaphore operations
 * Get the number of semaphores in a set. Only 1 semaphore operaton at a time.
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


int main(int argc, char *argv[]){

	/*
		variables for shared memory
		key_t from <sys/types.h> key_t type to initialize shared mem key.
	*/
    key_t SHM_key;
    int shmid;                      // specific handler to the shared memory area
    char (*SHM_ptr)[col];           // Shared Mem pointer. This is what will get used to read/write shared mem

	/*
		parent processes and two child processes after fork()
		pid_t data type is a process identification to represent process ids
	*/
    pid_t Food_id;                  // Fork Food Processor id
    pid_t Fish_id;                  // Fish Food Processor id

	/*
		initialize time variables
		arithmetic data type from <time.h> library
	*/
	time_t endwait;
	time_t start = time(NULL);		// using time seed
	time_t seconds = 30;			// loop/program ends after this many seconds. 
	endwait = start + seconds;		// ending time

    SHM_key = ftok(".", 'x');

    // shmget returns an identifier to int shmid
    shmid = shmget(SHM_key, sizeof(char[row][col]), IPC_CREAT | 0666);

	// attaching a shared mem to the segment and get a pointer to it
    SHM_ptr = shmat(shmid, NULL, 0);

    /*
        creating an empty shared memory with a Fish in it
        Param: SHM_ptr
        takes in shared memory, performs read/write on it
    */
    createCoordinator(SHM_ptr);
    createFish(SHM_ptr);
    printCoordinator(SHM_ptr);

	// Detach the memory segment
    shmdt((void*)SHM_ptr);              
    printf("Shared Memory Array Created. \n\n");

	/**
	 * Creating a new semaphore set using the semget. returns a sem_id when it is successful
	 * the key is used same as the shared memory key
	 * Only 1 elements needs to be passed in the semaphore array
	 */
	int sem_id = semget(SHM_key, 1,  0666|IPC_CREAT);
	if(sem_id < 0){			// return an error upon fail
		perror("Error: semget");
		exit(11);
	}

	/**
	 * initialize the semaphore to have a counter of 1
	 * union u to pass to semctl
	 * */
	union semun u;
	// set the value of u to 1
	u.val = 1;		
	
	/**
	 * semctl function provides a variety of semaphore control operations as sepcified by cmd
	 * semctl: lets perfrom one control operation on one semaphore set. It gives up
	 * immediately if any control operation fails.					
	 * */
	if(semctl(sem_id, 0, SETVAL, u) < 0){
		perror("Error: semctl");
		exit(12);
	}

	// args to use for execv . This is the first parameter of execv
    char *args2[] = {"./fish", NULL};

    // handles interrupts, Kills the processor and childern
    signal(SIGINT, interruptHandler);

	// fopen will create a new file or open an existing file
	FILE *fp = fopen("output.txt", "wb");
	printf("Start time: %s", ctime(&start));

	// close the file
	fclose(fp);

	// open the file 
	fp = fopen("output.txt", "a");
	fprintf(fp, "Start time: %s", ctime(&start));

	// Loop that runs for seconds timer
    while(start < endwait){                
		// child 1 processor = Food processor    
        Food_id = fork();

		// Food fork is successful                       
        if(Food_id == 0){
			
			printf("\nPellet Process id: %d\n", getpid());
			
			srand(time(NULL));					// seed time as the random number
    		int x = rand() % (row-3) + 1;		// random row position for spawing pellet
    		int y = rand() % (col-3) + 1;		// random col position for spawing pellet

			printf("Random Produced Pellet Created: [%d] [%d]\n",x,y);

			char rand_position_x[3];			
			sprintf(rand_position_x, "%d", x);	// springf- writes the data in the string pointer to by str
			char rand_position_y[3];
			sprintf(rand_position_y, "%d", y);

			// execv replaces the current process image with a new process image specified by path.
			// No return is made because the call process image is replaced by new
			char *args1[] = {"./food", rand_position_x, rand_position_y, NULL};
            execv(args1[0], args1);             // execute the Food processor
            perror("execv");                    // detect if execv is successful or return error
        }

        else{
            wait(NULL);                         // wait for food process to execute first
            Fish_id = fork();                   // forking Fish child process
            if(Fish_id == 0){                   // child 2 processor = Fish processor
                printf("\nFish Process id: %d\n", getpid());
                execv(args2[0], args2);			// run Fish processor
                perror("execv");
            }

            else{                               // parent process
                wait(NULL);                     // wait for both child processor to finish first
                printf("\nParent Processor id: %d\n", getpid());

                SHM_ptr = shmat(shmid, NULL, 0);
                
                printCoordinator(SHM_ptr);
                fprintf(fp, "Food Process id: %d\n", Food_id);	// write to a file
			    fprintf(fp, "Fish Process id: %d\n", Fish_id);
                
                // loop through shared mem and write it to a file in 2d array format
                for(int i=0; i<row; i++){		
				    for(int j=0; j<col; j++){
					    fprintf(fp, "[%c\t]", SHM_ptr[i][j]);
				    }
				    fprintf(fp, "\n");
			    }

			    fprintf(fp, "\n");
                shmdt((void*)SHM_ptr);

				// end while loop after timer
                ctime(&endwait);
            }
        }
        start = time(NULL);
		// semop(sem_id, &v, 1);
    }

    fclose(fp);
    printf("\nTIME OUT\n");
	fprintf(fp, "End time: %s\n", ctime(&endwait));
	printf("End time: %s\n", ctime(&endwait));

    //clear semaphores, shared memory & buffer
    shmdt((void*)SHM_ptr);

	// detach from shared memory and destroy it
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}
