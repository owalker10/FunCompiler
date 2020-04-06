#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>




/* Kinds of tokens */
enum Kind {
    ELSE,    // else
    END,     // <end of string>
    EQ,      // =
    EQEQ,    // ==
    FUNC,    // function
    ID,      // <identifier>
    IF,      // if
    INT,     // <integer value >
    LBRACE,  // {
    LEFT,    // (
    MUL,     // *
    NONE,    // <no valid token>
    PLUS,    // +
    PRINT,   // print
    RBRACE,  // }
    RIGHT,   // )
    WHILE    // while
};

/* information about a token */
typedef struct Token {
	enum Kind kind;
	uint64_t value;
	char *start;
	int length;
    struct Token* next;

} Token;

typedef struct Fun {
	Token* token;
	struct Fun* next;
} Fun;

struct Entry {
	uint64_t value;
	struct Entry *children;
};

/* The symbol table */
struct Entry *table;

uint64_t get(char *id) {
	int c = *id;
	struct Entry *e = &table[c-'a'];
	id++;
	c = *id;
	while (c != 0)
	{
        if (e->children == NULL)
            return 0;
		int idx = (c >= '0' && c<= '9') ? c-'0' : c-'a'+10;
		e = &e->children[idx];
        id++;
        c = *id;
	}
	return e->value;
}

uint64_t getl(char* start, int length)
{
	int c = *start;
	struct Entry *e = &table[c-'a'];
	start++;
	length--;
	c = *start;
	while (length > 0)
	{
        if (e->children == NULL)
            return 0;
		int idx = (c >= '0' && c<= '9') ? c-'0' : c-'a'+10;
		e = &e->children[idx];
        start++;
        c = *start;
        length--;
	}
	return e->value;
}


void set(char *id, uint64_t value) {
	char c = *id;
	struct Entry *e = &table[c-'a'];
	id++;
	c = *id;
	while (c != 0)
	{
		int idx = (c >= '0' && c<= '9') ? c-'0' : c-'a'+10;
		if (e->children == NULL)
		{
			e->children = (struct Entry*) calloc(36,sizeof(struct Entry));
		}
		e = &e->children[idx];
		id++;
		c = *id;
	}
	e->value = value;
}

void setl(char *start, int length, uint64_t value) {
	char c = *start;
	struct Entry *e = &table[c-'a'];
	start++;
	length--;
	c = *start;
	while (length > 0)
	{
		int idx = (c >= '0' && c<= '9') ? c-'0' : c-'a'+10;
		if (e->children == NULL)
		{
			e->children = (struct Entry*) calloc(36,sizeof(struct Entry));
		}
		e = &e->children[idx];
		start++;
		c = *start;
		length--;
	}
	e->value = value;
}

/* The current token */
static Token *current = &((Token) { NONE, 0, NULL, 0, NULL});
static Fun *curFun = &((Fun) {NULL,NULL});

static jmp_buf escape;

static FILE *f;

static int lcount = 0;
static int braceCount = 0;
static int inMain = 1;
static int inWhile = 0;

enum Kind peek();

static char *remaining() {
	return current->start;
}

static void error() {
	printf("error at '%s'\n", remaining());
	longjmp(escape, 1);
}

enum Kind peek() {
	if (current->kind == NONE)
	{
		error();
		return NONE;
	}
	return current->kind;

}

void consume() {
	peek();
	current = current->next;
}

char *getId(void) {
	char *id = malloc(current->length+1);
	for (int i = 0; i < current->length; i++)
	{
		id[i] = current->start[i];
	}
	id[current->length] = 0;
	return id;
}

uint64_t getInt(void) {
	return current->value;
}

void expression(int doit);
uint64_t statement(int doit);
void seq(int doit);

/* "f<address>" ie "f0x55512f14a458" */
char* tokenToLabel(Token* token){
	uint64_t address = (uint64_t)token;
	char* label = malloc(16);
	for (int i = 14; i >= 3; i--)
	{
		uint64_t digit = address%16;
		/* convert 0-15 to '0'-'9' or 'a'-'f' */
		label[i] = digit > 9 ? digit + 87 : '0' + (address%16);
		address/=16;
	}
    label[0] = 'f';
    label[1] = '0';
    label[2] = 'x';
	label[15] = 0;

	return label;
}

