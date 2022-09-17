#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <errno.h>

typedef struct node
{
    struct node* next;
    char* directory;

}node;

typedef struct queue
{
    struct node *head;
    struct node *tail;
    atomic_int size;

}queue;

typedef struct argument
{
    struct queue *Q;
    char* term;
    int num_of_threads;

}argument;

pthread_mutex_t qlock;
pthread_cond_t cond;
pthread_mutex_t start;
pthread_cond_t cond_start;
atomic_int counter = 0; //this is the number of awake threads.
atomic_int error = 0; //this is a flag indicating if an error happend in one of the threads.
atomic_int num_of_files;// number of files that the program has printed.
atomic_int initiated_threads=0;

char* abs_path(char* directory, struct dirent* entry) //given an dentry, this function will returns the asb-path of this directory.
{
    int len;
    char* abspath;
    len = strlen(directory) + strlen(entry->d_name) + 2;
    abspath = malloc(sizeof(char) * (len));
    if (abspath==NULL){
        fprintf(stderr, "problem with memory allocation in abs_path.\n");
        return NULL;
    }
    strcpy(abspath, directory);
    strcat(abspath, "/");
    strcat(abspath, entry->d_name);
    abspath[len - 1] = '\0';
    return abspath;
}

void enqueue(queue* Q, node *x)
{
    pthread_mutex_lock(&qlock);
    if(Q->size==0){ //the queue is empty, hence the head and the tail both be the new node.
        Q->tail=x;
        Q->head=x;
    }
    else{ //the queue is not empty. adding the new node to the end of the queue.
        Q->tail->next=x;
        Q->tail=x;
    }

    Q->size++;

    pthread_cond_signal(&cond); //notify some other sleeping thread to wake up and continue its work.
    pthread_mutex_unlock(&qlock);
}

node* dequeue(queue* Q)
{
    node *x;
    pthread_mutex_lock(&qlock);
    counter--; //i'm about to go to sleep!
    while (Q->size==0){ //the queue is empty!
        if (counter==0) //theres no more awake threads & the queue is empty
        {
            pthread_cond_signal(&cond); //signal the other sleeping threads to wake up in order to exit.
            pthread_mutex_unlock(&qlock);
            return NULL;
        }
        pthread_cond_wait(&cond, &qlock); //otherwise, theres some awake thread. wait for the queue to be not empty.
    }
    counter++; //the thread is awake, thus the number of awake threads need to increase by one.
    if (Q->size==0){ //this thread woke up because of all other threads are also waiting and thers no more directories
        return NULL;
    }

    x=Q->head; //we will return the first node
    if(Q->size==1){ //theres only one element in the queue, that after the removal the queue will be empty.
        Q->tail=NULL;
        Q->head=NULL;
    }
    else{ //there are more nodes in the queue
        Q->head=Q->head->next;
    }
    Q->size--;
    pthread_mutex_unlock(&qlock);
    return x;
}

node* create_node(char* directory)
{
    node *x;
    x = malloc(sizeof(node));
    if (x==NULL){
        return NULL;
    }
    x->directory=directory;
    x->next=NULL;
    return x;
}

queue* create_queue(node* x)
{
    queue *Q;
    Q = malloc(sizeof(queue));
    if (Q==NULL){
        return NULL;
    }
    Q->head=x;
    Q->tail=x;
    Q->size=1;
    return Q;
}

