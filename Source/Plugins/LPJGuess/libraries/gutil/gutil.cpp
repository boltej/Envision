///////////////////////////////////////////////////////////////////////////////////////
/// \file gutil.cpp
/// \brief GUTIL LIBRARY (Fully portable version)
///
/// This library is provided as a component of the ecosystem modelling platform
/// LPJ-GUESS. It combines the functionality of the XTRING and BENUTIL libraries
///
/// FULL PORTABILITY VERSION: tested and should work in any Unix, Linux or Windows
/// environment.
///
/// Enquiries to: Joe Siltberg, Lund University: joe.siltberg@nateko.lu.se
/// All rights reserved, copyright retained by the author.
///
/// \author Ben Smith, University of Lund
/// $Date: 2014-03-13 13:07:20 +0100 (Thu, 13 Mar 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "gutil.h"

#include <stdarg.h>

void fail() {

	::printf("Error in GUTIL library: out of memory\n");
	fprintf(stderr,"Error in GUTIL library: out of memory\n");
	exit(99);
}

const unsigned long XEGMENT=32;

void xtring::init() {

	buf=NULL;
	ptext=new char[XEGMENT];
	if (!ptext) fail();
	nxegment=1;
	*ptext='\0';
}

void xtring::resize(unsigned long nchar) {

	// Resizes character buffer pointed to by ptext if necessary to
	// accomodate nchar characters, plus a \0

	nchar++;
	if (nchar>nxegment*XEGMENT || (nxegment-1)*XEGMENT>=nchar) {
		ptext[nxegment*XEGMENT-1]='\0';
		unsigned long nxegment_old=nxegment;
		nxegment=nchar/XEGMENT+!!(nchar%XEGMENT);
		char* pnew=new char[nxegment*XEGMENT];
		if (!pnew) fail();
		if (nxegment>nxegment_old || strlen(ptext)<nxegment*XEGMENT) strcpy(pnew,ptext);
		else pnew[nxegment*XEGMENT-1]='\0';
		delete[] ptext;
		ptext=pnew;
	}
}

void xtring::expand(unsigned long nchar) {

	if (nchar>=nxegment*XEGMENT) {
		nxegment=nchar/XEGMENT+1;
		char* pnew=new char[nxegment*XEGMENT];
		if (!pnew) fail();
		unsigned long i;
		for (i=0;i<nchar;i++)
			pnew[i]=ptext[i];
		pnew[i]='\0';
		delete[] ptext;
		ptext=pnew;
	}
}

xtring::xtring(const xtring& s) {

	// COPY CONSTRUCTOR

	init();
	resize(strlen(s.ptext));
	strcpy(ptext,s.ptext);
}

xtring::xtring() {

	// CONSTRUCTOR: xtring s;

	init();
	*ptext='\0';
}

xtring::xtring(char* inittext) {

	// CONSTRUCTOR: xtring s="text";

	init();
	resize(strlen(inittext));
	strcpy(ptext,inittext);
}

xtring::xtring(const char* inittext) {

	// CONSTRUCTOR: xtring s="text";

	init();
	resize(strlen(inittext));
	strcpy(ptext,inittext);
}

xtring::xtring(char c) {

	// CONSTRUCTOR: xtring s='c';

	init();
	ptext[0]=c;
	ptext[1]='\0';
}

xtring::xtring(unsigned long n) {

	// CONSTRUCTOR: xtring s[n]

	init();
	resize(n);
	*ptext='\0';
}

xtring::xtring(int n) {

	// CONSTRUCTOR: xtring s[n]

	init();
	resize(n);
	*ptext='\0';
}

xtring::xtring(unsigned int n) {

	// CONSTRUCTOR: xtring s[n]

	init();
	resize(n);
	*ptext='\0';
}

xtring::xtring(long n) {

	// CONSTRUCTOR: xtring s[n]

	init();
	resize(n);
	*ptext='\0';
}

xtring::~xtring() {

	// DESTRUCTOR

	if (buf) delete[] buf;
	delete[] ptext;
}

xtring::operator char*() {

	// Conversion function (cast operator) to char*

	return ptext;
}

xtring::operator const char*() const {

	// Conversion function (cast operator) to const char*

	return ptext;
}

void xtring::reserve(unsigned long n) {

	// Set string buffer length to at least n (not including trailing \0)

	resize(n);
}


unsigned long xtring::len() {

	// String length not including trailing \0

	return strlen(ptext);
}

