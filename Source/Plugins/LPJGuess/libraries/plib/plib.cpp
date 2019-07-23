///////////////////////////////////////////////////////////////////////////////////////
/// \file plib.cpp
/// \brief PLIB Version 2.1 (Fully portable version)
///
/// Documentation: see header file plib.h
///
/// Enquiries to: Joe Siltberg, Lund University: joe.siltberg@nateko.lu.se
/// All rights reserved, copyright retained by the author.
///
/// \author Ben Smith, University of Lund
/// $Date: 2013-09-03 11:06:34 +0200 (Tue, 03 Sep 2013) $

#include "stdafx.h"
#include "plib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>
#include "recursivefilereader.h"

enum itemtype {PLIB_XTRING,PLIB_STDSTRING,PLIB_INT,PLIB_DOUBLE,PLIB_SET,
	PLIB_BOOLEAN,PLIB_FLAG};
enum wordtype {WORD_IDENTIFIER,WORD_STRING,WORD_NUMBER,WORD_OPENPARENTHESIS,
	WORD_CLOSEPARENTHESIS,WORD_GROUP,WORD_NOWORD};
enum errortype {PLIB_NOERROR,PLIB_INVALIDNUMBER,PLIB_OUTOFMEMORY,PLIB_ENDOFFILE,
	PLIB_PARENTHESISNESTING,PLIB_GROUPHEADER,PLIB_DUPLICATEGROUPNAME,PLIB_FILEOPEN,
	PLIB_EXPECTIDENTIFIER,PLIB_UNDEFINEDITEM,PLIB_EXPECTSTRING,PLIB_EXPECTNUMBER,
	PLIB_OUTOFRANGE,PLIB_EXPECTOPENPARENTHESIS,PLIB_ABORT,PLIB_UNRECOGNISEDTOKEN};

#ifdef MAXWORD
#undef MAXWORD
#endif

struct Plibitem {

	itemtype type;
	xtring identifier;
	void* param;
	double min;
	double max;
	union {
		int nparam;
		int maxlen;
		int id;
	};
	int callback;
	bool called;
};

class Pliblist {

public:
	int callback;
	struct itemkey {
		Plibitem* pitem;
		itemkey* pprev;
		itemkey* pnext;
	};

private:
	itemkey* m_pfirst;
	itemkey* m_pthis;

public:
	Pliblist();

	bool addtostart(Plibitem& pitem);
	Plibitem* getfirstitem();
	Plibitem* getnextitem();
	void removeitem();
};

struct Plibword {

	wordtype type;
	xtring string;
	union {
		double num;
		Plibword* pgroup;
	};
	xtring filename;
	int lineno;
	Plibword* pnext;
};

struct Plibgroup {

	xtring name;
	Plibword* pfirst;
	Plibgroup* pnext;
};

static const int MAXWORD=499;
	// maximum number of characters (excluding terminating \0) for a word in script file

static Pliblist* pthislist;
static char currch;

static xtring lastfile;
static xtring lastword;
static int lastline=0;

static bool ishelp=false; // true if call is to function plibhelp, otherwise false

static bool abortparse=false;

Pliblist::Pliblist() {

	m_pfirst=m_pthis=NULL;
}

bool Pliblist::addtostart(Plibitem& pitem) {

	itemkey* ik;

	ik=new itemkey;
	if (!ik) return false;
	ik->pitem=new Plibitem;
	if (!ik->pitem) return false;
	*(ik->pitem)=pitem;
	ik->pnext=m_pfirst;
	ik->pprev=NULL;
	if (m_pfirst) m_pfirst->pprev=ik;
	m_pfirst=ik;
	m_pthis=ik;
	return true;
}

Plibitem* Pliblist::getfirstitem() {

	if (!m_pfirst) return NULL;
	m_pthis=m_pfirst;
	return m_pthis->pitem;
}

Plibitem* Pliblist::getnextitem() {

	if (!m_pthis) return NULL;
	m_pthis=m_pthis->pnext;
	if (!m_pthis) return NULL;
	return m_pthis->pitem;
}

void Pliblist::removeitem() {

	if (!m_pthis) return;
	if (m_pthis->pnext) m_pthis->pnext->pprev=m_pthis->pprev;
	if (m_pthis->pprev) m_pthis->pprev->pnext=m_pthis->pnext;
	if (m_pthis==m_pfirst) m_pfirst=m_pthis->pnext;
	delete m_pthis->pitem;
	delete m_pthis;
	m_pthis=NULL;
}

