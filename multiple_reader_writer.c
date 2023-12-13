#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define INPUT_LENGTH 64
#define MAX_COMMAND 64

typedef struct
{
   char *filename;
   sem_t  writeLock,readLock;
   int numReaders,numWriters;
   // semt_t rmutex,wmutex;
} fileInfo;


int readerCount = 0, writerCount = 0;
sem_t rmutex,wmutex,fileLock;
fileInfo *files[200];
int totalFiles=0;

void initialize()
{   
   sem_init(&rmutex,0,1); // to prevent race condition for readerCount ;
   sem_init(&wmutex,0,1); // to prevent race condition for  writerCount;
   sem_init(&fileLock,0,1);
   for (int i = 0; i < 200; i++)
   {  
    fileInfo *file_info = (fileInfo*)malloc(sizeof(fileInfo));
    file_info->filename = NULL;
    sem_init(&file_info->writeLock, 0, 1);
    sem_init(&file_info->readLock, 0, 1);
    file_info->numReaders=0;
    file_info->numWriters=0;
    files[i] = file_info;
   }
}
void copyCommand(char **commands_copy,char **commands){
    int i=0;
   for (i = 0; i < MAX_COMMAND && commands[i] != NULL; i++)
    {
        commands_copy[i] = strdup(commands[i]);
    }
    commands_copy[i] = NULL;
}
int findIndex(char *filename)
{
   for (int i = 0; i < totalFiles; i++)
   {
      if (files[i]->filename!=NULL && strcmp(files[i]->filename, filename) == 0)
      {
         return i;
      }
   }
   return -1;
}
void parseInput(char *str, char **cmd)
{
   int commands = 0;
   char *token = strtok(str, " ");
   while (token != NULL && commands < MAX_COMMAND)
   {
      cmd[commands++] = token;
      token = strtok(NULL, " ");
   }
   cmd[commands] = NULL;
}

void concatenate(char **cmd,char *str){
     int i=0,count=0;
     while(count<3){
        if(str[i]==' '){
            count++;
            while(str[i]==' '){
                i++;
            }
        }
        else{
            i++;
        }
     }
     char content[100];
     int j=0;
     while(str[i]!='\0'){
        content[j++]=str[i++];
     }
     content[j]='\0';
     strcpy(cmd[3],content);
     for(int j=4;cmd[j]!=NULL;j++){
        cmd[j]=NULL;
     }
}
void *readFile(void *filename)
{
  char *file = (char *)filename;
  int index= findIndex(file);
  if(index==-1){
   index=totalFiles;
   sem_wait(&fileLock);
   files[totalFiles++]->filename=strdup(file);
   sem_post(&fileLock);
  }
  fileInfo* file_info =files[index];
  sem_wait(&file_info->readLock); // should it be here or after releasing the rmutex ??
  sem_wait(&rmutex);
  readerCount++;
  file_info->numReaders++;
  if(file_info->numReaders==1){
   sem_wait(&file_info->writeLock);
  }
  sem_post(&rmutex);
  sem_post(&file_info->readLock); // should it be here or after reading the bytes?
  
  // print the line number of bytes read....
  FILE *f=fopen(file,"r");
  if (f == NULL) {
    printf("Error opening file for reading: %s\n", file);
    return NULL;
  }

  int ch;
  size_t bytesRead = 0;
  while ((ch = fgetc(f)) != EOF) {
    bytesRead++;
  }

  fclose(f);
  printf("Read %s of %zu bytes with %d readers and %d writers present\n",file, bytesRead, readerCount, writerCount);
  
  sem_wait(&rmutex);
  readerCount--;
  file_info->numReaders--;
  if(file_info->numReaders==0){
   sem_post(&file_info->writeLock);
  }
  sem_post(&rmutex);

}

