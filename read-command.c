// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include "stack.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const int INITIAL_TOKEN_SIZE = 50;
const int CHAR_EMPTY = -2;

static int count = 0;

void copy(struct command *source, struct command* destination)
{
    memcpy(destination, source, sizeof(struct command));
}

void Stackcheckpop (struct stack* item, int linenumber)
{
    if( Stackisempty(item) == 1)
    {
        fprintf(stderr, "linenumber %d: syntax error", linenumber);
        exit(1);
    }
    Stackpop(item);
}



static char fpeek(void *stream)
{
    int c;
    
    c = fgetc(stream);
    c =ungetc(c, stream);
    
    return (char)c;
}

struct command_stream
{
    command_t item;
    command_stream_t next;
};




int is_valid_char(char c)
{
    if ((c >= '0' && c <= '9')
    || (c >= 'A' && c <= 'Z')
    || (c >= 'a' && c <= 'z')
    || c == '!'
    || c == '%' || c == '+'
    || c == ',' || c == '-'
    || c == '.' || c == '/'
    || c == ':' || c == '@'
    || c == '^' || c == '_')
    {
        return 1;
    }
    return 0;
}


command_stream_t make_command_stream (int (*getNextByte) (void *),
                                      void *getNextByte_argument    )
{
    command_stream_t head;
    head = checked_malloc(sizeof(struct command_stream));
    head->item = NULL;
    head->next = NULL;
    command_stream_t pointer;
    pointer = checked_malloc(sizeof(struct command_stream));
    pointer = head;
    struct stack operatorstack, commandstack;
    initializeStack(&operatorstack);
    initializeStack(&commandstack);
    head->item = &commandstack.actualstack[0];
    int linenumber = 1;
    int partofpreviousword = 0;
    int partofinput = 0;
    int partofoutput = 0;
    int endofword = 0;
    int numberofnewlinesinarow = 0;
    int restinginput = 0;
    int new = 0;
    int dontcountnewlinebecauseofandoror = 0;
    int e;
    char c;
    char d;
    while((e = getNextByte(getNextByte_argument)) != -1)
        
        //A = AND COMMAND, S = SIMPLE COMMAND, O = OR COMMAND, ; = SEQUENCE COMMAND, | = PIPELINE COMMAND, ( = SUBSHELL COMMAND,
    {
        c = (char) e;
        if(c == '\n')
        {
            restinginput = 0;
            partofpreviousword = 0;
            if (partofinput == 1 || partofoutput == 1)
            {
                
                fprintf(stderr, "linenumber %d: syntax error", linenumber);
                exit(1);
            }
            numberofnewlinesinarow++;
            if (numberofnewlinesinarow == 1 && dontcountnewlinebecauseofandoror == 0)
            {
                while(1)
                {
                    d = getNextByte(getNextByte_argument);
                    if (d == '&' || d == '|' || d == '(' || d == ')' || d == '<' || d == '>' || d == ';')
                    {
                        ungetc(d, getNextByte_argument);
                        fprintf(stderr, "linenumber %d: syntax error", linenumber);
                        exit(1);
                        break;
                    }
                    if (d == -1)
                    {
                        new++;
                        break;
                    }
                    if (d == '\n')
                    {
                        ungetc(d, getNextByte_argument);
                        new++;
                        break;
                    }
                    if (is_valid_char(d))
                    {
                        ungetc(d, getNextByte_argument);
                        break;
                    }
                }
                if( new == 0)
                {
                c = ';'; // change \n to ;
                }
            }
            linenumber++;
            if (numberofnewlinesinarow == 2 && dontcountnewlinebecauseofandoror == 0)
            {
                while (Stackisempty(&operatorstack) == 0)
                {
                    if (Stacktop(&operatorstack)->type == SUBSHELL_COMMAND)
                    {
                        copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.subshell_command);
                        Stackcheckpop(&commandstack, linenumber);
                        Stackcheckpop(&commandstack, linenumber);
                    }
                    else
                    {
                        operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
                        operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
                        copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
                        Stackcheckpop(&commandstack, linenumber);
                        copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
                        Stackcheckpop(&commandstack, linenumber);
                    }
                    Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                    copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                    Stackcheckpop(&operatorstack, linenumber);
                }
                pointer->item = malloc(sizeof(struct command));
                numberofnewlinesinarow = 0;
                pointer->next = malloc(sizeof(struct command_stream));
                copy(Stacktop(&commandstack), pointer->item);
                Stackcheckpop(&commandstack, linenumber);
                pointer = pointer->next;
                pointer->item = &commandstack.actualstack[0];
                pointer->next = NULL;
                partofpreviousword = 0;
                partofinput = 0;
                partofoutput = 0;
                endofword = 0;
                numberofnewlinesinarow = 0;
                new = 0;
                dontcountnewlinebecauseofandoror = 0;
            }
            if (c != ';')
            {
            continue;
            }
        }
        if(c == '#')
        {
            while(getNextByte(getNextByte_argument) != '\n')
            {}
            linenumber++;
            numberofnewlinesinarow++;
            continue;
        }
        if(c == ' ')
        {
            if (fpeek(getNextByte_argument) == ' ')
            {
                continue;
            }
            if (fpeek(getNextByte_argument) != '\n')
            {
                numberofnewlinesinarow = 0;
            }
            if (is_valid_char(fpeek(getNextByte_argument)) == 0 && fpeek(getNextByte_argument) != '<' && fpeek(getNextByte_argument) != '>')
                {
                    partofpreviousword = 0;
                }
            continue;
        }
        if(is_valid_char(c) == 1)
        {
            restinginput = 0;
            numberofnewlinesinarow = 0;
            dontcountnewlinebecauseofandoror = 0;
            if (partofinput == 1)
            {
                Stackinputpush(&commandstack, c);
                if (is_valid_char(fpeek(getNextByte_argument)) == 0)
                {
                    partofinput = 0;
                    restinginput++;
                    resetinputoutput(&commandstack, 1);
                }
                continue;
            }
            if (partofoutput == 1)
            {
                Stackoutputpush(&commandstack, c);
                if (is_valid_char(fpeek(getNextByte_argument)) == 0)
                {
                    partofoutput = 0;
                    resetinputoutput(&commandstack, 0);
                }
                continue;
            }
            if (partofpreviousword == 0)
            {
            Stackpush(&commandstack, 'S');
            }
            Stackcommandpush(&commandstack, c, partofpreviousword);
            if (is_valid_char(fpeek(getNextByte_argument)) == 0 || (int)fpeek(getNextByte_argument) == -1)
            {
	      while(1)
		{
		  d = getNextByte(getNextByte_argument);
		  if (d != ' ')
		    {
		      ungetc(d, getNextByte_argument);
		      break;
		    }
		}
                partofinput = 0;
                partofoutput = 0;
                if (d == '&' || d == '|' || d == '(' || d == ')' || d == '<' || d == '>' || d == '\n')
                {
                    endofword = 1;
                }
                incrementstackword(&commandstack, endofword);
                endofword = 0;
            }
            partofpreviousword = 1;
            continue;
        }
        if (partofpreviousword == 0 && (c == '<' || c == '>') && restinginput == 0)
        {
            fprintf(stderr, "linenumber %d: syntax error", linenumber);
            exit(1);
        }
        if(is_valid_char(c) == 0)
            {
                restinginput = 0;
                partofpreviousword = 0;
                partofinput = 0;
                numberofnewlinesinarow = 0;
                partofoutput = 0;
                char d = fpeek(getNextByte_argument);
		if (c == '&' && c != d)
		  {
		    fprintf(stderr, "linenumber %d: syntax error", linenumber);
		    exit(1);
		  }                
		if (c == ';')
                {
                    dontcountnewlinebecauseofandoror++;
                }
                if (c == '|' && d != c)
                {
                    dontcountnewlinebecauseofandoror++;
                }
                if (d == c && d == '&')
                {
                    c = 'A';
                    dontcountnewlinebecauseofandoror++;
                    getNextByte(getNextByte_argument);
                }
                else if (d == c && d == '|')
                {
                    c = 'O';
                    dontcountnewlinebecauseofandoror++;
                    getNextByte(getNextByte_argument);
                }
                if (c == '(')
                {
                    Stackpush(&operatorstack, c);
                    continue;
                }
                if (c == '<')
                {
                    if (dontcountnewlinebecauseofandoror > 0)
                    {
                        fprintf(stderr, "linenumber %d: syntax error", linenumber);
                        exit(1);
                    }
                    partofinput = 1;
                    continue;
                }
                
                if (c == '>')
                {
                    if (dontcountnewlinebecauseofandoror == 1)
                    {
                        fprintf(stderr, "linenumber %d: syntax error", linenumber);
                        exit(1);
                    }
                    partofoutput = 1;
                    continue;
                }
                if (Stackisempty(&operatorstack) == 0)
                {
                    if (c == ')')
                    {
                        while (Stacktop(&operatorstack)->type != SUBSHELL_COMMAND && Stackisempty(&operatorstack) == 0)
                        {
                            operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
                            operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
                            Stackcheckpop(&commandstack, linenumber);
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
                            Stackcheckpop(&commandstack, linenumber);
                            Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                            copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                            Stackcheckpop(&operatorstack, linenumber);
                        }
                        if (Stackisempty(&operatorstack))
                        {
                            fprintf(stderr, "linenumber %d: syntax error", linenumber);
                            exit(1);
                        }
                        copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.subshell_command);
                        Stackcheckpop(&commandstack, linenumber);
                        Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                        copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                        Stackcheckpop(&operatorstack, linenumber);
                        continue;
                        
                    }
                    if (c == ';')
                    {
                        while (Stacktop(&operatorstack)->type != SUBSHELL_COMMAND && Stackisempty(&operatorstack) == 0)
                        {
                            operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
                            operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
                            Stackcheckpop(&commandstack, linenumber);
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
                            Stackcheckpop(&commandstack, linenumber);
                            Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                            copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                            Stackcheckpop(&operatorstack, linenumber);
                        }
                        Stackpush(&operatorstack, c);
                    }
                    
                    if (c == 'A'  || c == 'O')
                    {
                        while (Stacktop(&operatorstack)->type != SUBSHELL_COMMAND && Stacktop(&operatorstack)->type != SEQUENCE_COMMAND && Stackisempty(&operatorstack) == 0)
                        {
                            operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
                            operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
                            Stackcheckpop(&commandstack, linenumber);
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
                            Stackcheckpop(&commandstack, linenumber);
                            Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                            copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                            Stackcheckpop(&operatorstack, linenumber);
                        }
                        Stackpush(&operatorstack, c);
                    }
                    if (c == '|')
                    {
                        while (Stacktop(&operatorstack)->type != AND_COMMAND && Stacktop(&operatorstack)->type != OR_COMMAND && Stacktop(&operatorstack)->type != SUBSHELL_COMMAND && Stacktop(&operatorstack)->type != SEQUENCE_COMMAND && Stackisempty(&operatorstack) == 0)
                        {
                            operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
                            operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
                            Stackcheckpop(&commandstack, linenumber);
                            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
                            Stackcheckpop(&commandstack, linenumber);
                            Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
                            copy(Stacktop(&operatorstack), Stacktop(&commandstack));
                            Stackcheckpop(&operatorstack, linenumber);
                        }
                        Stackpush(&operatorstack, c);
                    }
                    
                
                }
                if (Stackisempty(&operatorstack) == 1)
                {
                    Stackpush(&operatorstack, c);
                }
            }

    }
    
    while (Stackisempty(&operatorstack) == 0)
    {
            if (Stacktop(&operatorstack)->type == SUBSHELL_COMMAND)
            {
                copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.subshell_command);
                Stackcheckpop(&commandstack, linenumber);
                Stackcheckpop(&commandstack, linenumber);
            }
        else
        {
            operatorstack.actualstack[operatorstack.top - 1].u.command[1] = checked_malloc(sizeof(struct command));
            operatorstack.actualstack[operatorstack.top - 1].u.command[0] = checked_malloc(sizeof(struct command));
            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[1]);
            Stackcheckpop(&commandstack, linenumber);
            copy(Stacktop(&commandstack), operatorstack.actualstack[operatorstack.top - 1].u.command[0]);
            Stackcheckpop(&commandstack, linenumber);
        }
            Stackpush(&commandstack, operatorstack.actualstack[operatorstack.top - 1].type);
            copy(Stacktop(&operatorstack), Stacktop(&commandstack));
            Stackcheckpop(&operatorstack, linenumber);
    }
    return head;
}

command_t
read_command_stream (command_stream_t s)
{
    int i;
    for (i = 0; i < count; i++)
    {
        s = s->next;
    }
    count++;
    if (s == NULL)
    {
        return NULL;
    }
    return s->item;
}