xtring xtring::upper() {

	// Upper case

	if (buf) delete[] buf;
	buf=new char[len()+1];
	if (!buf) fail();
	char* pold=ptext,*pnew=buf;
	do {
		if (*pold>='a' && *pold<='z') *pnew=*pold-32;
		else *pnew=*pold;
		pnew++;
	} while (*pold++);
	return (xtring)buf;
}

xtring xtring::lower() {

	// Lower case

	if (buf) delete[] buf;
	buf=new char[len()+1];
	if (!buf) fail();
	char* pold=ptext,*pnew=buf;
	do {
		if (*pold>='A' && *pold<='Z') *pnew=*pold+32;
		else *pnew=*pold;
		pnew++;
	} while (*pold++);
	return (xtring)buf;
}

xtring xtring::printable() {

	// Printable characters (ASCII code >=32)

	if (buf) delete[] buf;
	buf=new char[len()+1];
	if (!buf) fail();
	char* pold=ptext,*pnew=buf;
	while (*pold) {
		if (*pold>=' ') *pnew++=*pold;
		pold++;
	}
	*pnew='\0';
	return (xtring)buf;
}

xtring xtring::left(unsigned long n) {

	// Leftmost n characters

	if (buf) delete[] buf;
	if (n<=0) {
		buf=new char[1];
		if (!buf) fail();
		*buf='\0';
	}
	else {
		buf=new char[n+1];
		if (!buf) fail();
		char* pold=ptext,*pnew=buf;
		unsigned long i=0;
		while (i<n && *pold) {
			*pnew=*pold;
			pold++;
			pnew++;
			i++;
		}
		*pnew='\0';
	}
	return (xtring)buf;
}

xtring xtring::mid(unsigned long s) {

	// Rightmost portion of xtring, starting at character s

	if (buf) delete[] buf;
	if (s>=len()) {
		buf=new char[1];
		if (!buf) fail();
		*buf='\0';
	}
	else {
		buf=new char[len()-s+1];
		if (!buf) fail();
		strcpy(buf,ptext+s);
	}
	return (xtring)buf;
}

xtring xtring::mid(unsigned long s,unsigned long n) {

	// Middle n characters starting at character s

	if (buf) delete[] buf;
	if (s>=len() || n<=0) {
		buf=new char[1];
		if (!buf) fail();
		*buf='\0';
	}
	else {
		buf=new char[n+1];
		if (!buf) fail();
		char* pold=ptext+s,*pnew=buf;
		unsigned long i=0;
		while (i++<n && *pold) *pnew++=*pold++;
		*pnew='\0';
	}
	return (xtring)buf;
}


xtring xtring::right(unsigned long n) {

	// Rightmost n characters

	unsigned long length = len();

	unsigned long s = 0;

	if (n < length) {
		s = length - n;
	}
	return mid(s);
}

long xtring::find(const char* s) {

	// Find string s in this xtring
	// (returns -1 if string does not occur)

	long length=strlen(s);
	long bound=len()-length;
	long i=0;
	while (i<=bound) {
		xtring sub=mid(i,length);
		if (sub==s) return i;
		i++;
	}
	return -1;
}

long xtring::find(char c) {

	// Find character in this xtring

	long i;
	long bound=len();

	for (i=0;i<bound;i++)
		if (ptext[i]==c) return i;

	return -1;
}

long xtring::findoneof(const char* s) {

	// Find one of several characters in this xtring

	long i,j;
	long bound=len();
	long nchar=strlen(s);

	for (i=0;i<bound;i++)
		for (j=0;j<nchar;j++)
			if (ptext[i]==s[j]) return i;

	return -1;
}

long xtring::findnotoneof(const char* s) {

	// Find first character NOT matching characters in s

	long i,j;
	long bound=len();
	long nchar=strlen(s);
	char found;

	for (i=0;i<bound;i++) {
		found=0;
		for (j=0;j<nchar && !found;j++)
			if (ptext[i]==s[j]) found=1;
		if (!found) return i;
	}
	return -1;
}

double xtring::num() {

	// Conversion to number

	char* endptr;
	double dval;

	dval=strtod(ptext,&endptr);
	if (*endptr) return 0;
	return dval;
}

char xtring::isnum() {

	// Whether xtring can be converted to a number

	char* endptr;

	strtod(ptext,&endptr);
	if (*endptr) return 0;
	return 1;
}

