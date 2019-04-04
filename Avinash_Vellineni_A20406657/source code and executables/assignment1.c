#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> //type representing a directory stream (Data type through DIR)
// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int mode;          // the mode to open the file with
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

struct seqcmd {
  int type;          // ;
  struct cmd *left;  // left side of semicolan operator for seq execution
  struct cmd *right; // right side of semicolan operator for seq execution
};

struct andcmd {
  int type;         // &
  struct cmd *left; // left side of and operator for parallel execution
  struct cmd *right; // right side of and operator for parallel execution
};
int fork1(void);  // Fork but exits on failure.

struct cmd *parsecmd(char*); // to parse the command given in the command prompt
char *execution_dir_location(char*);  //to execute each command

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct andcmd *acmd;
  struct execcmd *ecmd;
  struct seqcmd *scmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(0);
  
  switch(cmd->type){  // check whether ; | & > or <
  default:
    fprintf(stderr, "unknown runcmd\n");   // executes on unknown command
    exit(-1);

  case ' ':   // executes when the command type is empty
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(0);
    execv(execution_dir_location(ecmd->argv[0]), ecmd->argv);
    fprintf(stderr, "exec not implemented\n");
    // Your code here ...
    break;

  case '>':
  case '<':  // executes for redirection io commands
    rcmd = (struct redircmd*)cmd;
    fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    runcmd(rcmd->cmd);
    break;
 case ';':   // implementation for sequential execution.
    scmd = (struct seqcmd*)cmd;
    if(fork1() == 0)  // child process
      runcmd(scmd->left);
    wait(&r);
    runcmd(scmd->right);
    break;
    
  case '|':  // executes for pipe command
    pcmd = (struct pipecmd*)cmd;
    fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    break;
  case '&':  // implementation for parallel execution
    acmd = (struct andcmd*)cmd;
    if(fork1() == 0)  // child process
      runcmd(acmd->left);
	runcmd(acmd->right);
	wait(&r);
	wait(&r);
	wait(&r);
    break;
  }    
  exit(0);
}

char *
execution_dir_location(char *cinput)
{  
  char *location = getenv("PATH");  // searches for the environment string pointed to by path and returns the associated value to the string.
  char *location_of_directory = strtok(location, ":"); // breaks the string (location) using series of delimiters(:)
  DIR *d;  // open directory
  struct dirent *dir; // read directory
  while (location_of_directory != NULL) { //while location of the is not null
    d = opendir(location_of_directory);
    if (d != NULL) {
      while ((dir = readdir(d)) != NULL) {   //while directory is not null
        if (strcmp(dir->d_name, cinput) == 0) { //compares the value pointer by the two strings
          char *end_location = malloc(strlen(location_of_directory) + strlen(cinput) + 2);   //malloc is used for memory alloction and strlen used for compute the length of the string
          end_location = strcat(end_location, location_of_directory);   //appends string2 at the end of string
          end_location = strcat(end_location, "/");
          end_location = strcat(end_location, cinput);
          return end_location;
        }
      }
    } else {
		fprintf(stderr, "cannot open this directory");    
    }
    location_of_directory = strtok(NULL, ":");  // breaks the string using series of delimiters(:)
  }
  return cinput;
}


int
getcmd(char *buf, int nbuf)
{
  
  if (isatty(fileno(stdin)))
    fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));  // starting point for parsing of the input commands
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *suacmd, char *file, int type)  // for io  redirection operation
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = suacmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)  // for pipe operation
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
seqcmd(struct cmd *left, struct cmd *right) // seqcmd structure for assigning values in cmd
{
  struct seqcmd *cmd;
  cmd = malloc(sizeof(*cmd));  // allocates the requested memory and returns a pointer to it
  memset(cmd, 0, sizeof(*cmd));  // is used to fill a block of memory with a particular value
  cmd->type = ';';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
andcmd(struct cmd *left, struct cmd *right) // andcmd structure for assigning values in cmd
{
  struct andcmd *cmd;
  cmd = malloc(sizeof(*cmd)); // allocates the requested memory and returns a pointer to it
  memset(cmd, 0, sizeof(*cmd)); // is used to fill a block of memory with a particular value
  cmd->type = '&';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;";   // including & and ; symbols for checking with the given input in the command prompt

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case ';':  // checking for ;
  case '&':  // checking for &
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);  // searches for occurrence of the symbols in the input command 
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);  // Parsing of the input command
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){  //  parsing using and operator
    gettoken(ps, es, 0, 0);
    cmd = andcmd(cmd, parseline(ps, es));
  }
  if(peek(ps, es, ";")){   // parsing using semicolan operator
    gettoken(ps, es, 0, 0);
    cmd = seqcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)  // parse operation for the pipe command
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)  // parse operation for the io redirection command
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es) // execution function for parse operation
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|&;")){  // execute the wile statement only for redirection command 
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