void helpheader(char* blockname) {

	// Called by helpout and block header overload of declareitem to output
	// header string for help text for block with specified name

	xtring output=(xtring)"\nKeywords defined within block \""+blockname+"\":\n\n";
	plib_receivemessage(output);
}

void helpout(xtring& identifier, xtring& help) {

	// Called by declareitem to output help text if requested (by a call to plibhelp)

	const char* TAB = "\n                ";
	xtring output;
	output.printf("%-15s", (char*)identifier);
	output += identifier.len() > 15 ? TAB : " ";

	xtring helpcpy = "(no help available)";
	if (*help) {
		const int MAXHELPSTRING = 63;
		helpcpy = help.left(MAXHELPSTRING);
		div_t q = div(help.len(), MAXHELPSTRING);
		for (int l=1; l < (q.quot + int(q.rem > 0)); l++) {
			helpcpy += TAB;
			helpcpy += help.mid(l * MAXHELPSTRING, MAXHELPSTRING);
		}
	}
	plib_receivemessage(output + helpcpy + "\n");
}

bool declareitem(xtring identifier,xtring* param,int maxlen,int callback,xtring help) {

	// String with specified maximum length
	// e.g. title 'kiruna 20 67.5'

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_XTRING;
		item.param=param;
		item.maxlen=maxlen;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier, std::string* param, int maxlen, int callback, xtring help) {

	// String with specified maximum length
	// e.g. title 'kiruna 20 67.5'

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_STDSTRING;
		item.param=param;
		item.maxlen=maxlen;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier,int* param,int min,int max,int nparam,
	int callback,xtring help) {

	// Integer(s) in specified range
	// e.g. npat 10

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_INT;
		item.param=param;
		item.min=min;
		item.max=max;
		item.nparam=nparam;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier,double* param,double min,double max,
	int nparam,int callback,xtring help) {

	// Double(s) in specified range
	// e.g. soilawc 55 110

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_DOUBLE;
		item.param=param;
		item.min=min;
		item.max=max;
		item.nparam=nparam;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier,bool* param,int nparam,int callback,xtring help) {

	// Boolean(s) with parameter
	// e.g. include 1

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_BOOLEAN;
		item.param=param;
		item.nparam=nparam;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier,bool* param,int callback,xtring help) {

	// Boolean without parameter (flag)
	// e.g. anpp

	Plibitem item;

	if (ishelp) helpout(identifier,help);
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_FLAG;
		item.param=param;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

bool declareitem(xtring identifier,int id,int callback,xtring help) {

	// Block heading
	// e.g. taxon "picea" (

	Plibitem item;

	if (ishelp) {
		helpout(identifier,help);
		helpheader(identifier);
		plib_declarations(id,"");
	}
	else {
		item.identifier=identifier.lower();
		item.type=PLIB_SET;
		item.id=id;
		item.callback=callback;
		item.called=false;
		if (!pthislist->addtostart(item)) return false;
	}
	return true;
}

void callwhendone(int callback) {

	if (!ishelp) pthislist->callback=callback;
}

void nextch(RecursiveFileReader& in) {

	// Gets next 'valid' character on in
	// Ignored non-printable characters (<32, except \n)

	if (in.Feof()) {
		currch=' ';
		return;
	}
	do {
		currch=in.Fgetc();
	} while (currch<' ' && currch!='\n' && !in.Feof());
	if (in.Feof()) currch=' ';
}

void readto(xtring& string,char* termlist,RecursiveFileReader& in) {

	// Reads sequential characters on stream in up to first instance of one of
	// the characters in termlist, returning resultant string in string

	char* termptr;
	bool term=false;
	int len=0;

	do {
		termptr=termlist;
		while (*termptr)
			if (currch==*termptr++) term=true;
		if (in.Feof()) term=true;
		if (!term && len<MAXWORD) string[len++]=currch;
		if (!term) nextch(in);
	} while (!term);
	string[len]=0;

	lastword=string;
}

void getnextword(Plibword& plibword,RecursiveFileReader& in,errortype& err) {

	bool iscomment=false;
	xtring string;

	do {
		iscomment=false;
		if (!currch) nextch(in);
		while ((currch==' ' || currch=='\t' || currch=='\n') && !in.Feof()) nextch(in);
		if (in.Feof()) { // end of file
			plibword.type=WORD_NOWORD;
			plibword.lineno = 0;
			plibword.filename = "";
			err=PLIB_NOERROR; //PLIB_ENDOFFILE;
			return;
		}
		else {
			plibword.lineno = in.currentlineno();
			plibword.filename = in.currentfilename();
		}

		if ((currch>='a' && currch<='z') || (currch>='A' && currch<='Z')) { // identifier
			char termlist[]=" \t\n-+.\"\'()!";
			readto(string,termlist,in);
			plibword.string=string.lower();
			plibword.type=WORD_IDENTIFIER;
		}
		else if (currch=='\'' || currch=='\"') { // string
			xtring termlist=currch;
			nextch(in);
			readto(string,termlist,in);
			plibword.string=string;
			plibword.type=WORD_STRING;
			nextch(in);
		}
		else if (currch=='-' || currch=='+' || (currch>='0' && currch<='9') || currch=='.') { // number
			char termlist[]=" \t\n\"\'()!";
			readto(string,termlist,in);
			if (!string.isnum()) {
				err=PLIB_INVALIDNUMBER;
				return;
			}
			plibword.string=string;
			plibword.num=string.num();
			plibword.type=WORD_NUMBER;
		}
		else if (currch=='(') { // open parenthesis
			plibword.type=WORD_OPENPARENTHESIS;
			plibword.string="(";
			nextch(in);
		}
		else if (currch==')') { // close parenthesis
			plibword.type=WORD_CLOSEPARENTHESIS;
			plibword.string=")";
			nextch(in);
		}
		else if (currch=='!') { // comment
			nextch(in);
			while (currch!='\n' && !in.Feof()) nextch(in);
			iscomment=true;
		}
		else {
			err=PLIB_UNRECOGNISEDTOKEN;
			return;
		}
	} while (iscomment);
	err=PLIB_NOERROR;
}

bool findgroup(Plibgroup* pfirstgroup,xtring& name,Plibgroup& plibgroup) {

	// Looks for group with specified name
	// Returns false if group not in list

	Plibgroup* pgroup=pfirstgroup;

	while (pgroup) {
		if (pgroup->name==name) {
			plibgroup=*pgroup;
			return true;
		}
		pgroup=pgroup->pnext;
	}
	return false;
}

void loadscript(RecursiveFileReader& in,Plibword*& pfirstword,errortype& err,bool isgroup,
                Plibgroup*& pfirstgroup,bool& firstgroup,Plibgroup*& plastgroup,
                Plibword& plibword) {

	Plibword* pword;
	Plibword* pthisword=nullptr;
	bool firstword=true;
	bool groupheader=false;
	bool waitgroupname=false;
	bool importstatement=false;
	bool addword;
	Plibgroup plibgroup;
	//Plibgroup* pthisgroup;
	Plibgroup* pgroup;
	int parct=0;

	while (!in.Feof()) {
		getnextword(plibword,in,err);
		lastfile = plibword.filename;
		lastword=plibword.string;
		lastline=plibword.lineno;
		if (err) return;
		else {
			addword=true;
			if (groupheader) {
				if (waitgroupname) {
					if (plibword.type!=WORD_STRING) {
						err=PLIB_GROUPHEADER;
						return;
					}
					plibword.string=plibword.string.lower();
					if (findgroup(pfirstgroup,plibword.string,plibgroup)) {
						err=PLIB_DUPLICATEGROUPNAME;
						return;
					}
					plibgroup.name=plibword.string;
					waitgroupname=false;
				}
				else { // waiting for open parenthesis
					if (plibword.type!=WORD_OPENPARENTHESIS) {
						err=PLIB_GROUPHEADER;
						return;
					}
					//parct++;
					// Load group
					plibgroup.pfirst=NULL;
					loadscript(in,plibgroup.pfirst,err,true,pfirstgroup,firstgroup,
						plastgroup,plibword);
					if (err) return;
					plibgroup.pnext=NULL;
					pgroup=new Plibgroup;
					if (!pgroup) {
						err=PLIB_OUTOFMEMORY;
						return;
					}
					*pgroup=plibgroup;
					if (firstgroup) {
						pfirstgroup=pgroup;
						firstgroup=false;
						plastgroup=pfirstgroup;
					}
					else {
						plastgroup->pnext=pgroup;
						plastgroup=pgroup;
					}
					//pthisgroup=pgroup;
					groupheader=false;
					waitgroupname=false;
				}
				addword=false;
			}
			else if (importstatement) {
				importstatement = false;
				addword = false;
				if (plibword.type != WORD_STRING) {
					err = PLIB_EXPECTSTRING;
					return;
				}
				else if (!in.addfile(plibword.string)) {
					err = PLIB_FILEOPEN;
					return;
				}
			}
			else if (plibword.type==WORD_CLOSEPARENTHESIS) {
				if (parct) parct--;
				else {
					if (isgroup) return; // end of group block
					// else
					err=PLIB_PARENTHESISNESTING;
					return;
				}
			}
			else if (plibword.type==WORD_OPENPARENTHESIS)
				parct++;
			else if (plibword.type==WORD_IDENTIFIER) {
				if (plibword.string==(char*)"group") {
					groupheader=true;
					waitgroupname=true;
					addword=false;
				}
				else if (plibword.string == "import") {
					importstatement = true;
					addword = false;
				}
				else {
					// Check if this is a call to a group
					if (findgroup(pfirstgroup,plibword.string,plibgroup)) {
						plibword.pgroup=plibgroup.pfirst;
						plibword.type=WORD_GROUP;
					}
				}
			}
			else if (plibword.type==WORD_NOWORD) {
				addword=false;
			}
			if (addword) {
				plibword.pnext=NULL;
				pword=new Plibword;
				if (!pword) {
					err=PLIB_OUTOFMEMORY;
					return;
				}
				*pword=plibword;
				if (firstword) {
					pfirstword=pword;
					firstword=false;
				}
				else {
					pthisword->pnext=pword;
				}
				pthisword=pword;
			}
		}
	}
	err=PLIB_NOERROR;
	return;
}


void findidentifier(Pliblist& pliblist,xtring& identifier,Plibitem*& pitem) {

	// Looks for a plib item with specified identifier in pliblist,
	// returning pointer to item (or NULL if item is not found)

	pitem=pliblist.getfirstitem();
	while (pitem) {
		if (pitem->identifier==identifier) return;
		pitem=pliblist.getnextitem();
	}
	pitem=NULL;
}

bool itemparsed(xtring identifier) {

	Plibitem* pitem;
	xtring string=identifier.lower();
	findidentifier(*pthislist,string,pitem);
	if (!pitem) return false;
	return pitem->called;
}

void killpliblist(Pliblist* plist) {

	Plibitem* pitem;

	pitem=plist->getfirstitem();
	while (pitem) {
		plist->removeitem();
		pitem=plist->getfirstitem();
	}
}

Plibword* parse_plib_script(int id,xtring setname,errortype& err,Plibword* pfirstword,
	bool newset,Pliblist* plist) {

	Pliblist pliblist;
	Plibword* pword=pfirstword,*pgroupword;
	Plibitem* pitem;
	pthislist=&pliblist;
	if (newset) {
		pliblist.callback=0;
		plib_declarations(id,setname);
		plist=&pliblist;
		if (abortparse) {
			killpliblist(&pliblist);
			err=PLIB_ABORT;
			return pword;
		}
	}
	int i;

	while (pword) {

		lastword=pword->string;
		lastline=pword->lineno;
		lastfile=pword->filename;
		if (pword->type==WORD_GROUP) {
			pgroupword=parse_plib_script(id,"",err,pword->pgroup,false,plist);
			if (err) {
				killpliblist(&pliblist);
				return pgroupword;
			}
		}
		else if (pword->type==WORD_CLOSEPARENTHESIS) {
			if (!id) err=PLIB_PARENTHESISNESTING;
			else err=PLIB_NOERROR;
			pthislist=plist;
			if (plist->callback && newset) {
				plib_callback(plist->callback);
				if (abortparse) {
					err=PLIB_ABORT;
				}
			}
			killpliblist(&pliblist);
			return pword;
		}
		else if (pword->type!=WORD_IDENTIFIER) {
			err=PLIB_EXPECTIDENTIFIER;
			killpliblist(&pliblist);
			return pword;
		}
		else {
			findidentifier(*plist,pword->string,pitem);
			if (!pitem) {
				err=PLIB_UNDEFINEDITEM;
				killpliblist(&pliblist);
				return pword;
			}
			// Now pitem points to item to assign to
			switch (pitem->type) {
			case PLIB_XTRING:
			case PLIB_STDSTRING:
				pword=pword->pnext;
				lastword=pword->string;
				lastline=pword->lineno;
				if (!pword) {
					err=PLIB_ENDOFFILE;
					killpliblist(&pliblist);
					return pword;
				}
				if (pword->type!=WORD_STRING) {
					err=PLIB_EXPECTSTRING;
					killpliblist(&pliblist);
					return pword;
				}
				if (static_cast<int>(pword->string.len())>pitem->maxlen-1)
					pword->string=pword->string.left(pitem->maxlen-1);

				if (pitem->type == PLIB_XTRING) {
					*((xtring*)(pitem->param))=pword->string;
				}
				else {
					*((std::string*)(pitem->param))=pword->string;
				}
				pitem->called=true;
				break;
			case PLIB_INT:
				for (i=0;i<pitem->nparam;i++) {
					pword=pword->pnext;
					lastword=pword->string;
					lastline=pword->lineno;
					if (!pword) {
						err=PLIB_ENDOFFILE;
						killpliblist(&pliblist);
						return pword;
					}
					if (pword->type!=WORD_NUMBER) {
						err=PLIB_EXPECTNUMBER;
						killpliblist(&pliblist);
						return pword;
					}
					if (pword->num<pitem->min || pword->num>pitem->max) {
						err=PLIB_OUTOFRANGE;
						killpliblist(&pliblist);
						return pword;
					}
					*((int*)(pitem->param)+i)=(int)(pword->num+0.5);
				}
				pitem->called=true;
				break;
			case PLIB_DOUBLE:
				for (i=0;i<pitem->nparam;i++) {
					pword=pword->pnext;
					lastword=pword->string;
					lastline=pword->lineno;
					if (!pword) {
						err=PLIB_ENDOFFILE;
						killpliblist(&pliblist);
						return pword;
					}
					if (pword->type!=WORD_NUMBER) {
						err=PLIB_EXPECTNUMBER;
						killpliblist(&pliblist);
						return pword;
					}
					if (pword->num<pitem->min || pword->num>pitem->max) {
						err=PLIB_OUTOFRANGE;
						killpliblist(&pliblist);
						return pword;
					}
					*((double*)(pitem->param)+i)=pword->num;
				}
				pitem->called=true;
				break;
			case PLIB_BOOLEAN:
				for (i=0;i<pitem->nparam;i++) {
					pword=pword->pnext;
					lastword=pword->string;
					lastline=pword->lineno;
					if (!pword) {
						err=PLIB_ENDOFFILE;
						killpliblist(&pliblist);
						return pword;
					}
					if (pword->type!=WORD_NUMBER) {
						err=PLIB_EXPECTNUMBER;
						killpliblist(&pliblist);
						return pword;
					}
					*((bool*)(pitem->param)+i)=(bool)(!!(pword->num));
				}
				pitem->called=true;
				break;
			case PLIB_FLAG:
				*(bool*)(pitem->param)=true;
				pitem->called=true;
				break;
			case PLIB_SET:
				pword=pword->pnext;
				lastword=pword->string;
				lastline=pword->lineno;
				if (!pword) {
					err=PLIB_ENDOFFILE;
					killpliblist(&pliblist);
					return pword;
				}
				if (pword->type!=WORD_STRING) {
					err=PLIB_EXPECTSTRING;
					killpliblist(&pliblist);
					return pword;
				}
				setname=pword->string;
				pword=pword->pnext;
				lastword=pword->string;
				lastline=pword->lineno;
				if (!pword) {
					err=PLIB_ENDOFFILE;
					killpliblist(&pliblist);
					return pword;
				}
				if (pword->type!=WORD_OPENPARENTHESIS) {
					err=PLIB_EXPECTOPENPARENTHESIS;
					killpliblist(&pliblist);
					return pword;
				}
				pword=pword->pnext;
				lastword=pword->string;
				lastline=pword->lineno;
				pword=parse_plib_script(pitem->id,setname,err,pword,true,NULL);
				pitem->called=true;
				if (err) {
					killpliblist(&pliblist);
					return pword;
				}
				break;
			}
			if (pitem->callback) {
				plib_callback(pitem->callback);
				if (abortparse) {
					err=PLIB_ABORT;
					killpliblist(&pliblist);
					return pword;
				}
			}
		}
		pword=pword->pnext;
	}
	if (id && newset) err=PLIB_ENDOFFILE; // should be a close parenthesis
	else {
		pthislist=plist;
		if (plist->callback && newset) plib_callback(plist->callback);
		if (abortparse) {
			err=PLIB_ABORT;
			killpliblist(&pliblist);
			return pword;
		}
		err=PLIB_NOERROR;
	}
	killpliblist(&pliblist);
	return NULL;
}

void getmessageheader(xtring& text) {

	text.printf("In %s, line %d, \"%s\":\n",(char*)lastfile,
		(char*)lastword,lastline);
}

void sendmessage(xtring heading,xtring message) {

	xtring text;
	text.printf("In %s, line %d, \"%s\":\n  ",(char*)lastfile,lastline,
		(char*)lastword);
	text+=heading+": "+message+"\n";
	plib_receivemessage(text);
}

void geterrormessage(errortype err) {

	switch (err) {
	case PLIB_INVALIDNUMBER:
		sendmessage("Error","Cannot be interpreted as a number");
		break;
	case PLIB_OUTOFMEMORY:
		sendmessage("Error","Out of memory");
		break;
	case PLIB_ENDOFFILE:
		sendmessage("Error","Unexpected end of file");
		break;
	case PLIB_PARENTHESISNESTING:
		sendmessage("Error","Parenthesis nesting error");
		break;
	case PLIB_GROUPHEADER:
		sendmessage("Error","Invalid format for group header");
		break;
	case PLIB_DUPLICATEGROUPNAME:
		sendmessage("Error","Duplicate group name");
		break;
	case PLIB_FILEOPEN:
		sendmessage("Error","Could not open file for input");
		break;
	case PLIB_EXPECTIDENTIFIER:
		sendmessage("Error","Identifier expected");
		break;
	case PLIB_UNDEFINEDITEM:
		sendmessage("Error","Undefined identifier");
		break;
	case PLIB_EXPECTSTRING:
		sendmessage("Error","String expected");
		break;
	case PLIB_EXPECTNUMBER:
		sendmessage("Error","Number expected");
		break;
	case PLIB_OUTOFRANGE:
		sendmessage("Error","Value out of range");
		break;
	case PLIB_EXPECTOPENPARENTHESIS:
		sendmessage("Error","Parenthesis expected");
		break;
	case PLIB_ABORT:
		sendmessage("Error","PLIB forcibly terminated by calling module");
		break;
	case PLIB_UNRECOGNISEDTOKEN:
		sendmessage("Error","Unrecognised token");
	case PLIB_NOERROR:
		// No nothing (included to avoid warnings from pedantic compilers)
		break;
	}
}

void plibabort() {

	abortparse=true;
};

void killplib(Plibword* pfirstword,Plibgroup* pfirstgroup) {

	Plibword* pword,*pthisword;
	Plibgroup* pgroup,*pthisgroup;

	pword=pfirstword;
	while (pword) {
		pthisword=pword;
		pword=pword->pnext;
		delete pthisword;
	}

	pgroup=pfirstgroup;
	while (pgroup) {
		pword=pgroup->pfirst;
		while (pword) {
			pthisword=pword;
			pword=pword->pnext;
			delete pthisword;
		}
		pthisgroup=pgroup;
		pgroup=pgroup->pnext;
		delete pthisgroup;
	}
}

bool plib(xtring filename) {

	Plibword* pfirstword=NULL;
	Plibgroup* pfirstgroup=NULL;
	Plibgroup* plastgroup=NULL;
	Plibword plibword,*pword;
	errortype err;
	bool firstgroup=true;

	abortparse=false;

	lastfile=filename;
	lastline=0;

	RecursiveFileReader in;
	if (!in.addfile(filename)) {
		geterrormessage(PLIB_FILEOPEN);
		return false;
	}

	currch='\0';
	pfirstgroup=NULL;

	loadscript(in,pfirstword,err,false,pfirstgroup,firstgroup,plastgroup,plibword);

	if (err) {
		geterrormessage(err);
	}
	else {
		pword=parse_plib_script(0,"",err,pfirstword,true,NULL);
		if (err) {
			geterrormessage(err);
		}
	}
	killplib(pfirstword,pfirstgroup);

	if (err) return false;
	return true;
}

void plibhelp() {

	ishelp=true;

	helpheader((xtring)"global");
	plib_declarations(0,"");

	ishelp=false;
}