void xtring::printf(const char* fmt,...) {

	va_list v;
	va_start(v,fmt);

	const int MINBUF=100;
	xtring sect;
	xtring output="";
	xtring fspec;
	xtring wspec;
	xtring buffer(MINBUF);
	const char* pfmt=fmt;
	char* pchar;
	int i;
	char waitspec=0,readspec=0,readwidth,havewidth;

	i=0;
	while (*pfmt) {
		if (waitspec) {
			if ((*pfmt>='0' && *pfmt<='9') || *pfmt=='-' || *pfmt=='+' || *pfmt==' ' ||
			     *pfmt=='#' || *pfmt=='h' || *pfmt=='l' || *pfmt=='I' || *pfmt=='L' ||
			     *pfmt=='c' || *pfmt=='C' || *pfmt=='d' || *pfmt=='i' || *pfmt=='o' ||
			     *pfmt=='u' || *pfmt=='x' || *pfmt=='X' || *pfmt=='e' || *pfmt=='E' ||
			     *pfmt=='f' || *pfmt=='g' || *pfmt=='G' || *pfmt=='n' || *pfmt=='p' ||
			     *pfmt=='s' || *pfmt=='S' || *pfmt=='.') {
				readspec=1;
				havewidth=0;
				readwidth=0;
				wspec="0";
				fspec="%";
				pfmt--;
			}
			else output+=*pfmt;
			waitspec=0;
		}
		else if (readspec) {
			if (*pfmt!='*') fspec+=*pfmt;
			if (readwidth) {
				if (*pfmt>='0' && *pfmt<='9') wspec+=*pfmt;
				else {
					readwidth=0;
					havewidth=1;
				}
			}
			else if (*pfmt>='0' && *pfmt<='9' && !havewidth) {
				wspec=*pfmt;
				readwidth=1;
			}
			else if (*pfmt=='*' && !havewidth) {
				wspec.reserve(MINBUF);
				sprintf(wspec,"%d",va_arg(v,int));
				fspec+=wspec;
				havewidth=1;
			}
			else if (*pfmt!='-' && *pfmt!='+' && *pfmt!=' ' && *pfmt!='#') {
				havewidth=1;
			}
			if (*pfmt=='c' || *pfmt=='C' || *pfmt=='d' || *pfmt=='i' || *pfmt=='o' ||
				*pfmt=='u' || *pfmt=='x' || *pfmt=='X') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,int));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='e' || *pfmt=='E' || *pfmt=='f' || *pfmt=='g' ||
				*pfmt=='G') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,double));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='p') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,void*));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='s' || *pfmt=='S') {
				pchar=va_arg(v,char*);
				buffer.reserve((unsigned long)(strlen(pchar)+wspec.num()));
				sprintf(buffer,fspec,pchar);
				output+=buffer;
				readspec=0;
			}
		}
		else if (*pfmt=='%') waitspec=1;
		else {
			sect[i++]=*pfmt;
			if (*(pfmt+1)=='%' || !*(pfmt+1)) {
				sect[i]='\0';
				output+=sect;
				i=0;
			}
		}
		pfmt++;
	}
	resize(output.len());
	strcpy(ptext,output);
}

xtring& xtring::operator=(xtring& s) {

	// Assignment: s1=s2;

	resize(s.len());
	strcpy(ptext,s.ptext);
	return *this;
}

xtring& xtring::operator=(const char* s) {

	// Assignment: s="text"

	resize(strlen(s));
	strcpy(ptext,s);
	return *this;
}

xtring& xtring::operator=(char c) {

	// Assignment: s='c'

	resize(1);
	ptext[0]=c;
	ptext[1]='\0';
	return *this;
}

xtring xtring::operator+(xtring& s2) {

	// Concatenate: s1+s2

	if (buf) delete[] buf;
	buf=new char[len()+s2.len()+1];
	if (!buf) fail();
	strcpy(buf,ptext);
	strcat(buf,s2.ptext);
	return (xtring)buf;
}

xtring xtring::operator+(const char* s2) {

	// Concatenate: s1+"text"

	if (buf) delete[] buf;
	buf=new char[len()+strlen(s2)+1];
	if (!buf) fail();
	strcpy(buf,ptext);
	strcat(buf,s2);
	return xtring(buf);
}

xtring xtring::operator+(char c) {

	// Concatenate s1+'c'

	return (xtring)ptext+(xtring)c;
}

xtring& xtring::operator+=(xtring& s2) {

	// Concatenate s1+=s2;

	resize(len()+s2.len());
	strcat(ptext,s2.ptext);
	return *this;
}