/* handle id, literals, and (...) */
void e1(int doit) {
	if (peek() == LEFT) {
		consume();
		expression(doit); /* puts something in r8 */
		if (peek() != RIGHT) {
			error();
		}
		consume();
		return;
	} else if (peek() == INT) {
		if (doit)
		{
			/* movq $<num> %r8 */
			fprintf(f,"    movq $%lu, %%r8\n",getInt());
		}
		consume();
		return;
	} else if (peek() == ID) {
        if (doit)
		{
			/* movq <ID> %r8 */
			fputs("    movq ",f);
			for (int i = 0; i < current->length; i++)
			{
				fputc((current->start)[i],f);
			}
			fputs("_, %r8\n",f);
		}
		consume();
        return;
    } else if (peek() == FUNC) {
        if (doit){
        	/* move the address of the label of the function to %r8 */
        	fprintf(f,"    movq $%s, %%r8\n",tokenToLabel(current));
        }
        consume();
        statement(0);
        return;

	} else {
		error();
		return;
	}
}

/* handle '*' */
void e2(int doit) {
	e1(doit); /* puts something in r8 */
	while (peek() == MUL) {
		consume();
		if (doit) fputs("    push %r8\n",f);
		e1(doit);
		if (doit){
			fputs("    movq %r8, %r9\n",f);
			fputs("    pop %r8\n",f);
			fputs("    imul %r9, %r8\n",f);
		}

	}
	return;
}

/* handle '+' */
void e3(int doit) {
	e2(doit); /* puts something in r8 */
	while (peek() == PLUS) {
		consume();
		if (doit) fputs("    push %r8\n",f);
		e2(doit);
		if (doit){
			fputs("    movq %r8, %r9\n",f);
			fputs("    pop %r8\n",f);
			fputs("    add %r9, %r8\n",f);
		}
	}
	return;
}

/* handle '==' */
void e4(int doit) {
	e3(doit); /* puts something in r8 */
	while (peek() == EQEQ) {
		consume();
		if (doit) fputs("    push %r8\n",f);
		e3(doit);
		if (doit){
			fputs("    movq %r8, %r9\n",f);
			fputs("    pop %r8\n",f);
			fputs("    xor %rax, %rax\n",f);
			fputs("    cmp %r8, %r9\n",f);
			fputs("    lahf\n",f); /* loads flags into %ah */
			fputs("    shr $14, %ax\n",f); /* get zero flag bit */
			fputs("    and $1, %ax\n",f);
			fputs("    movq %rax, %r8\n",f);
		}
	}
	return;
}

void expression(int doit) {
	e4(doit);
}

uint64_t statement(int doit) {
	enum Kind p = peek();
    switch(p) {
		case ID: {
			char *id = doit ? getId() : NULL;
			consume();
            /* check if function call */
            if (peek() == LEFT){
                consume();
                if (peek()!= RIGHT)
                    error();
                consume();
                if (doit){
                    fprintf(f,"    movq %s_, %%r8\n",id);
                    free(id);
                    int c = braceCount;
                    Token* n = current; 
                    while (c > 0 && n->kind == RBRACE){
                        c--;
                        n = n->next;
                    }
                  
                    if (c==0 && braceCount > 0 && !inMain && !inWhile)
                    {
                         fputs("    jmp *%r8\n",f);  
                    }
                    else{
                        fputs("    call *%r8\n",f);
                    } 
               
                    
                }
                return 1;
            }
			if (peek() != EQ)
				error();
			consume();
            /* check if function definition */
            if (peek() == FUNC){
            	/* put the value of the function label into the id */
            	if (doit){
            		fprintf(f,"    movq $%s, %s_\n",tokenToLabel(current),id);
            		free(id);
            	}
                consume();
                statement(0);    
            }
            /* else value definition */
            else {
            	expression(doit);
            	if (doit){
            		fprintf(f,"    movq %%r8, %s_\n",id);
            		free(id);
            	}
            }

			return 1;
        }
		
		case LBRACE:
            braceCount++;
			consume();
			seq(doit);
			if (peek() != RBRACE)
				error();
            braceCount--;
			consume();
			return 1;
		case IF: {
			
			lcount++;
			int theCount = lcount;

            consume();
            expression(doit); /* puts expression in %r8 */
            if (doit){
	            fputs("    cmp $0, %r8\n",f);

	            fprintf(f,"    jne .if_%d\n",theCount);
	            fprintf(f,"    je .else_%d\n",theCount);
	            fprintf(f,".if_%d:\n",theCount);
	        }
            statement(doit); /* write the IF code */

            if (doit){
            	fprintf(f,"    jmp .cont_%d\n",theCount);
            	fprintf(f,".else_%d:\n",theCount);
            }
            
            if (peek() == ELSE){
            	consume();
            	statement(doit);
            	if (doit)
            		fprintf(f,"    jmp .cont_%d\n",theCount);
            }
            if (doit)
            	fprintf(f,".cont_%d:\n",theCount);

            return 1;
		}
		case WHILE: {
			consume();
            inWhile++;
            
            lcount++;
            int theCount = lcount;

            if (doit) fprintf(f,".start_%d:\n",theCount); /* label start */
            expression(doit);
            if (doit){
            	fputs("    cmp $0, %r8\n",f);
            	fprintf(f,"    je .cont_%d\n",theCount); /* if cond == 0, go to end */
            }
            statement(doit); /* generate while loop code */
            if (doit){
            	fprintf(f,"    jmp .start_%d\n",theCount); /* go back up */
				fprintf(f,".cont_%d:\n",theCount); /* label next part of code */
			}
            inWhile--;

            return 1;
		}
		case PRINT:
			consume();
			expression(doit);
			if (doit){
				fputs("    movq %r8, %rsi\n",f);
				fputs("    call print\n",f);
			}
	    
			return 1;

		default:
		return 0;
	}
}


