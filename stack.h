//
//  stack.h
//  lab 1a
//
//  Created by Kevin Huynh on 4/7/15.
//  Copyright (c) 2015 Kevin Huynh. All rights reserved.
//

#ifndef __lab_1a__stack__
#define __lab_1a__stack__

#include <stdio.h>
#include "alloc.h"
#include "command-internals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct stack
{
    struct command* actualstack;
    int top;
    int inputtop;
    int outputtop;
    int secondtop; //how many words are there in a simple command
    int thirdtop; //how long the most recent word is being created
    int size;
    int secondsize; //size of how many words possible
    int thirdsize; //size of how long each word is possible
    int inputsize;
    int outputsize;
};

void initializeStack(struct stack* item);
void Stackpush(struct stack* array, char item);
void Stackcommandpush(struct stack* array, char item, int partofpreviousword);
void Stackinputpush(struct stack* array, char item);
void Stackoutputpush(struct stack* array, char item);
void Stackpop(struct stack* item);
struct command* Stacktop(struct stack* item);
void incrementstackword(struct stack* item, int endofword);
void resetinputoutput(struct stack* array, int inputoroutput);
int Stackisempty(struct stack* item);

#endif /* defined(__lab_1a__stack__) */