xtring& xtring::operator+=(const char* s2) {

	// Concatenate s1+="text";

	resize(len()+strlen(s2));
	strcat(ptext,s2);
	return *this;
}

xtring& xtring::operator+=(char c) {

	// Concatenate s1+='c';

	resize(len()+1);
	strcat(ptext,(xtring)c);
	return *this;
}

char& xtring::operator[](unsigned long n) {

	// Reference s[n] (NB: resizes xtring if necessary)

	expand(n);
	return ptext[n];
}

char& xtring::operator[](int n) {

	expand(n);
	return ptext[(unsigned long)n];
}

char& xtring::operator[](unsigned int n) {

	expand(n);
	return ptext[(unsigned long)n];
}

char& xtring::operator[](long n) {

	expand(n);
	return ptext[(unsigned long)n];
}

// Comparison operators

bool xtring::operator==(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)) return false;
	return true;
}

bool xtring::operator==(char* s2) const {

	if (strcmp(ptext,s2)) return false;
	return true;
}

bool xtring::operator==(const char* s2) const {

	if (strcmp(ptext,s2)) return false;
	return true;
}

bool xtring::operator==(char c) const {

	if (strcmp(ptext,(xtring)c)) return false;
	return true;
}

bool xtring::operator!=(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)) return true;
	return false;
}

bool xtring::operator!=(char* s2) const {

	if (strcmp(ptext,s2)) return true;
	return false;
}

bool xtring::operator!=(const char* s2) const {

	if (strcmp(ptext,s2)) return true;
	return false;
}

bool xtring::operator!=(char c) const {

	if (strcmp(ptext,(xtring)c)) return true;
	return false;
}

bool xtring::operator<(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)<0) return true;
	return false;
}

bool xtring::operator<(char* s2) const {

	if (strcmp(ptext,s2)<0) return true;
	return false;
}

bool xtring::operator<(const char* s2) const {

	if (strcmp(ptext,s2)<0) return true;
	return false;
}

bool xtring::operator<(char c) const {

	if (strcmp(ptext,(xtring)c)<0) return true;
	return false;
}

bool xtring::operator>(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)>0) return true;
	return false;
}

bool xtring::operator>(char* s2) const {

	if (strcmp(ptext,s2)>0) return true;
	return false;
}

bool xtring::operator>(const char* s2) const {

	if (strcmp(ptext,s2)>0) return true;
	return false;
}

bool xtring::operator>(char c) const {

	if (strcmp(ptext,(xtring)c)>0) return true;
	return false;
}

bool xtring::operator<=(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)<=0) return true;
	return false;
}

bool xtring::operator<=(char* s2) const {

	if (strcmp(ptext,s2)<=0) return true;
	return false;
}

bool xtring::operator<=(const char* s2) const {

	if (strcmp(ptext,s2)<=0) return true;
	return false;
}

bool xtring::operator<=(char c) const {

	if (strcmp(ptext,(xtring)c)<=0) return true;
	return false;
}

bool xtring::operator>=(xtring& s2) const {

	if (strcmp(ptext,s2.ptext)>=0) return true;
	return false;
}

bool xtring::operator>=(char* s2) const {

	if (strcmp(ptext,s2)>=0) return true;
	return false;
}

bool xtring::operator>=(const char* s2) const {

	if (strcmp(ptext,s2)>=0) return true;
	return false;
}

bool xtring::operator>=(char c) const {

	if (strcmp(ptext,(xtring)c)>=0) return true;
	return false;
}


void unixtime(xtring& result) {

	time_t t;
	time(&t);
	result=ctime(&t);
	result=result.left(result.findoneof("\n"));
}


FILE** pin;
xtring inxtr;
bool iseol=false;
char cbuf;

inline char xgetc() {

	// Equivalent to getc(*pin), but returns character stored in cbuf (global)
	// if there is one (see xungetc below)

	char copy;

	if (cbuf) {
		copy=cbuf;
		cbuf=0;
		return copy;
	}
	return getc(*pin);
}

inline void xungetc(char& ch) {

	// Equivalent to ungetc(ch,*pin), but assigns ch to cbuf instead of trying
	// to push it back onto the stream (which may not be possible)

	cbuf=ch;
}

inline bool xfeof() {

	// Equivalent to feof(*pin), but returns false if character stored in cbuf
	// even if feof(*pin)==true

	if (feof(*pin) && !cbuf) return true;
	return false;
}