void *writeFile1(void *command)
{
    char **commands= (char **)command;
    char* writeTo= strdup(commands[2]);
    char *writeFrom=strdup(commands[3]);
    int dstIndex=findIndex(writeTo);
    int srcIndex=findIndex(writeFrom);
    if(dstIndex==-1){
      sem_wait(&fileLock);
      dstIndex=totalFiles;
      files[totalFiles++]->filename=strdup(writeTo);
      sem_post(&fileLock);
    }
    if(srcIndex==-1){
      sem_wait(&fileLock);
      srcIndex=totalFiles;
      files[totalFiles++]->filename=strdup(writeFrom);
      sem_post(&fileLock);
   }
   fileInfo* srcInfo =files[srcIndex];
   fileInfo* dstInfo =files[dstIndex];
  
    // Acquire destination file lock first to provide writer preference
    sem_wait(&wmutex);
    writerCount++;
    dstInfo->numWriters++;

    if (dstInfo->numWriters == 1) {
        sem_wait(&dstInfo->readLock); // Block readers when a writer is active on the destination file
    }
    sem_post(&wmutex);

    // // Acquire source file lock
    sem_wait(&dstInfo->writeLock);
    sem_wait(&srcInfo->readLock);

    // Perform the file content copy from source to destination
    FILE *srcFile = fopen(writeFrom, "r");
    if (srcFile == NULL) {
        printf("Error opening source file for reading: %s\n", writeFrom);
        return NULL;
    } 
   FILE *dstFile = fopen(writeTo, "a"); // Open destination file in write mode to start fresh
   if (dstFile == NULL) {
      printf("Error opening destination file for writing: %s\n", writeTo);
      return NULL;
   } 
   
   char buffer[100];
   size_t bytesWrite = 0;
   fseek(dstFile, 0, SEEK_END);
   if (ftell(dstFile) != 0) {
      fputs("\n", dstFile);
      bytesWrite++;
   }
   
   while (fgets(buffer, 100, srcFile) != NULL) {
         fputs(buffer, dstFile);
         bytesWrite += strlen(buffer);
   }

   fclose(dstFile);
   fclose(srcFile);
    
   printf("Writing to %s added %zu bytes with %d readers and %d writers present\n",writeTo, bytesWrite, readerCount, writerCount);
   
    sem_post(&srcInfo->readLock);
    sem_post(&dstInfo->writeLock);

    sem_wait(&wmutex);
    writerCount--;
    dstInfo->numWriters--;
    if (dstInfo->numWriters == 0) {
        sem_post(&dstInfo->readLock); // Allow readers when there are no writers on the destination file
    }
    sem_post(&wmutex);

}
void *writeFile2(void *command)
{
   char **commands=(char **)command;
   char *writeTo= commands[2];
   int index= findIndex(writeTo);
   if(index==-1){
      sem_wait(&fileLock);
      index=totalFiles;
      files[totalFiles++]->filename=strdup(writeTo);
      sem_post(&fileLock);   
   }
   fileInfo* file_info =files[index];
   sem_wait(&wmutex);
   writerCount++;
   file_info->numWriters++;
   if(file_info->numWriters==1){
      sem_wait(&file_info->readLock);
   }
   sem_post(&wmutex);
   sem_wait(&file_info->writeLock);
  
   FILE* dstFile=fopen(writeTo,"a");
   if (dstFile == NULL) {
        printf("Error opening file for reading: %s\n", writeTo);
        return NULL;
   }
   
   size_t bytesWrite = 0;
   fseek(dstFile, 0, SEEK_END);
   if (ftell(dstFile) != 0) {
      fputs("\n", dstFile);
      bytesWrite++;
   }

   fputs(commands[3],dstFile);
   fclose(dstFile);
   bytesWrite+=strlen(commands[3]);
   printf("Writing to %s added %zu bytes with %d readers and %d writers present\n",writeTo, bytesWrite, readerCount, writerCount);

   sem_post(&file_info->writeLock);
   sem_wait(&wmutex);
   writerCount--;
   file_info->numWriters--;
   if(file_info->numWriters==0){
      sem_post(&file_info->readLock);
   }
   sem_post(&wmutex);

}
int main()
{

   initialize();
   char *input = (char*)malloc(200);
   char *commands[MAX_COMMAND];
   pthread_t *threads = (pthread_t*) malloc(sizeof(pthread_t) * 200);
   int threadCount = 0;
   while (1)
   {
      fgets(input, 200, stdin);
      size_t input_length = strcspn(input, "\n\r");
      input[input_length] = '\0';
      char *copy=strdup(input);
      parseInput(copy, commands);

      if (strcmp(commands[0], "exit") == 0)
      {
         break;
      }

      //  check if it is write operation or read operation by comparing the first char
      pthread_t thread;
      if (strcmp(commands[0], "read") == 0)
      {  
            char **commands_copy = malloc(sizeof(char *) * (MAX_COMMAND + 1));
            copyCommand(commands_copy,commands);
            pthread_create(&thread, NULL, readFile, (void *)commands_copy[1]);

      }
      else if (strcmp(commands[0], "write") == 0)
      {
         if (strcmp(commands[1], "1") == 0)
         {  
            char **commands_copy = malloc(sizeof(char *) * (MAX_COMMAND + 1));
            copyCommand(commands_copy,commands);
            pthread_create(&thread, NULL, writeFile1, (void *)commands_copy);

         }
         else
         { 
            char **commands_copy = malloc(sizeof(char *) * (MAX_COMMAND + 1));
            copyCommand(commands_copy,commands);
            concatenate(commands_copy,input);
            pthread_create(&thread, NULL, writeFile2, (void *)commands_copy);
         }
      }
      else
      {
         printf("Invalid input\n");
         continue;
      }
      threads[threadCount++] = thread;
   }
   for (int i = 0; i < threadCount; i++)
   {
      pthread_join(threads[i], NULL);
   }

   // Free the thread pool
   free(threads);
   free(input);
   // Free the file info structs
   for (int i = 0; i < 200; i++)
   {
      free(files[i]);
   }
   sem_destroy(&rmutex);
   sem_destroy(&wmutex);
   sem_destroy(&fileLock);
   return 0;
}
