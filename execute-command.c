// UCLA CS 111 Lab 1 command execution
#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include "read-command.c"

#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>



/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */


typedef struct dependency_node *dependency_node_t;
typedef struct dependency_graph *dependency_graph_t;

int debugmode = 0;

struct dependency_node
{
    command_t c;
    dependency_node_t* b4;
    size_t b4_size;
    size_t b4_mem;
    
    dependency_node_t* after;
    size_t aft_size;
    size_t aft_mem;
    
    char** read;
    size_t read_size;
    size_t read_mem;
    char** write;
    size_t write_size;
    size_t write_mem;
    char** args;
    size_t arg_size;
    size_t arg_mem;
};

struct dependency_graph
{
    dependency_node_t* no_dep;
    size_t no_dep_size;
    size_t no_dep_mem;
    
    dependency_node_t* dep;
    size_t dep_size;
    size_t dep_mem;
    
    
};

void init_node( dependency_node_t i, command_t c)
{
    i->c = c;
    i->b4_mem = sizeof(dependency_node_t);
    i->b4 = checked_malloc(i->b4_mem);
    i->b4_size = 0;
    i->read = checked_malloc(sizeof(char*)*512);
    i->write = checked_malloc(sizeof(char*)*512);
    i->args = checked_malloc(sizeof(char*)*512);
    
}

void destroy_node(dependency_node_t *n)
{
    if(*n == NULL)
    {return;}
    free((*n)->args);
    free((*n)->write);
    free((*n)->read);
    free((*n)->b4);
    free((*n)->after);
    free(*n);
    *n =NULL;
}

int add_dep_to_graph(dependency_graph_t g, dependency_node_t n, int arr)
{
    if(arr == 0)
    {
        if (g->no_dep_mem<(sizeof(dependency_node_t)* (g->no_dep_size+1))) {
            g->no_dep = checked_grow_alloc(g->no_dep, &(g->no_dep_mem));
        }
        g->no_dep[g->no_dep_size] = n;
        g->no_dep_size++;
    }
    else if (arr == 1)
    {
        if(g->dep_mem<(sizeof(dependency_node_t)* (g->dep_size +1)))
        {
            g->dep = checked_grow_alloc(g->dep, &(g->dep_mem));
        }
        g->dep[g->dep_size] = n;
        g->dep_size++;
        
    }
    else
        return -1;
    return 0;
}

int rm_no_dep_node( dependency_graph_t g, size_t nd)
{
    if(nd >= g->dep_size){return -1;}
    free(g->no_dep[nd]);
    size_t pos;
    for(pos= nd; pos<(g->no_dep_size-1); pos++)
    {
        g->no_dep[pos] = g->no_dep[pos+1];
    }
    g->no_dep_size--;
    
    return 0;
}

// Moves a node from the array of executable nodes to the array of
//	nodes with dependencies
int move_e_to_d (dependency_graph_t d, size_t epos)
{
    if (epos >= d->no_dep_size) { return -1; }
    
    int err = add_dep_to_graph (d, d->no_dep[epos], 1);
    d->dep_size++;
    
    size_t it;
    for (it = epos; it < (d->no_dep_size - 1); it++)
    {
        d->no_dep[it] = d->no_dep[it + 1];
    }
    
    d->dep_size--;
    
    return 0;
}

// Moves a node from the array of nodes with dependencies to the array of
//	independent nodes
int move_d_to_e (dependency_graph_t d, size_t dpos)
{
    if (dpos >= d->dep_size) { return -1; }
    
    int err = add_dep_to_graph (d, d->dep[dpos], 0);
    d->dep_size++;
    
    size_t it;
    //free(d->dep[dpos]);
    for (it = dpos; it < (d->dep_size - 1); it++)
    {
        d->dep[it] = d->dep[it + 1];
    }
    
    d->dep_size--;
    
    return 0;
}


void init_graph(dependency_graph_t d)
{
    d->no_dep = checked_malloc(sizeof(dependency_node_t));
    d->dep = checked_malloc(sizeof(dependency_node_t));
    d->no_dep_mem =sizeof(dependency_node_t);
    d->no_dep_size =0;
    d->dep_mem = sizeof(dependency_node_t);
    d->dep_size = 0;
}