inline int readfixedwidth(int& width) {

	// Reads the next 'width' characters from stream in
	// Returns the number of characters successfully read (may be <width on eof)

	int cread=0;
	char ch;

	iseol=false;

	inxtr.reserve(width);
	if (xfeof()) {
		inxtr[cread]='\0';
		iseol=1;
		return 0;
	}

	while (cread<width) {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			inxtr[cread]='\0';
			iseol=1;
			return cread;
		}
		inxtr[cread++]=ch;
	}
	inxtr[cread]='\0';
	return cread;
}

inline int readtoeol(bool breakcomma) {

	// Reads to end of the current line or eof from stream in
	// Returns number of characters read

	int cread=0;
	char ch;

	iseol=false;

	do {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			inxtr[cread]='\0';
			iseol=true;
			return cread;
		}
		else if (breakcomma && ch==',') {
			inxtr[cread]='\0';
			return cread;
		}
		inxtr[cread++]=ch;
	} while (true);
}

inline void readtocomma() {

	// Reads to next comma or next non-white-space character,
	// whichever comes first (newline or eof always terminates input)

	char ch;
	iseol=false;

	do {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			iseol=true;
			return;
		}
		if (ch==',') return;
		if (ch!=' ' && ch!='\t') {
			xungetc(ch);
			return;
		}
	} while (true);
}

inline int readtowhitespace(bool& breakcomma) {

	// Reads to next space, tab, newline or eof from stream in
	// Space or tab is not read in (returned to stream)
	// Returns number of characters read

	int cread=0;
	char ch;

	iseol=false;

	do {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			iseol=true;
			inxtr[cread]='\0';
			return cread;
		}
		if (breakcomma) {
			if (ch==' ' || ch=='\t') {
				inxtr[cread]='\0';
				readtocomma();
				return cread;
			}
			if (ch==',') {
				inxtr[cread]='\0';
				return cread;
			}
		}
		else {
			if (ch==' ' || ch=='\t') {
				xungetc(ch);
				inxtr[cread]='\0';
				return cread;
			}
		}
		inxtr[cread++]=ch;
	} while (true);
}


inline int readtochar(char sep) {

	// Reads to next instance of character 'sep'
	// Character sep is lost to the stream, but not stored as part of the input string
	// Newline or eof also terminate input
	// Returns number of characters read

	int cread=0;
	char ch;

	iseol=false;

	do {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			iseol=true;
			inxtr[cread]='\0';
			return cread;
		}
		if (ch==sep) {
			inxtr[cread]='\0';
			return cread;
		}
		inxtr[cread++]=ch;
	} while (true);
}

inline void skipwhitespace() {

	// Reads to next non-white-space character
	// Newline or eof also terminate input

	iseol=false;
	char ch;

	do {
		ch=xgetc();
		if (xfeof() || ch=='\n') {
			iseol=true;
			return;
		}
		if (ch!=' ' && ch!='\t') {
			xungetc(ch);
			return;
		}
	} while (true);
}

