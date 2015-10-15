//
//  stack.c
//  lab 1a
//
//  Created by Kevin Huynh on 4/7/15.
//  Copyright (c) 2015 Kevin Huynh. All rights reserved.
//

#include "stack.h"
#include "command.h"
const int INITIAL_ARRAY_SIZE = 20;

void initializeStack(struct stack* item)
{
    item->top = 0;
    item->inputtop = 0;
    item->outputtop = 0;
    item->secondtop = 0;
    item->thirdtop = 0;
    item->size = 20;
    item->secondsize = 20;
    item->thirdsize = 20;
    item->inputsize = 0;
    item->outputsize = 0;
    item->actualstack = checked_malloc(INITIAL_ARRAY_SIZE * sizeof(struct command));
}

void Stackpush(struct stack* array, char item)
{
    array->secondtop = 0;
    array->actualstack[array->top].status = -1;
    if (array->top == array->size - 1)
    {
        array->size = array->size + INITIAL_ARRAY_SIZE;
        array->actualstack = checked_realloc(array->actualstack, array->size * sizeof(char));
    }
    if (item == 'A')
    {
        array->actualstack[array->top].type = AND_COMMAND;
    }
    if (item == 'O')
    {
        array->actualstack[array->top].type = OR_COMMAND;
    }
    if (item == ';')
    {
        array->actualstack[array->top].type = SEQUENCE_COMMAND;
    }
    if (item == '|')
    {
        array->actualstack[array->top].type = PIPE_COMMAND;
    }
    if (item == '(')
    {
        array->actualstack[array->top].type = SUBSHELL_COMMAND;
        array->actualstack[array->top].u.subshell_command = checked_malloc(sizeof(struct command));
    }
    if (item == 'S')
    {
        array->actualstack[array->top].type = SIMPLE_COMMAND;
    }
    array->top++;
    
    
    return;
}

void Stackcommandpush(struct stack* array, char item, int partofpreviousword)
{
    if (partofpreviousword == 1)
    {
        if (array->thirdtop == array->thirdsize - 1)
        {
            array->thirdtop = array->thirdtop + INITIAL_ARRAY_SIZE;
        array->actualstack[array->top - 1].u.word[array->secondtop] = (char*)checked_realloc(array->actualstack[array->top - 1].u.word[array->secondtop], array->thirdsize * sizeof(char));
        }
            array->actualstack[array->top - 1].u.word[array->secondtop][array->thirdtop] = item;
        array->thirdtop++;
        return;
    }
    
    array->actualstack[array->top - 1].u.word = (char**)checked_malloc(INITIAL_ARRAY_SIZE * sizeof(char*));
    array->actualstack[array->top - 1].u.word[array->secondtop] = (char*)checked_malloc(INITIAL_ARRAY_SIZE * sizeof(char));
    array->actualstack[array->top - 1].u.word[array->secondtop][array->thirdtop] = item;
    array->thirdtop++;
}

void Stackinputpush(struct stack* array, char item)
{
    if (array->inputtop == 0)
    {
    array->actualstack[array->top - 1].input = checked_malloc(INITIAL_ARRAY_SIZE * sizeof(char));
    }
    
    if (array->inputtop + 1 == array->inputsize - 1)
    {
        array->inputsize = array->inputsize + INITIAL_ARRAY_SIZE;
        array->actualstack[array->top - 1].input = checked_realloc(array->actualstack[array->top - 1].input ,array->inputsize * sizeof(char));
    }

    array->actualstack[array->top - 1].input[array->inputtop] = item;
    array->actualstack[array->top - 1].input[array->inputtop + 1] = '\0';
    array->inputtop++;
}

void Stackoutputpush(struct stack* array, char item)
{
    if (array->outputtop == 0)
    {
        array->actualstack[array->top - 1].output = checked_malloc(INITIAL_ARRAY_SIZE * sizeof(char));
    }
    
    if (array->outputtop + 1 == array->outputsize - 1)
    {
        array->outputsize = array->outputsize + INITIAL_ARRAY_SIZE;
        array->actualstack[array->top - 1].output = checked_realloc(array->actualstack[array->top - 1].output ,array->outputsize * sizeof(char));
    }
    
    array->actualstack[array->top - 1].output[array->outputtop] = item;
    
    array->actualstack[array->top - 1].output[array->outputtop + 1] = '\0';
    array->outputtop++;
}

void incrementstackword(struct stack* item, int endofword)
{
    item->actualstack[item->top - 1].u.word[item->secondtop][item->thirdtop] = '\0';
    item->secondtop++;
    //chekced_realloc for secondtop
    if (item->secondtop == item->secondsize - 1)
    {
        item->secondsize = item->secondsize + INITIAL_ARRAY_SIZE;
        item->actualstack[item->top - 1].u.word = (char**)checked_realloc(item->actualstack[item->top - 1].u.word, item->secondsize * sizeof(char*));
    }
    if (endofword == 0)
    {
    item->actualstack[item->top - 1].u.word[item->secondtop] = (char*)checked_malloc(INITIAL_ARRAY_SIZE * sizeof(char));
    }
    item->thirdtop = 0;
}

void Stackpop(struct stack* item)
 {
     item->actualstack[item->top - 1].input = NULL;
     item->actualstack[item->top - 1].output = NULL;
item->top--;
 }


void resetinputoutput(struct stack* array, int inputoroutput)
{
    if (inputoroutput == 1)
    {
        array->inputtop = 0;
        return;
    }
    array->outputtop = 0;
}

struct command* Stacktop(struct stack* item)
 {
     if (item->top == 0)
     {
         return &item->actualstack[item->top];
     }
 return &item->actualstack[item->top - 1];
 }
 
 int Stackisempty(struct stack* item)
 {
     if(item->top == 0)
     {
        return 1;
     }
    return 0;
 }
