#ifndef _ATYPES_HXX_
#define _ATYPES_HXX_

#define SETSIZE         256
#define MAXAFFIXES      256
#define MAXWORDLEN      100
#define XPRODUCT        1
#define MAXLNLEN        1024

#define TESTAFF( a , b , c ) memchr((void *)(a), (int)(b), (size_t)(c) )

struct affentry
{
   char * strip;
   char * appnd;
   short  stripl;
   short  appndl;
   short  numconds;
   short  cpflag;
   char   conds[SETSIZE];
};


#endif