bool readfloat(int nitem,int& width,int dec,int exp,char termch,double* parg) {

	int i,j,len;
	char ch;
	bool havesign;
	bool negcard;
	bool haveexpsign;
	bool havedec;
	bool haveexp;
	bool negexp;

	double cardpart;
	double decpart;
	double decmul;
	double value;
	int exppart;
	int posdec;
	int posexp;

	bool breakcomma=termch==',';

	if (termch==',' || termch=='$') termch=0;

	do {

		if (xfeof()) return false;

		havesign=false;
		negcard=false;
		haveexpsign=false;
		negexp=false;
		havedec=false;
		haveexp=false;
		cardpart=0;
		decpart=0;
		decmul=1.0;
		exppart=0;

		if (width) {
			readfixedwidth(width);
		}
		else { // no width specified, read to next white space or end of line
			if (!iseol) skipwhitespace();
			if (iseol) {
				*parg++=0.0;
				goto next;
			}
			if (termch) readtochar(termch);
			else readtowhitespace(breakcomma);
		}

		len=inxtr.len();

		if (dec>=0 || exp>=0) { // strip non-numeric characters from string
			i=j=0;
			ch=inxtr[j];
			posdec=posexp=-1;
			while (ch) {
				if (ch=='e' || ch=='E') {
					if (posexp<0) {
						inxtr[i++]=inxtr[j++];
						posexp=i;
					}
					else j++;
				}
				else if (ch=='.') {
					if (posexp<0 && posdec<0) {
						inxtr[i++]=inxtr[j++];
						posdec=i;
					}
					else j++;
				}
				else if (ch=='-' || (ch>='0' && ch<='9'))
					inxtr[i++]=inxtr[j++];
				else j++;
				ch=inxtr[j];
			}
			inxtr[i]='\0';

			len=inxtr.len();
			if (exp>=0 && posexp<0 && posdec<len-exp) posexp=len-exp;
			else posexp=len;

			if (dec>=0 && posdec<0) posdec=posexp-dec;
			else posdec=posexp;
		}
		else posdec=posexp=len;

		len=inxtr.len();
		for (i=0;i<len;i++) {
			ch=inxtr[i];
			if (i<posdec) { // cardinal part
				if (ch=='-' && !havesign) negcard=true;
				else if (ch=='.' && !havedec && !haveexp) {
					posdec=i+1;
					if (posexp<posdec) posexp=posdec;
					havedec=true;
				}
				else if ((ch=='E' || ch=='e') && !haveexp) {
					posexp=i+1;
					posdec=i+1;
					haveexp=true;
				}
				else if (ch>='0' && ch<='9') {
					havesign=true;
					cardpart=cardpart*10.0+(double)(ch-'0');
				}
			}
			else if (i<posexp) { // decimal part
				if ((ch=='E' || ch=='e') && !haveexp) {
					posexp=i+1;
					posdec=i+1;
					haveexp=true;
				}
				else if (ch>='0' && ch<='9') {
					decmul/=10.0;
					decpart+=(double)(ch-'0')*decmul;
				}
			}
			else { // exponent part
				if (ch=='-' && !haveexpsign) negexp=true;
				else if (ch>='0' && ch<='9') {
					haveexpsign=true;
					exppart=exppart*10+(ch-'0');
				}
			}
		}
		if (negcard)
			value=-cardpart-decpart;
		else
			value=cardpart+decpart;

		if (exppart) {
			if (negexp)
				inxtr.printf("1E-%d",exppart);
			else
				inxtr.printf("1E%d",exppart);
			value*=inxtr.num();
		}
		*parg++=value;
next:
		nitem--;
	} while (nitem>0);
	return true;
}

bool readint(int& nitem,int& width,char termch,int* parg) {

	bool havesign;
	bool neg;
	int value;
	int len,i;
	char ch;

	bool breakcomma=termch==',';

	if (termch==',' || termch=='$') termch=0;

	do {

		if (xfeof()) return false;
		havesign=false;
		neg=false;
		value=0;

		if (width) {
			readfixedwidth(width);
		}
		else { // no width specified, read to next white space or end of line
			if (!iseol) skipwhitespace();
			if (iseol) {
				*parg++=0;
				goto next;
			}
			if (termch) readtochar(termch);
			else readtowhitespace(breakcomma);
		}

		len=inxtr.len();
		for (i=0;i<len;i++) {
			ch=inxtr[i];
			if (ch=='-' && !havesign) {
				havesign=true;
				neg=true;
			}
			else if (ch>='0' && ch<='9') {
				havesign=true;
				value=value*10+ch-'0';
			}
		}

		if (neg) value=-value;

		*parg++=value;
next:
		nitem--;
	} while (nitem>0);

	return true;
}

bool readchar(int nitem,int& width,char termch,xtring* parg,bool read_to_eol) {

	bool nomulti=nitem>1 && width<0;

	bool breakcomma=termch==',';

	if (termch==',' || termch=='$') termch=0;

	do {
		if (xfeof()) return false;

		if (width) {
			readfixedwidth(width);
		}
		else {
			if (!iseol) skipwhitespace();
			if (iseol) {
				*parg++="";
				goto next;
			}
			if (termch) readtochar(termch);
			else if (read_to_eol) readtoeol(breakcomma);
			else readtowhitespace(breakcomma);
		}

		*parg++=inxtr;
next:
		nitem--;

		while (nomulti && nitem>0) {
			// Disallow multiple strings if width not specified
			// Set extra arguments to blank

			*parg++="";
		}
	} while (nitem>0);

	return true;
}


