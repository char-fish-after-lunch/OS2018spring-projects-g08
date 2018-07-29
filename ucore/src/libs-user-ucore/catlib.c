#include <malloc.h>
#include <stdio.h>
#include <ulib.h>
#include <mult.c>
#include <divmod.c>
#include <string.c>

#define BUFSIZE 1024
//Print String
void  _catlib__PrintString(char * v){
    cprintf("%s", v);
}

//Print Integer
void _catlib__PrintInt(int v){
    cprintf("%d", v);
}
//Print Bool
void _catlib__PrintBool(bool b){
    if (b){
        cprintf("True");
    }else{
        cprintf("False");
    }
}

//Halt
void _catlib__Halt(){
    exit(0);
}


//Read Integer
int _catlib__ReadInteger(){
    char buf[30];
    int p;
    for (p = 0; p < 30; p++){
        buf[p] = 0;
    }
    int i = 0,t;
    char c;
    int temp = 1;
    int ans = 0;
    while(1){
        c = getchar();
        if (c >= '0' && c <= '9'){
            break;
        }
    }
    while(1){
        if (c < '0' || c > '9'){
            break;
        }
        buf[i++] = c;
        c = getchar();
    }
    for(t = i - 1; t >= 0; t--){
        ans += (buf[t] - '0') * temp;
        temp = __mulsi3(temp, 10);
    }
    return ans;
}

//Read String
#define BUFSIZE 32
char* _catlib__ReadLine(){
    char*buffer = malloc(BUFSIZE);
    int ret, i = 0;
    while (1) {
        char c;
        if ((ret = read(0, &c, sizeof(char))) < 0) {
            return NULL;
        } else if (ret == 0) {
            if (i > 0) {
                buffer[i] = '\0';
                break;
            }
            return NULL;
        }
        
        if (c == 3) {
            return NULL;
        } else if (c >= ' ' && i < BUFSIZE - 1) {
            //putc(c);
            buffer[i++] = c;
        } else if (c == '\b' && i > 0) {
            //putc(c);
            i--;
        } else if (c == '\n' || c == '\r') {
            //putc(c);
            buffer[i] = '\0';
            break;
        }
    }
    return buffer;
}

//Multiply
long _catlib__MUL(long a1, long a2){
    return __mulsi3(a1, a2);
}
//Divide
long _catlib__DIV(long a1, long a2){
    return __divsi3(a1,a2);
}
//Mod
long _catlib__MOD(long a1, long a2){
    return __modsi3(a1,a2);
}
//String Compare
bool _catlib__StringEqual(char* a, char *b){
    int ret = strcmp(a, b);
    if (ret == 0){
        return 1;
    }else{
        return 0;
    }
}
// Memory Allocate
void * _catlib__Alloc(unsigned size){
    return malloc(size);
}

