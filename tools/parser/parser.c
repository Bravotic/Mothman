#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define BUFFER_SIZE 64

#define TOKEN_WILDCARD 1

#define parser_main main

/* Why do you use shorts for strings?
 * To put it simply, we do it so we can have regular expression
 * support. If we find any special regular expression characters
 * we read them in and set them to a number a character can't
 * possibly be, anything over 255. Since we only officially
 * support UTF-8, we have no overlap and regular expressions 
 * are free to exist. */

/* Read a line from a file, assumes end of line is \n */
static short*
parser_readline(FILE* src)
{
	short *line;
	unsigned char c;
	int i, linecap;

	line = (short*)malloc(BUFFER_SIZE * sizeof(short));
	linecap = BUFFER_SIZE;
	i = 0;
	
	/* Using a label/goto here so I don't have to rely on compiler 
	 * optimization in order to make this an infinite loop */
	readLoop:
		c = fgetc(src);
		if(c == 255 || c == '\n'){
			line[i] = 0;
			return line;
		}

		if(c > 32){
			line[i++] = c;
		}
		
		/* If our line is too long make sure to increase the size of
		the buffer */
		if(i >= linecap){
			linecap *= 2;
			line = (short*)realloc(line, linecap * sizeof(short));
		}
	goto readLoop;
}

static parseState_t*
createState()
{
	parseState_t* st;
	static int statenumiter = 0;

	st = (parseState_t*)malloc(1 * sizeof(parseState_t));
	st->conditions = (parseCond_t*)malloc(BUFFER_SIZE * sizeof(parseCond_t));
	st->condlen = 0;
	st->statenum = statenumiter++;
	return st;
}

static void
freeStates(parseState_t* s)
{
	int i;
	for(i = 0; i < s->condlen; i++){
		freeStates(s->conditions[i].next);
	}
	free(s->conditions);
	free(s);
}

int 
parser_main(int argc, char** argv)
{
	short **tokens;
	FILE *fTokens;
	int tokencap, tokenlen, i, tokenNumber;
	parseState_t *init, *current, *last;

	/* Step 1: Read our tokens into memory */

	/* Open the file 'tokens.txt' which contains our tokens we need to
	parse */
	fTokens = fopen("html-tags.txt", "r");

	/* Initialize our token list */
	tokens = (short**)malloc(BUFFER_SIZE * sizeof(short*));
	tokencap = BUFFER_SIZE;
	tokenlen = 0;

	i = 0;

	/* Read through every line and add them to the token list */
	while(!feof(fTokens)){
		short* token;
		int toklen, j;

		/* Read our token from file */
		token = parser_readline(fTokens);
		if(token[0] != 0){
			tokens[i++] = token;

			if(i >= tokencap){
				tokencap *= 2;
				tokens = (short**)realloc(tokens, tokencap * sizeof(short*));
			}
		}
		else{
			free(token);
		}

	}
	tokenlen = i;
	
	/* Step 2: Go through and create the states */
	
	init = createState();
	init->c = STATE_BEGIN;

	tokenNumber = 1;

	/* We go through each token and turn it into a series of states */
	for(i = 0; i < tokenlen; i++){
		int j, k, toklen;
		current = init;

		for(toklen = 0; tokens[i][toklen] != 0; toklen++);

		/* Read through each character in the token string */	
		for(k = 0; k <= toklen; k++){
			int found = 0;

			/* Check every condition to see if we already have one, we can just reuse it */
			for(j = 0; j < current->condlen; j++){

				/* If we do, simply use the conditon */
				if(current->conditions[j].condition == tokens[i][k]){

					current = current->conditions[j].next;
					found = 1;
				}
			}
			
			/* If not, we need to add a new condition */
			if(found == 0){
				parseCond_t condition;
				parseState_t *newState;

				newState = createState();
				newState->c = tokens[i][k];

				condition.condition = tokens[i][k];
				condition.next = newState;

				current->conditions[current->condlen++] = condition;
				current = newState;
			}

		}
		/* When we get here, we should be at the end of the token, so we add the token number */
		current->token = tokenNumber++;
	}

	/* Step 3: Hand off to generator */
	parserGenerator(init);

	/* Step 4: Free, and we're done */
	for(i = 0; i < tokenlen; i++){
		free(tokens[i]);
	}
	free(tokens);

	freeStates(init);
}