void parse(char *prog)
{
	fputs("    .data\n",f);
	fputs("format__: .byte '%', 'l', 'u', 10, 0\n",f);
	fputs("argc_: .quad 0\n",f);

	Token *t = current;
	Fun* fn = curFun;
	char *keywords[] = {"if","else","while","print","fun"};
	char c = *prog;
	while (c != 0)
	{
		if (c >= 1 && c <= 32)
		{
			prog++;
		}
		else if (c >= 'a' && c <= 'z')
		{
			/* check for keywords */
			int key = 0;
			for (int i = 0; i < 5; i++)
			{
                int match = 1;
				int j;
                int l = strlen(keywords[i]);
				for (j = 0; j < l; j++)
				{
					if (prog[j] != keywords[i][j]){
                        match = 0;
                        break;
                    }					
                }
                if (match){
				    char nextchar = prog[j];
					if (!((nextchar >= 'a' && nextchar <= 'z') || (nextchar >= '0' && nextchar <= '9')))
					{
						Token *new = (Token*)malloc(sizeof(Token));
						switch (i)
						{
							case 0:
							new->kind = IF; break;
							case 1:
							new->kind = ELSE; break;
							case 2:
							new->kind = WHILE; break;
							case 3:
							new->kind = PRINT; break;
                            case 4:
                            new->kind = FUNC; break;
						}
						new->start = prog;
						new->length = l;
						t->next = new;
						t = new;

						/* if this is a function token, save it to the list */
						if (i == 4){
							Fun *newFun = (Fun*)malloc(sizeof(Fun));
							newFun->token = t;
							fn->next = newFun;
							fn = newFun;
							set(tokenToLabel(t),(uint64_t)t);
						}

					prog+=j; /* move the cursor to the first unparsed character */
                        c = *prog;
						key = 1;
						break;
					}
				}
            }
				if (key){ continue; }
			/* so we know this is an identifier */
				int l = 1;
				char nextchar = prog[1];
				while (((nextchar >= 'a' && nextchar <= 'z') || (nextchar >= '0' && nextchar <= '9')))
				{
					
					l++;
					nextchar = prog[l];
				}

				/* if this is a new identifier, put it in the data segment */

				/* we don't want to add argc a second time! */
				char* ARGC = "argc";
				int isargc = 1;
				for (int i = 0; i < 4; i++)
				{
					if (i == l){
						isargc = 0;
						break;
					}
					if (prog[i] != ARGC[i])	{
						isargc = 0;
						break;
					}				
				}
                /* only put in data segment if not already there, and if the string doesn't match "argc" */
				if (!getl(prog,l) && (!isargc || ((prog[4] >= 'a' && prog[4] <= 'z')||(prog[4] >= '0' && prog[4] <= '9')))){
					for (int i = 0; i < l; i++){
						fputc(prog[i],f);
					}
					fputs("_: .quad 0\n",f);
					setl(prog,l,1);
				}
				
				Token *new = (Token*)malloc(sizeof(Token));
				new->kind = ID;
				new->start = prog;
				new->length = l;
				t->next = new;
				t = new;

				prog+=l;
			
		}
    	/* this must be an immediate value */ 
		else if (c >= '0' && c <= '9')
		{
			uint64_t value = c - 48;
			int l = 1;
			char nextchar = prog[1];
			while ((nextchar >= '0' && nextchar <= '9') || nextchar == '_')
			{
				if (nextchar != '_'){
					value *=10;
					value += prog[l] - 48;
				}
				l++;
                nextchar = prog[l];
			}
			Token *new = (Token*)malloc(sizeof(Token));
			new->kind = INT;
			new->start = prog;
			new->length = l;
            new->value = value;
			t->next = new;
			t = new;
			prog+=l;
		}
		else if (c == '=')
		{
			Token *new = (Token*)malloc(sizeof(Token));
			new->start = prog;
			if (prog[1] == '='){
				new->kind = EQEQ;
				new->length = 2;
				prog+=2;
			}
			else{
				new->kind = EQ;
				new->length = 1;
				prog++;
			}
			t->next = new;
			t = new;
		}

		/* token must be single character long */
		else{
			Token *new = (Token*)malloc(sizeof(Token));
			new->start = prog;
			new->length = 1;
			switch (c){
				case '+':
				new->kind = PLUS;
				break;
				case '*':
				new->kind = MUL;
				break;
				case '{':
				new->kind = LBRACE;
				break;
				case '}':
				new->kind = RBRACE;
				break;
				case '(':
				new->kind = LEFT;
				break;
				case ')':
				new->kind = RIGHT;
				break;
				default:
				error();
				break;
			}
			t->next = new;
			t = new;
			prog++;

		}

		c = *prog;
	}
    
    Token *new = (Token*) malloc(sizeof(Token));
    new->start = prog;
    new->length = 1;
    new->next = NULL;
    new->kind = END;
    t->next = new;

    	/* skip the placeholder null token that was originally made */
	current = current->next;
	curFun = curFun->next;

}