bool readfor(FILE* in, const char* fmt, ...) {

	va_list v;
	va_start(v,fmt);

	bool waitwidth=false;
	bool waitfloat=false;
	bool waitdec=false;
	bool waitexp=false;
	bool waitint=false;
	bool waitchar=false;
	bool waitcomma=false;
	bool croff=false;
	bool read_to_eol=false;
	const char* pchar = fmt;
	int nitem=0;
	int width=0;
	int dec=0;
	int exp=0;
	iseol=false;
	pin=&in;

	cbuf=0;

	while (*pchar) {
		if (*pchar!=' ') {
			if (*pchar!='$' && *pchar!='#') croff=read_to_eol=false;
			else if (*pchar=='$') croff=true;
			else if (*pchar=='#') read_to_eol=true;
			if (waitwidth) {
				if (*pchar>='0' && *pchar<='9') { // width specifier
					width=width*10+*pchar-'0';
				}
				else if (*pchar=='.' && waitfloat) {
					waitdec=true;
					waitwidth=false;
				}
				else if ((*pchar=='e' || *pchar=='E') && waitfloat) {
					dec=0;
					waitexp=true;
					waitwidth=false;
				}
				else if (*pchar=='/') {
					if (waitfloat) {
						if (!readfloat(nitem,width,-1,-1,0,va_arg(v,double*))) return false;
						waitfloat=false;
						nitem=0;
						width=0;
					}
					else if (waitint) {
						if (!readint(nitem,width,0,va_arg(v,int*))) return false;
						waitint=false;
						nitem=0;
						width=0;
					}
					else {
						if (!readchar(nitem,width,0,va_arg(v,xtring*),read_to_eol)) return false;
						waitchar=false;
						nitem=0;
						width=0;
					}
					if (!iseol) readtoeol(false);
					iseol=false;
					waitwidth=false;
				}
				else if (waitfloat) {
					if (!readfloat(nitem,width,-1,-1,*pchar,va_arg(v,double*))) return false;
					waitfloat=false;
					waitwidth=false;
					nitem=0;
					width=0;
				}
				else if (waitint) {
					if (!readint(nitem,width,*pchar,va_arg(v,int*))) return false;
					waitint=false;
					waitwidth=false;
					nitem=0;
					width=0;
				}
				else {
					if (!readchar(nitem,width,*pchar,va_arg(v,xtring*),read_to_eol)) return false;
					waitchar=false;
					waitwidth=false;
					nitem=0;
					width=0;
				}
			}
			else if (waitdec) {
				if (*pchar>='0' && *pchar<='9') { // decimal places specifier
					dec=dec*10+*pchar-'0';
				}
				else if (*pchar=='e' || *pchar=='E') {
					waitexp=true;
					waitdec=false;
				}
				else if (*pchar=='/') {
					if (!readfloat(nitem,width,dec,-1,0,va_arg(v,double*))) return false;
					if (!iseol) readtoeol(false);
					iseol=false;
					waitdec=false;
					waitfloat=false;
					nitem=0;
					width=0;
					dec=0;
				}
				else {
					if (!readfloat(nitem,width,dec,-1,*pchar,va_arg(v,double*))) return false;
					waitdec=false;
					waitfloat=false;
					nitem=0;
					width=0;
					dec=0;
				}
			}
			else if (waitexp) {
				if (*pchar>='0' && *pchar<='9') { // exponent width specifier
					exp=exp*10+*pchar-'0';
				}
				else if (*pchar=='/') {
					if (!readfloat(nitem,width,dec,exp,0,va_arg(v,double*))) return false;
					if (!iseol) readtoeol(false);
					iseol=false;
					waitexp=false;
					waitfloat=false;
					nitem=0;
					width=0;
					dec=0;
					exp=0;
				}
				else {
					if (!readfloat(nitem,width,dec,exp,*pchar,va_arg(v,double*))) return false;
					waitexp=false;
					waitfloat=false;
					nitem=0;
					width=0;
					dec=0;
					exp=0;
				}
			}
			else if (waitcomma) {
				waitcomma=false;
			}
			else {
				if (*pchar>='0' && *pchar<='9') { // number of items specifier
					nitem=nitem*10+*pchar-'0';
				}
				else if (*pchar=='f' || *pchar=='F') { // floating point number
					waitfloat=true;
					waitwidth=true;
				}
				else if (*pchar=='i' || *pchar=='I') { // integer
					waitint=true;
					waitwidth=true;
				}
				else if (*pchar=='A' || *pchar=='a') { // character string
					waitchar=true;
					waitwidth=true;
				}
				else if (*pchar=='/') { // to end of line
					readtoeol(false);
					iseol=false;
				}
				else if (*pchar=='X' || *pchar=='x') { // skip characters
					readfixedwidth(nitem);
					nitem=0;
					waitcomma=true;
				}
				else if (*pchar!='$') { // take any other character (including a comma) as a separator specifier
					readtochar(*pchar);
				}
			}
		}
		pchar++;
	}
	if (waitfloat) {
		if (waitexp) {
			if (!readfloat(nitem,width,dec,exp,0,va_arg(v,double*))) return false;
		}
		else if (waitdec) {
			if (!readfloat(nitem,width,dec,-1,0,va_arg(v,double*))) return false;
		}
		else {
			if (!readfloat(nitem,width,-1,-1,0,va_arg(v,double*))) return false;
		}
	}
	else if (waitint) {
		if (!readint(nitem,width,0,va_arg(v,int*))) return false;
	}
	else if (waitchar) {
		if (!readchar(nitem,width,0,va_arg(v,xtring*),read_to_eol)) return false;
	}

	if (!iseol && !croff) readtoeol(false);
	return true;
}


