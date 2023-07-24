#define STATE_BEGIN 	0
#define STATE_END 	1

typedef struct parseCond_t
{
	short condition;
	struct parseState_t* next;
} parseCond_t;

typedef struct parseState_t
{
	char c;
	int condlen;
	parseCond_t* conditions;
	int token;
	int statenum;
} parseState_t;

