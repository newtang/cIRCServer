#include <stdbool.h>
#include <string.h>

bool isWhiteSpace(char c){
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

char* parseCommand(char *str){
    int i =0;

    char *end = str + strlen(str) - 1;
    while(end > str && isWhiteSpace((unsigned char)*end)){
        end--;
    } 
    // Write new null terminator
    *(end+1) = 0;


    while(str[i] != '\0'){
        if(str[i] == ' '){
            str[i] = '\0';
            return &str[i+1];
        }
        ++i;
    }
}