void seq(int doit) {
	while (statement(doit)) ;
}




/* writes out the functions' assembly in .text */
void buildFuncs()
{
	/*
	for each item in fun list,
	fputs(tokenToLabel:)
	current = fun->token->next
	call statement(1)
    fputs(ret)
    fun = fun->next
	*/

    inMain = 0;
    braceCount = 0;
    while (curFun != NULL){
	    fputs(tokenToLabel(curFun->token),f);
	    fputs(":\n",f);
	    current = curFun->token->next;
	    statement(1);
	    fputs("    ret\n",f);
	    curFun = curFun->next;
	}
}

void program(void) {
	fputs("    .text\n",f);
    fputs("    .global main\n",f);
    fputs("main:\n",f);
    fputs("    movq %rdi, argc_\n",f);
    fputs("    .extern printf\n",f);
	seq(1);
	fputs("    xor %rax, %rax\n",f);
	fputs("    ret\n",f);
    fputs("print:\n    lea format__(%rip),%rdi\n    xor %rax, %rax\n    call printf\n    ret\n",f);
    enum Kind end = peek();
	buildFuncs();
	if (end != END)
		error();
}

void compile(char *prog){
	current->kind = NONE;
	table = (struct Entry*) calloc(26,sizeof(struct Entry));
	parse(prog); /* builds tokens, data seg, & function list */
	int x = setjmp(escape);
	if (x == 0)
		program();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr,"usage: %s <name>\n",argv[0]);
        exit(1);
    }

    char* name = argv[1]; /*filename*/
    size_t len = strlen(name);

    size_t sLen = len+3;  // ".s" + 0
    char* sName = (char*) malloc(sLen);
    if (sName == 0) {
        perror("malloc");
        exit(1);
    }

    strncpy(sName,name,sLen);
    strncat(sName,".s",sLen);

    size_t fLen = len+5; // ".fun" + 0
    char* fName = (char*) malloc(fLen);
    if (sName == 0) {
        perror("malloc");
        exit(1);
    }
    strncpy(fName,name,fLen);
    strncat(fName,".fun",fLen);

    printf("compiling %s to produce %s\n",fName,sName);

    f = fopen(sName,"w");
    if (f == 0) {
        perror(sName);
        exit(1);
    }

    /*
    fputs("    .data\n",f);
    fputs("format: .byte '%', 'd', 10, 0\n",f);
    fputs("    .text\n",f);
    fputs("    .global main\n",f);
    fputs("main:\n",f);
    fputs("    movq $0,%rax\n",f);
    fputs("    lea format(%rip),%rdi\n",f);
    fputs("    movq $42,%rsi\n",f);
    fputs("    .extern printf\n",f);
    fputs("    call printf\n",f);
    fputs("    movq $0,%rax\n",f);
    fputs("    ret\n",f);
    */

    /* do stuff here*/
    FILE *file = fopen(fName, "r");
    /* from https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer */
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *prog = malloc(fsize + 1);
    fread(prog, 1, fsize, file);
    prog[fsize] = 0;


    compile(prog);
    fclose(file);

    fclose(f);

    size_t commandLen = len*2 + 1000;
    char* command = (char*) malloc(commandLen); 

    snprintf(command,commandLen,"gcc -static -g -o %s.exec %s",name,sName);

    printf("compiling %s to produce %s.exec\n",sName,name);
    printf("running %s\n",command);
    int rc = system(command);
    if (rc != 0) {
        perror(command);
        exit(1);
    }

    return 0;
}