void formatf(xtring& output,char* format,va_list& v) {


	const int MINBUF=100;
	char* pchar;
	xtring sect;
	xtring fspec;
	xtring wspec;
	xtring buffer(MINBUF);
	char* pfmt=(char*)format;
	int i;
	char waitspec=0,readspec=0,readwidth,havewidth;

	output="";
	i=0;
	while (*pfmt) {
		if (waitspec) {
			if ((*pfmt>='0' && *pfmt<='9') || *pfmt=='-' || *pfmt=='+' || *pfmt==' ' ||
			     *pfmt=='#' || *pfmt=='h' || *pfmt=='l' || *pfmt=='I' || *pfmt=='L' ||
			     *pfmt=='c' || *pfmt=='C' || *pfmt=='d' || *pfmt=='i' || *pfmt=='o' ||
			     *pfmt=='u' || *pfmt=='x' || *pfmt=='X' || *pfmt=='e' || *pfmt=='E' ||
			     *pfmt=='f' || *pfmt=='g' || *pfmt=='G' || *pfmt=='n' || *pfmt=='p' ||
			     *pfmt=='s' || *pfmt=='S' || *pfmt=='.') {
				readspec=1;
				havewidth=0;
				readwidth=0;
				wspec="0";
				fspec="%";
				pfmt--;
			}
			else output+=*pfmt;
			waitspec=0;
		}
		else if (readspec) {
			if (*pfmt!='*') fspec+=*pfmt;
			if (readwidth) {
				if (*pfmt>='0' && *pfmt<='9') wspec+=*pfmt;
				else {
					readwidth=0;
					havewidth=1;
				}
			}
			else if (*pfmt>='0' && *pfmt<='9' && !havewidth) {
				wspec=*pfmt;
				readwidth=1;
			}
			else if (*pfmt=='*' && !havewidth) {
				wspec.reserve(MINBUF);
				sprintf(wspec,"%d",va_arg(v,int));
				fspec+=wspec;
				havewidth=1;
			}
			else if (*pfmt!='-' && *pfmt!='+' && *pfmt!=' ' && *pfmt!='#') {
				havewidth=1;
			}
			if (*pfmt=='c' || *pfmt=='C' || *pfmt=='d' || *pfmt=='i' || *pfmt=='o' ||
				*pfmt=='u' || *pfmt=='x' || *pfmt=='X') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,int));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='e' || *pfmt=='E' || *pfmt=='f' || *pfmt=='g' ||
				*pfmt=='G') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,double));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='p') {
				buffer.reserve((unsigned long)(MINBUF+wspec.num()));
				sprintf(buffer,fspec,va_arg(v,void*));
				output+=buffer;
				readspec=0;
			}
			else if (*pfmt=='s' || *pfmt=='S') {
				pchar=va_arg(v,char*);
				buffer.reserve((unsigned long)(strlen(pchar)+wspec.num()));
				sprintf(buffer,fspec,pchar);
				output+=buffer;
				readspec=0;
			}
		}
		else if (*pfmt=='%') waitspec=1;
		else {
			sect[i++]=*pfmt;
			if (*(pfmt+1)=='%' || !*(pfmt+1)) {
				sect[i]='\0';
				output+=sect;
				i=0;
			}
		}
		pfmt++;
	}
}


bool fileexists(const xtring& filename) {

	// Returns true if filename exists

	FILE* handle;
	handle=fopen(filename,"r+t");
	if (!handle) return false;
	fclose(handle);
	return true;
}
