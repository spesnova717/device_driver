/* vmedeb.cc */
/* Created by Enomoto Sanshiro on 17 July 2000. */
/* Last updated by Enomoto Sanshiro on 15 July 2010. */


#define _LARGEFILE64_SOURCE

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string>
#include "vmedrv.h"


using namespace std;


typedef unsigned WordType;
static const char* DeviceFile = "/dev/vmedrv32d32";


int ProcessDumpCommand(int File, unsigned BaseAddress, istream& InputStream);
int ProcessModifyCommand(int File, unsigned BaseAddress, istream& InputStream);
int Dump(int File, unsigned Offset, unsigned Size);
int Modify(int File, unsigned Offset);


int main(int argc, char** argv)
{
    unsigned BaseAddress;
    
    if (
	(argc < 2) ||
	(! (istringstream(argv[1]) >> hex >> BaseAddress))
    ){
	cerr << "USAGE: " << argv[0] << " ";
	cerr << "BASE_ADDRESS(hex)" << endl;
	cerr << " ex) " << argv[0] << " 2000000" << endl;
	return EXIT_FAILURE;
    }

    int File;
    if ((File = open(DeviceFile, O_RDWR)) == -1) {
        cerr << "ERROR: " << strerror(errno) << endl;
	exit(EXIT_FAILURE);
    }

    string Input;
    while (cout << "-> " << flush, getline(cin, Input, '\n')) {
	istringstream InputStream(Input);

	string Command;
	if (InputStream >> Command) {
	    if (Command == "d") {
		ProcessDumpCommand(File, BaseAddress, InputStream);
	    }
	    else if (Command == "m") {
		ProcessModifyCommand(File, BaseAddress, InputStream);
	    }	    
	    else if (Command == "q") {
		break;
	    }
	    else {
		cout << "d OFFSET N_WORDS: dump N_WORDS of words from address OFFSET" << endl;
		cout << "m OFFSET: dump and modify words from address OFFSET" << endl;
		cout << "h: show this help" << endl;
		cout << "q: quit" << endl;
	    } 
	}
    }

    close(File);

    return 0;
}


int ProcessDumpCommand(int File, unsigned BaseAddress, istream& InputStream)
{
    unsigned OffsetAddress, Size;
    if (InputStream >> hex >> OffsetAddress >> Size) {
        Size *= sizeof(WordType);
	return Dump(File, BaseAddress + OffsetAddress, Size);
    }
    
    return 0;
}


int ProcessModifyCommand(int File, unsigned BaseAddress, istream& InputStream)
{
    unsigned OffsetAddress;
    if (! (InputStream >> hex >> OffsetAddress)) {
	return 0;
    }

    unsigned Address = BaseAddress + OffsetAddress;

    for (;;) {
	if (! Modify(File, Address)) {
	    return 0;
	}
	Address += sizeof(WordType);
    }

    return 1;
}


int Dump(int File, unsigned Address, unsigned Size)
{
    static char Buffer[32768];
    if (Size > sizeof(Buffer)) {
	cout << "WARNING: Size is too large." << endl;
	return 0;
    }

    if (lseek64(File, Address, SEEK_SET) == -1) {
        cerr << "ERROR: lseek64(): " << strerror(errno) << endl;
	return 0;
    }

    int ReadSize;
    if ((ReadSize = read(File, Buffer, Size)) == -1) {
        cerr << "ERROR: read(): " << strerror(errno);
	return 0;
    }

    WordType Word;
    for (unsigned Index = 0; Index < ReadSize / sizeof(WordType); Index++) {
	Word = ((WordType*) Buffer)[Index];
	cout << hex << setfill('0');
	cout << setw(4) << ((Address >> 16) & 0x0000ffff);
	cout << setw(4) << ((Address >> 0) & 0x0000ffff);
	cout << ": ";
	cout << setw(4) << ((Word >> 16) & 0x0000ffff);
	cout << setw(4) << ((Word >> 0) & 0x0000ffff);
	cout << "  ";

	for (unsigned i = sizeof(WordType); i > 0; i--) {
	    char Byte = (Word >> (8 * (i - 1))) & 0xff;
	    if (isprint(Byte)) {
		cout << Byte;
	    }
	    else {
		cout << '.';
	    }
	}
	cout << endl;

        Address += sizeof(WordType);
    }

    return 1;
}


int Modify(int File, unsigned Address)
{
    if (lseek64(File, Address, SEEK_SET) == -1) {
        cerr << "ERROR: lseek64(): " << strerror(errno) << endl;
	return 0;
    }

    WordType Word;
    if (read(File, &Word, sizeof(WordType)) == -1) {
        cerr << "ERROR: read(): " << strerror(errno);
	return 0;
    }

    cout << hex << setfill('0');
    cout << setw(4) << ((Address >> 16) & 0x0000ffff);
    cout << setw(4) << ((Address >> 0) & 0x0000ffff);
    cout << " (";
    cout << setw(4) << ((Word >> 16) & 0x0000ffff);
    cout << setw(4) << ((Word >> 0) & 0x0000ffff);
    cout << "): " << flush;
    
    static char Input[128];
    if (! (cin.getline(Input, sizeof(Input), '\n'))) {
	return 0;
    }
    if (strlen(Input) == 0) {
	return 1;
    }

    WordType NewWord;
    if (! (istringstream(Input) >> hex >> NewWord)) {
	return 0;
    }

    if (lseek64(File, Address, SEEK_SET) == -1) {
        cerr << "ERROR: lseek64(): " << strerror(errno) << endl;
	return 0;
    }

    if (write(File, &NewWord, sizeof(WordType)) == -1) {
        cerr << "ERROR: read(): " << strerror(errno);
	return 0;
    }    

    return 1;
}
