#include<stdio.h>
#include<string.h>

#define MAX_INPUT 1024

int main(){
    char input[MAX_INPUT];
    while(1){
        printf("tsh>");
        fflush(stdout); // make sure prompt appears

        //1. Read a line
        if(fgets(input,MAX_INPUT,stdin)==NULL){
            perror("fgets failed");
            continue;
        }

        input[strcspn(input,"\n")] = '\0'; // Remove trailing '\n'

        // 2. Built-in: exit
        if(strcmp(input,"exit")==0){
            printf("Byeee!!!");
            printf("\n");
            break;
        }
        printf("%s\n",input);
    }
}