int not_enough_mem(void* ptr, size_t* sz, size_t obSize, size_t mem)
{
    if (obSize * (*sz + 1) > mem)
    {
        return 1;
    }
    return 0;
}
void process_command(command_t c, char** arg, size_t* size, size_t* mem)
{
    char** w;
    switch (c->type){
        case SEQUENCE_COMMAND:
            process_command(c->u.command[0], arg, size, mem);
            process_command(c->u.command[1], arg, size, mem);
            break;
        case SIMPLE_COMMAND:
            if (c->input != NULL)
            {
                if (not_enough_mem (arg, size, sizeof (char*), *mem))
                { arg = checked_grow_alloc (arg, mem); }
                arg[*size] = c->input; (*size)++;
            }
            else if( c->output != NULL)
            {
                if (not_enough_mem (arg, size, sizeof (char*), *mem))
                { arg = checked_grow_alloc (arg, mem); }
                arg[*size] = c->output; (*size)++;
            }

            w=&(c->u.word[1]);
            while(*++w)
            {
                if (not_enough_mem (arg, size, sizeof (char*), *mem))
                { arg = checked_grow_alloc (arg, mem); }
                arg[*size] = *w; (*size)++;

            }
            break;
        case SUBSHELL_COMMAND:
            if (c->input != NULL)
            {
                if (not_enough_mem (arg, size, sizeof (char*), *mem))
                { arg = checked_grow_alloc (arg, mem); }
                arg[*size] = c->input; (*size)++;
            }
            else if (c->output != NULL)
            {
                if (not_enough_mem (arg, size, sizeof (char*), *mem))
                { arg = checked_grow_alloc (arg, mem); }
                arg[*size] = c->output; (*size)++;

            }
            process_command (c->u.subshell_command, arg, size, mem);
            break;
        default: 
            process_command(c->u.command[0], arg, size, mem);
            process_command(c->u.command[1], arg, size, mem);
            break;   
    }
    return;
}

void inorderPrint( command_t root, int* order, struct command array[] )
{
    // Print all the items in the tree to which root points.
    // The items in the left subtree are printed first, followed
    // by the item in the root node, followed by the items in
    // the right subtree.
    if ( root != NULL ) {  // (Otherwise, there's nothing to print.)
        inorderPrint( root->u.command[0], order, array);    // Print items in left subtree.
        array[*order] = *root;// Print the root item.
        order++;
        inorderPrint( root->u.command[1], order, array);
        // Print items in right subtree.
    }
} // end inorderPrint()

