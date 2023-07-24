#include "parser.h"
#include <stdio.h>

void
recursivePrint(parseState_t* s)
{
	int i;


	if(s->condlen > 0){
		printf("\tps%d:\n", s->statenum);
		
		int defaultTo = 0;

		if(s->condlen == 1 && s->conditions[0].condition == 0){
			printf("\t\tif((unsigned char)s[i] == 0)return %d;\n", s->conditions[0].next->token);
			printf("\t\telse return 0;\n");
		}
		else{
			printf("\t\tswitch((unsigned char)s[i]){\n");
			for(i = 0; i < s->condlen; i++){
				if(s->conditions[i].condition == 0){
					defaultTo = s->conditions[i].next->token;	
				}
				else{
					printf("\t\t\tcase %3d:\ti++;\tgoto ps%d;\n", s->conditions[i].condition, s->conditions[i].next->statenum);
				}
			}

			if(defaultTo == 0){
				printf("\t\t\tcase   0:\t\treturn 0;\n");
				printf("\t\t\tdefault :\t\treturn 0;\n");
			}
			else{	
				printf("\t\t\tcase 0:\t\treturn %d;\n", defaultTo);
			}

			printf("\t\t}\n");
		}
	}

	for(i = 0; i < s->condlen; i++){
		recursivePrint(s->conditions[i].next);
	}

}

void
parserGenerator(parseState_t* s)
{
	printf("/* This is generated with the simple parser generator that can be found in tools/parser/ */\n");
	printf("unsigned int parseTagname(char* s){\n");
	printf("\tint i = 0;\n");
	recursivePrint(s);
	printf("}\n");
}
