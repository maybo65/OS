#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/// gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c
/// ps aux | egrep Z


int prepare(void){
	///change the SIGINT handling to do nothing. 
	struct sigaction sa;
	sa.sa_handler= SIG_IGN;
	if (sigaction(SIGINT, &sa, NULL)<0){
		fprintf(stderr, "prepare failed.\n");
		return -1;
	}
	return 0;
}


int zombie_killer(){ 
	// this is here to prevent zombies in the case of backroung commends.
	struct sigaction sa;
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT;
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGCHLD, &sa, NULL)<0){
		fprintf(stderr, "zombie_killer failed.\n");
		return 0;
	}
	return 1;

}

void reset_sigint_handler(){
	// this is here to set the sigint handler back to defult for the Foreground child processes.
	struct sigaction sa;
	sa.sa_handler= SIG_DFL;
	if(sigaction(SIGINT, &sa, NULL)<0){
		fprintf(stderr, "reset_sigint_handler failed.\n");
		exit(1);
	}

}	

int check_char(char* str){
	if (strcmp(str,"&")==0)
		return 1;
	if (strcmp(str,"|")==0)
		return 2;
	if (strcmp(str,">")==0)
		return 3;
	else
		return 0;
}




int piping(int count, int i, char **arglist){
	// handel the case of a commend with a pipe (|). 
	arglist[i]=NULL; 
	int pfds[2];
	if (pipe(pfds)<0){ 
		fprintf(stderr, "pipe faild in fuction piping.\n");
		return 0;
	}
	int f1=fork(); 
	if (f1<0){
		fprintf(stderr, "fork faild in fuction piping.\n");
		return 0;
	}

	else if (f1==0){ //this is the first child process- the writhing side. 
		reset_sigint_handler();
		if (dup2(pfds[1],1)<0){ // redirecting the output of the first child process to the reading part of the pipe.
			fprintf(stderr, "dup2 error. piping function failed.\n");
			exit(1);
		}
		if(close(pfds[0])==-1){//close the unused side of the pipe since the 1st child dose not read from the pipe.
			fprintf(stderr, "error in closing reading end of pipe.\n");
			exit(1);
		}  
		if(close(pfds[1])==-1){// close the writing part of the pipe since we already redirected this part using the dup2 commend..
			fprintf(stderr, "error in closing writing end of pipe.\n");
			exit(1);
		}  
		if(execvp(arglist[0],arglist)<0){ //excute the program of the child. 
			fprintf(stderr, "invalid commend. piping function failed.\n");
			exit(1);
		}

	}

	else {
		int f2=fork();  //creating 2nd child process 
		if (f2<0){
			fprintf(stderr, "fork faild in fuction piping.\n");
			return 0;
		}
		else if (f2==0){ // this is the process which gonna read from the pipe.
		reset_sigint_handler();
		if (dup2(pfds[0],0)<0){ //replace the standrd input of the second child processes with the input part of the pipe. 
			fprintf(stderr, "dup2 error. piping function failed.\n");
			exit(1);
		}
		if (close(pfds[0])==-1){ // close the reading part of the pipe since we already redirected this part using the dup2 commend.
			fprintf(stderr, "error in closing reading end of pipe.\n");
			exit(1);
		}

		if (close(pfds[1])==-1){//close the unused side of the pipe since the 2nd child dose not write. 
			fprintf(stderr, "error in closing writing end of pipe.\n");
			exit (1);
		} 
		if (execvp(arglist[i+1], arglist+i+1)<0){//excute the program of the 2nd child. 
			fprintf(stderr, "invalid commend1. piping function failed.\n");
			exit(1);
		}

		}
		else{ //the father process.
		if (close(pfds[0])==-1){
			fprintf(stderr, "error in closing reading end of pipe.\n");
			return 0;
		}

		if (close(pfds[1])==-1){ 
			fprintf(stderr, "error in closing writing end of pipe.\n");
			return 0;
		} 
		int wait1 = waitpid(f1,NULL,0);
		int wait2= waitpid(f2,NULL,0);
		if ((wait1>0 && wait2>0) || errno== EINTR || errno== ECHILD){
			return 1;
		}
		else{
			fprintf(stderr, "waitpid failed in function piping.\n");
			return 0;
		}
	}

	}

	return 1; //should never get here.

}

int background(int count, char **arglist){
	// hendel the case of commend which need to be executed in the background (&).
	if (zombie_killer()==0){ //to prevent zombies. 
		exit(1);
	}
	arglist[count-1]=NULL; 
	int f=fork(); 
	if (f<0){
		fprintf(stderr, "fork faild in fuction background.\n");
		exit(1);
	}
	if (f==0){ 
		if (execvp(arglist[0], arglist)<0){
			fprintf(stderr, "invalid commend. background function failed.\n");
			exit(1);
			}
		else
			exit(0);

	}
	else
		// no need to wait since this is a backround commend.
		return 1;

}

int file(int count, char **arglist){
	// handels the case of commands with output redirection (>)
	int f=fork();
	if (f<0){
		fprintf(stderr, "fork faild in fuction file.\n");
		exit(1);
	}
	if (f==0){ // this is the child process
		reset_sigint_handler();
		int fd = open(arglist[count -1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd<0){ 
			fprintf(stderr, "Error in opening file, function file failed.\n");
			exit(1);
		}
		dup2(fd, 1); //reditrecting the output of the program from the stout to the file.
		arglist[count-2]=NULL; 
		close(fd);
		if (execvp(arglist[0], arglist)<0){
			fprintf(stderr, "invalid commend. file function failed.\n");
			exit(1);
		}
		else 
			exit(0);
	}

	else{

		if (waitpid(f,NULL,0)>0 || errno== EINTR || errno== ECHILD)
			return 1; // if we get here, everything is good.
		else{
			fprintf(stderr, "waitpid failed in function file.\n");
			return 0; //if we get here, some problem has occurred.
		}
	}
	
}

int reg(int count, char **arglist){ 
	// handel the case of regular commend.
	int f=fork();
	if (f<0){
		fprintf(stderr, "fork faild in fuction reg.\n");
		exit(1);
	}

	if (f==0){
		reset_sigint_handler();	
		if (execvp(arglist[0], arglist)<0){
			fprintf(stderr, "invalid commend. reg function failed.\n");
			exit(1);	
		}	
	}

	else{
		if (waitpid(f,NULL,0)>0 || errno== EINTR || errno== ECHILD) 
			return 1;
		
		else{
			fprintf(stderr, "waitpid failed in function reg.\n");
			return 0;
		}

	}
	return 1; //should never get here. 
}

int process_arglist(int count, char **arglist) {

	int flag=0;
	int i=0;
	int k;
	//check which kind of commend the arglist contains. 
	while (flag==0 && i<count){
		k=check_char(arglist[i]);
		if(k==0)
			i++;
		else
			flag=1;
	}
	// act accordingly to the type of the commend. 
	if (k==1) 
		return background(count, arglist);
	if (k==2)
		return piping(count, i, arglist);
	if (k==3)
		return file(count, arglist);
	else
		return reg(count, arglist);
	}


int finalize(){
	return 0;
}
