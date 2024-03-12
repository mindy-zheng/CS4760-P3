#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include <math.h> 
#include <string.h> 
#include <errno.h>
#include <sys/msg.h>

// By Mindy Zheng
// Date: 3/5/24

#define SH_KEY1 89918991
#define SH_KEY2 89928992
#define PERMS 0666


// Setting up message queue: 
typedef struct msgbuffer {                                                      
	long mtype;
	char strData[10]; 
    int intData;
} msgbuffer;


int main(int argc, char** argv) {
	printf("We are in worker, setting up msg queues\n"); 
	 // Setting up message queue to communicate:
    msgbuffer buf;
    buf.mtype = 1;
    int msqid = 0;
    key_t msgkey;
	
	printf("Getting key\n"); 
    // Get a key for our message queue
    if ((msgkey = ftok("msgq.txt", 1)) == -1) {
        perror("ftok failure");
        exit(1);
    }
	printf("Accessing queue\n"); 
    // Access existing queue
    if ((msqid = msgget(msgkey, 0644)) == -1) {
        perror("msgget in child");
        exit(1);
    }
	/*
    //printf("Child %d has access to the queue\n", getpid());
    // receive a message, but only one for us
    if (msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), 0) == -1) {
        perror("failed to receive message from parent\n");
        exit(1);
    }
	*/ 
	printf("We are in worker, setting up memory pointers\n"); // Debugging statement 
	
	// Setting up shared memory pointer for seconds channel 
	int sh_id = shmget(SH_KEY1, sizeof(int) *10, IPC_CREAT | PERMS);
    if (sh_id <= 0) {
        fprintf(stderr,"Shared memory get failed\n");
        exit(1);
    }
    int *seconds = shmat(sh_id, 0, 0);
	printf("Set up seconds\n"); 
	
	// Setting up shared memory channel for nanoseconds 
    sh_id = shmget(SH_KEY2, sizeof(int) *10, IPC_CREAT | PERMS);
    if (sh_id <= 0) {
        fprintf(stderr,"Shared memory get failed\n");
        exit(1);
    }
    int *nanoseconds = shmat(sh_id, 0, 0);
	
	//printf("Successfully set up shared memory channels in worker\n"); 
	// get process ids
    int pid = getpid();
    int ppid = getppid();
    buf.mtype = ppid;                                                           buf.intData = ppid;

	// Create simulated system clock - seconds & nanoseconds 
	int sys_seconds = *seconds; 
	int sys_nano = *nanoseconds; 

	// Calculate time limit 
	int term_seconds = atoi(argv[1]) + sys_seconds; 
	int term_nanoseconds = atoi(argv[2]) + sys_nano; 
	int iterations = 0; 

	
	// Initial state 
	printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n----Just Starting\n", pid, ppid, *seconds, *nanoseconds, term_seconds, term_nanoseconds); 
	
	while ((term_seconds == (*seconds)) && (term_nanoseconds > (*nanoseconds)) || (term_seconds > *seconds)) {
		printf("Worker: entering loop\n"); 
		if (msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), 0) == -1) { 
			perror("Failed to recieve message\n"); 
			exit(1); 
		} 
		iterations++; 
	 	printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n---- %d iterations have passed since starting\n", pid, ppid, *seconds, *nanoseconds, term_seconds, term_nanoseconds, iterations);
		// Send message to parent 
		buf.mtype = ppid; 
		buf.intData = pid; 
		strcpy(buf.strData, "1"); // means true 
	
		if (msgsnd(msqid, &buf, sizeof(msgbuffer)-sizeof(long), 0) == -1) { 
			perror("msgsnd to parent failed\n"); 
			exit(1); 
		} 
	} 

	// Termination state 
	 printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n---- TERMINATING after %d iterations \n", pid, ppid, *seconds, *nanoseconds, term_seconds, term_nanoseconds, iterations);
	// Termination msg to queue 
	buf.mtype = ppid; 
	buf.intData = 0; 
	strcpy(buf.strData, "0"); // indicate false or terminations 
	
	if (msgsnd(msqid, &buf, sizeof(msgbuffer)-sizeof(long), 0) == -1) { 
		perror("WORKER: msgsnd to parent failed\n"); 
		exit(1); 
	} 
			

	printf("Detatching worker shared memory\n"); 	
	// detatch shared memory channels 
	shmdt(seconds); 
	shmdt(nanoseconds); 
	
	return 0; 
} 