int Iterate(queue* Q, char* directory, char* term)
{
    DIR *dir;
    struct dirent *entry;
    char *abspath;
    struct stat *buf;
    dir = opendir(directory);
    if (dir == NULL)
    {
        if (errno==EACCES){ //we cant open the directory because we dont have access to it.
            printf("Directory %s: Permission denied.\n", directory);
            return 0;
        }

        fprintf(stderr,"Unable to read directory.\n");
        return 1;
    }

    int is_dir;

    buf = malloc(sizeof(struct stat));
    if (buf==NULL){
        return 1;
    }
    while ((entry = readdir(dir))) //as long there are still dir-entries we didnt scan.
    {
        abspath=abs_path(directory,entry);
        if (abspath==NULL){
            return 1;
        }
        lstat(abspath, buf);
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            is_dir=S_ISDIR(buf->st_mode); //lets check if the entry is another dir
            if (is_dir){ //it is a dir
                if (buf->st_mode & S_IXUSR && buf->st_mode & S_IRUSR){ //the dir is searchble
                    node* y=create_node(abspath);
                    if (y==NULL){
                        return 1;
                    }
                    enqueue(Q ,y); //add the new dir to the queue
                }else{ //the dir is not searchble
                    printf("Directory %s: Permission denied.\n", abspath);
                }

            }else{ //the entry is a file
                if (strstr(entry->d_name,term)!=NULL){ //the term is part of the file name
                    printf("%s\n",abspath);
                    num_of_files++;
                }
            }
        }
    }
    closedir(dir);
    free(buf);
    return 0;
}

void* thread_search(void *arg)
{
    queue* Q;
    char* term;
    int flag, num_of_threads;
    Q=((argument*)arg)->Q;
    term=((argument*)arg)->term;
    num_of_threads=((argument*)arg)->num_of_threads;

    initiated_threads++;
    pthread_mutex_lock(&start);

    while(initiated_threads < num_of_threads){ //wait for all the other threads to be initiate
        pthread_cond_wait(&cond_start, &start);

    }
    pthread_cond_broadcast(&cond_start); //all the other threads initiated. signal everybody to start.
    pthread_mutex_unlock(&start);

    counter++;
    node *x;
    while(1){
        x=dequeue(Q);
        if (x==NULL){
            pthread_exit((void*)0);
        }
        flag=Iterate(Q,x->directory,term);
        free(x);
        if (flag!=0){ //there was some problem during the iteration
            fprintf(stderr,"error occured in a thread.\n");
            error=1;
            counter--;
            pthread_cond_signal(&cond);
            pthread_exit((void*)1);
        }
    }
}

int is_not_searchable(char* directory){ //this function checks if the root dir is not searchable
    struct stat buf;
    if (lstat(directory, &buf) != 0){
        return 1;
    }
    if (!(buf.st_mode & S_IXUSR && buf.st_mode & S_IRUSR)){
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char* directory=argv[1];
    char* search_term=argv[2];
    int thread_num=atoi(argv[3]);
    if (argc!=4){
        fprintf(stderr, "invalid number of arguments. \n");
        exit(1);
    }
    if(is_not_searchable(directory)){
        fprintf(stderr, "%s is not a searchable directory! . \n", directory);
        exit(1);
    }
    node* x;
    queue* Q;
    int i, rc;
    pthread_t *threads;
    argument* arg;

    x=create_node(directory);
    if (x==NULL){
        fprintf(stderr, "problem with memory allocation in main thread.\n");
        exit(1);
    }
    Q=create_queue(x);
    if (Q==NULL){
        fprintf(stderr , "problem with memory allocation in main thread.\n");
        exit(1);
    }
    threads=malloc(sizeof(pthread_t)*thread_num);
    if (threads==NULL){
        fprintf(stderr, "problem with memory allocation in main thread.\n");
        exit(1);
    }
    arg=malloc(sizeof(argument));
    if (arg==NULL){
        fprintf(stderr, "problem with memory allocation in main thread.\n");
        exit(1);
    }
    pthread_mutex_init(&qlock, NULL);
    pthread_mutex_init(&start, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&cond_start, NULL);

    pthread_mutex_lock(&start);
    arg->Q=Q;
    arg->term=search_term;
    arg->num_of_threads=thread_num;
    for (i=0; i<thread_num;i++){
        rc = pthread_create(&threads[i], NULL, thread_search, (void*)(arg));
        if (rc) {
            fprintf(stderr,"ERROR in pthread_create(): the %d thread could not be created.\n", i);
            exit(1);
        }
    }

    pthread_cond_broadcast(&cond_start);
    pthread_mutex_unlock(&start);


    for (i=0; i<thread_num;i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Done searching, found %d files\n", num_of_files);
    free(Q);

    pthread_mutex_destroy(&qlock);
    pthread_mutex_destroy(&start);
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&cond_start);

    if (error){
        exit(1);
    }
    exit(0);
}