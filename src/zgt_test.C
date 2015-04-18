/* main program that processes input and invokes the Tx mgr methods */
// the main data structures are initialized here
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string>
#include <fstream>
#include "zgt_def.h"
#include "zgt_tm.h"
#include "zgt_global.h"
#include "zgt_extern.h"

void Tokenize(const string& str, string tokens[4], const string& delimiters = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);
	int i = 0;
	while (string::npos != pos || string::npos != lastPos) {
		// Found a token, add it to the array.
		tokens[i] = str.substr(lastPos, pos - lastPos);
		i++;

		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);

		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

int string2int(char *s, string str) {
	int i = 0;
	s = (char *) malloc(sizeof(char) * 12);
	while (str[i] != '\0') {
		s[i] = str[i];
		i++;
	}
	int k = atoi(s);
	return (k);
}

int main(int argn, char **argv) {
	long sgno = 1, obno, tid;
	char lockmode;
	char *infilename;
	zgt_tx *t;
	string delimiters = " ";
	char str[100];
	string s;
	int op;
	char *c;

	if (argn < 2) {
		printf("USAGE:\n");
		printf("\tzgt_test <input file name>\n");
		exit(1);
	}

	infilename = argv[1];
	ifstream inFile(infilename, ios::in);

	if (!inFile.is_open()) {
		cout << "\nError opening file: " << infilename << ".txt\n";
		exit(1);
	}

	//if invoked correctly, create one transaction manager object

	ZGT_Sh = new zgt_tm();
	ZGT_Ht = new zgt_ht(ZGT_DEFAULT_HASH_TABLE_SIZE);

	inFile.getline(str, 100);
	while (!inFile.eof()) {
		s = str;
		cout << s << "\n";
		string tokens[4];
		Tokenize(s, tokens);
		if (tokens[0] == "//")
			cout << s << "\n";
		else if (tokens[0] == "Log" || tokens[0] == "log") {
			cout << "Log file name:" << tokens[1] << "\n\n";
			//	printf("\nLog :", tokens[1]);
			ZGT_Sh->openlog(tokens[1]);
		}

		else if (tokens[0] == "BeginTx" || tokens[0] == "begintx") {
			tid = string2int(c, tokens[1]);
			printf("BeginTx : %d\n\n", tid);
			if ((op = ZGT_Sh->BeginTx(tid)) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			//        {  //error code }
		}

		else if (tokens[0] == "Read" || tokens[0] == "read") {
			int k = string2int(c, tokens[1]);
			tid = k;

			k = string2int(c, tokens[2]);
			obno = k;
			//    cout << "\nRead: " << tid << " : " << obno;
			printf("Read : %d : %d\n\n", tid, obno);
			if ((op = ZGT_Sh->TxRead(tid, obno)) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			//  { //error code }
		}

		else if (tokens[0] == "Write" || tokens[0] == "write") {
			int k = string2int(c, tokens[1]);
			tid = k;

			k = string2int(c, tokens[2]);
			obno = k;

			//      cout << "\nwrite: " << tid << " : " << obno;
			printf("Write : %d : %d\n\n", tid, obno);
			if ((op = ZGT_Sh->TxWrite(tid, obno)) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			// { //error code}
		} else if (tokens[0] == "Abort" || tokens[0] == "abort") {
			int k = string2int(c, tokens[1]);
			tid = k;

			//     cout << "\nAbort: " << tid;
			printf("Abort : %d\n\n", tid);
			if ((op = ZGT_Sh->AbortTx(tid)) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			// { //error code}
		}

		else if (tokens[0] == "Commit" || tokens[0] == "commit") {
			printf("Commit : %d\n\n", tid);
			int k = string2int(c, tokens[1]);
			tid = k;
			if ((op = ZGT_Sh->CommitTx(tid)) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			//  { //error code}
		} else if (tokens[0] == "Detect" || tokens[0] == "detect") {
			printf("Detect Cycles :\n\n");
			if ((op = ZGT_Sh->ddlockDet()) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			//  { //error code}
		} else if (tokens[0] == "choose" || tokens[0] == "Choose") {
			printf("Detect Cycles AND choose a victim:\n\n");
			if ((op = ZGT_Sh->chooseVictim()) < 0)
				cout << "\nerro from:" << tokens[0] << " for TID:" << tid << "\n";
			//  { //error code}
		} else {
			cout << "\ninput error:" << tokens[0] << "," << tokens[1] << "," << tokens[2] << "," << tokens[3] << "\n";
			cout << endl;
			inFile.close();
			pthread_exit(NULL);
		}
		inFile.getline(str, 100);
	}
	cout << endl;
	inFile.close();
	pthread_exit(NULL);
}