dependency_graph_t build_graph (command_stream_t s)
{
    dependency_graph_t ret_d;
    ret_d = NULL;
    size_t num;
    num = 0;
    size_t iter = 0;
    command_t comm;
    ret_d =checked_malloc(sizeof(struct dependency_graph));
    init_graph(ret_d);
    
    char**args;
    size_t a_size;
    size_t a_mem;
    a_mem = (sizeof(char*));
    args=checked_malloc(a_mem);
    int size= countNodes(s->item);
    
    char** read;
    size_t r_mem;
    size_t r_size;
    
    char** write;
    size_t w_mem;
    size_t w_size;
    
    size_t position=0;
    size_t innerpos = 0;
    
    for(num = 0; num < size;num++)
    {
        comm = s->cArray[num]; //each command in the array
        dependency_node_t n;
        n = checked_malloc (sizeof (struct dependency_node));
        init_node(n, comm); //make a node to add to the graph
        while(iter < ret_d->no_dep_size || iter < ret_d->dep_size)
        {
            //if (debugmode) { printf ("I AM THERE!"); }
            r_size = 0;
            r_mem = (sizeof(char*));
            read = checked_malloc(r_mem);
            w_mem = (sizeof(char*) * 3);
            w_size = 0;
            write = checked_malloc(w_mem);
            a_size = 0;
            size_t aMem = sizeof (char*);
            args = checked_malloc(aMem);
            //find all the arguments passed on the command line
            process_command(comm, args, &a_size, &aMem);
            process_command(comm, read, &r_size, &r_mem);
            process_command(comm, write, &w_size, &w_mem);
            //set all the node properties
            n->read = read;
            n->read_size = r_size;
            n->read_mem = r_mem;
            n->write = write;
            n->write_size = w_size;
            n->write_mem = w_mem;
            n->args = args;
            //printf("iter: %zu\n", iter);
            //error(1,0,"kid");
            //if(comm->input!= NULL)
            //	printf("%s\n", comm->input);
            //printf ("1");
            
            //if there is a value in the output array and there is a node in the executable array
            //of the graph, then add a dependency to the current node and then push it to the dependancy graph
            if(write != NULL && iter < ret_d->no_dep_size){//printf("O\n");
                while(position < w_size){
                    while(innerpos < ret_d->no_dep[iter]->read_size){
                        if(strcmp(ret_d->no_dep[iter]->read[innerpos], write[position])==0)
                        {
                            if (mem_need_grow (n->b4, &n->b4_size, sizeof(dependency_node_t) , n->b4_mem))
                            { n->b4 = checked_grow_alloc (n->b4, &(n->b4_mem)); }
                            
                            n->b4[n->b4_size] = ret_d->no_dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
         
            
            //if there is a value in the output array and there is a node in the dependancy array
            //of the graph, then add a dependency to the current node and then push it to the dependancy graph, because all commands
            //are parsed sequentially, if a dependancy occurs in the dependancy graph, then it must be added to the
            //dependancy graph
            if(write != NULL && iter < ret_d->dep_size){//printf("O\n");
                while(write[position]!=NULL){//printf("O2\n");
                    while(ret_d->dep[iter]->read[innerpos]!=NULL){
                        if(strcmp(ret_d->dep[iter]->read[innerpos], write[position])==0)
                        {
                            n->b4[n->b4_size] = ret_d->dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
            //printf ("3");
            
            //if there is a input depencdancy and there is a matching value in the dependancy graph
            //push the dependancy for this node
            if(read != NULL && iter < ret_d->dep_size){
                while(read[position]!=NULL){
                    while(ret_d->dep[iter]->write[innerpos]!=NULL){
                        if(strcmp(ret_d->dep[iter]->write[innerpos], read[position])==0)
                        {
                            n->b4[n->b4_size] = ret_d->dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
            //printf ("4");
            
            //same thing as above, but check the executable array for a dependancy
            if(read != NULL && iter < ret_d->no_dep_size){
                while(read[position]!=NULL){
                    while(ret_d->no_dep[iter]->write[innerpos]!=NULL){
                        if(strcmp(ret_d->no_dep[iter]->write[innerpos], read[position])==0)
                        {
                            n->b4[n->b4_size] = ret_d->no_dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
            //printf ("5");
            
            //this should do the same for the arguments parsed as command line and not strict
            //I/O redirection. This function checks the dependancy array of the graph
            if(args != NULL && iter < ret_d->dep_size){
                while(args[position]!=NULL){
                    while(ret_d->dep[iter]->write[innerpos]!=NULL){
                        if(strcmp(ret_d->dep[iter]->write[innerpos], args[position])==0)
                        {
                            n->b4[n->b4_size] = ret_d->dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
            //printf ("6");
            
            //same as above, but it checks the executable array for the graph
            if(args != NULL && iter < ret_d->no_dep_size){
                while(args[position]!=NULL){
                    while(ret_d->no_dep[iter]->write[innerpos]!=NULL){
                        if(strcmp(ret_d->no_dep[iter]->write[innerpos], args[position])==0)
                        {
                            n->b4[n->b4_size] = ret_d->no_dep[iter];
                            n->b4_size +=1;
                        }
                        innerpos+=1;
                    }
                    position +=1;
                }
            }
            //printf ("6.5");
            /*if(comm->input != NULL && iter < ret_d->depSize){
             if(ret_d->dep[iter]->c->output == comm->input)
             {
             n->before[n->bef_size] = ret_d->dep[iter];
             n->bef_size+=1;
             //ret_d->dep[iter]->after[ret_d->dep[iter]->aft_size] = n;
             //ret_d->dep[iter]->aft_size++;
             }
             }*/
            //reset all the values that you used before in the loop 
            position = 0;
            innerpos = 0;
            iter +=1;
        }
        //printf ("7");
        //if at any point the bef_size of the current node has been altered so it is greater than 0, add this
        //node to the dependancy array for the graph
        if(n->b4_size > 0){//if (debugmode) printf("befsize = %zu\n", n->bef_size);
            add_dep_to_graph(ret_d, n, 1);}
        //if the node has no dependancies, then push it to the executable array of the dependancy graph
        else{// if (debugmode) printf("befsize = %zu\n", n->bef_size);
            add_dep_to_graph(ret_d, n, 0);}
        //reset iter for the next command that needs to be added
        iter = 0;
        
        //TODO:
        // Determine which nodes already seen depend on this one.
        // Look through the independent nodes.
        // If a match is found, add this node as a dependency
        // And move it to the node with dependencies list
        // Look through the nodes with dependencies.
        // If a match is found, add this node as a dependency
        //printf ("9");
        
    }
    
    return ret_d;
}


int exec_command (command_t c, int time_travel);
void execute_parallel (command_stream_t s);

void execute_dep_graph (dependency_graph_t d)
{
    //printf ("12434324324325436");
    size_t iter = 0;
    size_t innerIter = 0;
    int status;
    int addstat;
    int dpos;
    if (d->dep_size>0)
    {
        int pid;
        pid = fork();
        if (pid == 0){
            execute_command(d->no_dep[0]->c, 0);
        }else if (pid<0)
            error(1, 0, "Child process failed to fork");
        else if (pid>0)
        {
            while(iter < d->dep_size)
            {
                //if (debugmode) printf("hello\n");
                for(/*innerIter*/; innerIter < d->dep[iter]->b4_size; innerIter++)
                {
                    if(d->dep[iter]->b4[innerIter] == d->no_dep[0])
                    {
                        for(dpos = innerIter; d->dep[dpos+1]!=NULL; dpos++)
                        {
                            d->dep[dpos] = d->dep[dpos+1];
                        }
                        d->dep[iter]->b4_size -= 1;
                    }
                }
                if (d->dep[iter]->b4_size == 0)
                {
                    //if (debugmode) printf("changing stuffs\n");
                    addstat = move_d_to_e(d, iter);
                    if (debugmode) printf("addstat: %d\n", addstat);
                }
                iter++;
            }
            //printf("\n");
            remove_e(d, 0);
            if (d->no_dep_size == 0)
            {
                while (waitpid(-1,&status,0)>0)
                    continue;
                exit(0);
            }
            execute_dep_graph(d);
        }
    }
    
    //TODO:
    // while d has independent nodes
    // remove a node n from d
    // Fork
    
    // If child:
    // execute n
    //for each node m dependent on n
    //	pop n from m's dependency array
    //	if m's dependency array is empty
    //		insert m into d.n_exec;
    
    // If parent:
    //	recurse
}

void execute_parallel (command_stream_t s)
{
    dependency_graph_t d;
    d = NULL;
    dependency_node_t n;
    n = NULL;
    
    
    // Make the dependency graph
    d = build_graph(s);
    
    // Execute the graph
    execute_dep_graph (d);
}
int command_status (command_t c)
{
  return c->status;
}


void
execute_simple_command(command_t command)
{
    pid_t pid = fork();
    if(pid > 0) {
        int status;
        if(waitpid(pid, &status, 0) == -1)
            error(1, errno, "Child process exit error");
        command->status = WEXITSTATUS(status);
    }
    else if(pid == 0) {
        if(command->input != NULL)
        {
            int nInput = open(command->input, O_RDONLY);
            if(nInput == -1)
                error(1, errno, "Couldn't open file as nInput");
            dup2(nInput, STDIN_FILENO);
            close(nInput);
        }
        if(command->output != NULL)
        {
            int nOutput = open(command->output, O_WRONLY | O_CREAT, 0664);
            if(nOutput == -1)
                error(1, errno, "Couldn't open file as output");
            dup2(nOutput, STDOUT_FILENO);
            close(nOutput);
        }
        execvp(command->u.word[0], command->u.word);
        exit(command->status);
    }
    else {
        error (1, 0, "forking error");
    }
}

void
execute_pipe_command(command_t command)
{
    int fd[2];
    pipe(fd);
    pid_t pid;
    if((pid=fork()) == 0)
    {
        dup2(fd[0],0);
        close(fd[1]);
        exec_command(command->u.command[1],0);
        close(fd[0]);
        exit(command->u.command[1]->status);
    } else if(pid > 0) {
        pid_t pid2;
        if((pid2=fork()) == 0) {
            dup2(fd[1],1);
            close(fd[0]);
            exec_command(command->u.command[0],0);
            close(fd[1]);
            exit(command->u.command[0]->status);
        } else if (pid2 > 0) {
            close(fd[0]);
            close(fd[1]);
            int status;
            pid_t wait_pid = waitpid(-1,&status,0);
            if(wait_pid == pid)
            {
                command->status = WEXITSTATUS(status);;
                waitpid(pid2,&status,0);
                return;
            }
            else if(wait_pid == pid2)
            {
                waitpid(pid,&status,0);
                command->status = WEXITSTATUS(status);
                return;
            }
        } else { error(1, errno, "forking error"); }
    } else { error(1, errno, "forking error"); }
}

void execute_and_command(command_t c)
{
    exec_command(c->u.command[0],0);
    
    if (c->u.command[0]->status == 0) {
        exec_command(c->u.command[1],0);
        c->status = c->u.command[1]->status;
    } else {
        c->status = c->u.command[0]->status;// first one returns false, don't execute second one
    }
}

void execute_or_command(command_t c)
{
    exec_command(c->u.command[0],0);
    if (c->u.command[0]->status == 0) { // first one returns true, don't execute the second command
        c->status = c->u.command[0]->status;
    } else {
        exec_command(c->u.command[1],0);
        c->status = c->u.command[1]->status;
    }
}

void execute_sequence_command(command_t c)
{
    exec_command(c->u.command[0],0);
    exec_command(c->u.command[1],0);
    c->status = c->u.command[1]->status;
}

void execute_subshell_command(command_t c)
{
    exec_command(c->u.subshell_command,0);
    c->status = c->u.subshell_command->status;
}

int exec_command(command_t c, int time_travel)
{
    switch(c->type)
    {
        case AND_COMMAND:
            execute_and_command(c);
            break;
        case OR_COMMAND:
            execute_or_command(c);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(c);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(c);
            break;
        case SIMPLE_COMMAND:
            execute_simple_command(c);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(c);
            break;
        default:
            error(1, 0, "Invalid command type");
    }
}

void
execute_command (command_t c, bool time_travel)
{
    if(!time_travel)
    {
        exec_command(c);
    }
}