#include "license.readme"

#include <unistd.h>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <cstdio>

#include "stdafx.h"
#include "hashmgr.hxx"

extern void mychomp(char * s);
extern char * mystrdup(const char *);

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// build a hash table from a munched word list

HashMgr::HashMgr(const char * tpath)
{
	tablesize = 0;
	hashsize = 0;
	tableptr = NULL;
	int ec = load_tables(tpath);
	if (ec) {
		/* error condition - what should we do here */
		fprintf(stderr,"Hash Manager Error : %d\n",ec);
		fflush(stderr);
		delete [] tableptr;
		tablesize = 0;
	}
}


HashMgr::~HashMgr()
{
	if (tableptr) {
		for (int i =0; i < tablesize; ++i) {
			struct hentry * next;
			struct hentry * dp = &tableptr[i];
			// free the data in the first element
			delete [] dp->word;
			delete [] dp->astr;
			dp = dp->next;
			// free the data and the structure of subsequent elements
			while (dp != NULL) {
				next = dp->next;
				delete [] dp->word;
				delete [] dp->astr;
				delete dp;
				dp = next;
			}
		}
		delete [] tableptr;
	}
}


// lookup a root word in the hashtable
struct hentry * HashMgr::lookup(const char *word)
{
	struct hentry *dp;
	if (tableptr) {
		dp = &tableptr[hash(word)];
		if (dp->word == NULL) return NULL;
		for (  ;  dp != NULL;  dp = dp->next) {
			if (strcmp(word,dp->word) == 0) return dp;
		}
	}
	return NULL;
}



// add a word to the hash table (private)

int HashMgr::add_word(const char * word, int wl, const char * aff, int al)
{
	struct hentry* hp = new hentry;

	hp->wlen = wl;
	hp->alen = al;
	hp->word = mystrdup(word);
	hp->astr = mystrdup(aff);
	hp->next = NULL;

	int i = hash(word);
	struct hentry * dp = &tableptr[i];

	if (dp->word == NULL) {
		*dp = *hp;
		delete hp;
	} else {
		while (dp->next != NULL) dp=dp->next; 
		dp->next = hp;
	}
	++hashsize;
	return 0;
}

// save hash table to a munched word list

int HashMgr::save_tables(const char * tpath)
{
	int nWordCount = 0;

	if (tableptr) {
		FILE * rawdict = fopen(tpath, "w");
		if (rawdict == NULL) return 1;

		char lineString[MAXDELEN];
		itoa(hashsize, lineString, 10);
		fputs(lineString ,rawdict);

		for (int i =0; i < tablesize; ++i) {
			struct hentry * dp = &tableptr[i];
			while (dp != NULL) {
				struct hentry * next = dp->next;
				if (dp->word != NULL) {
					if (dp->alen != NULL) {
						sprintf(lineString,"%s/%s", dp->word, dp->astr);
						fputs(lineString, rawdict);
					} else {
						fputs(dp->word, rawdict);
					}
				}
				dp = next;
			}
		}
		fclose(rawdict);
	}
	return 0;
}

// load a munched word list and build a hash table on the fly

int HashMgr::load_tables(const char * tpath)
{
	int wl, al;
	char *ap;

	// raw dictionary - munched file
	FILE * rawdict = fopen(tpath, "r");
	if (rawdict == NULL) return 1;

	// first read the first line of file to get hash table size */
	char ts[MAXDELEN];
	if (! fgets(ts, MAXDELEN-1,rawdict)) return 2;
	mychomp(ts);
	tablesize = atoi(ts);
	tablesize = tablesize + 5;
	if ((tablesize %2) == 0) tablesize++;

	// allocate the hash table
	tableptr = new hentry[tablesize];

	if (! tableptr) return 3;
	memset(tableptr, 0, tablesize*sizeof(struct hentry));
	//for (int i=0; i<tablesize; i++) tableptr[i].word = NULL;

	// loop through all words on much list and add to hash
	// table and create word and affix strings

	while (fgets(ts,MAXDELEN-1,rawdict)) {
		mychomp(ts);
		// split each line into word and affix char strings
		ap = strchr(ts,'/');
		if (ap) {
			*ap = '\0';
			ap++;
			al = strlen(ap);
		} else {
			al = 0;
			ap = NULL;
		}

		wl = strlen(ts);

		// add the word and its index
		add_word(ts,wl,ap,al);

	}
	fclose(rawdict);
	return 0;
}


// the hash function is a simple load and rotate
// algorithm borrowed

int HashMgr::hash(const char * word)
{
	long  hv = 0;
	for (int i=0; i < 4  &&  *word != 0; i++)
	hv = (hv << 8) | tolower(*word++);
	while (*word != 0) {
		ROTATE(hv,ROTATE_LEN);
		hv ^= tolower(*word++);
	}
	return (unsigned long) hv % tablesize;
}
