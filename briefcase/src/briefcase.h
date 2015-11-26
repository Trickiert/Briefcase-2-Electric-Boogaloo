
/* Struct including all the variables for the Breifcase */

typedef struct briefcase {
	int locked;
	int armed;
	float tempPot;
	float timerVal;
	int codePointer;
	int codeNum[4];
	int codeNumX[4];
	int correctCode[4];
	char printMsg[64];
	
	struct briefcase* pNext;	
} briefcase_t;

