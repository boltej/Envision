//////////////////////////////////////////////////////////////////////////////////////
// GLOBALNITROGENDEPOSITIONRCP85CC.H
// Header file for input from a fast data archive
// Created automatically by FastArchive on Tue Nov 03 17:49:58 2015
//
// The following #includes should appear in your source code file:
//
//   #include <stdio.h>
//   #include <stdlib.h>
//   #include <string.h>
//   #include "E:\Libs\Large_files_(no_bu)\ndep_cc\binfiles\GlobalNitrogenDepositionRCP85.h"
//
// Functionality to retrieve data from the archive is provided by class GlobalNitrogenDepositionRCP85Archive.
// The following public functions are provided:
//
// bool open(char* filename)
//   Attempts to open the specified file as a fast data archive. The format must be
//   exactly compatible with this version of GlobalNitrogenDepositionRCP85.h (normally the archive and
//   header file should have been produced together by the same program using class
//   CFastArchive). Returns false if the file could not be opened or had format
//   errors. open() with no argument is equivalent to open("GlobalNitrogenDepositionRCP85.bin").
//
// void close()
//   Closes the archive (if open).
//
// bool rewind()
//   Sets the file pointer to the first record in the archive file. Returns false if
//   no archive file is currently open.
//
// bool getnext(GlobalNitrogenDepositionRCP85& obj)
//   Retrieves the next record in the archive file and advances the file pointer to
//   the next record. Data are written to the member variables of obj. Returns false if
//   no archive file is currently open or if the file pointer is beyond the last
//   record. Use rewind() and getnext() to retrieve data sequentially from the archive.
//
// bool getindex(GlobalNitrogenDepositionRCP85& obj)
//   Searches the archive for a record matching the values specified for the index
//   items (longitude and latitude) in obj. If a matching record is found, the data are
//   written to the member variables of obj. Returns true if the archive was open and
//   a matching record was found, otherwise false. The search is iterative and fast.
//
// Sample program:
//
//   GlobalNitrogenDepositionRCP85Archive ark;
//   GlobalNitrogenDepositionRCP85 data;
//   bool success,flag;
//
//   // Retrieve all records in sequence and print values of longitude and latitude:
//
//   success=ark.open("GlobalNitrogenDepositionRCP85.bin");
//   if (success) {
//      flag=ark.rewind();
//      while (flag) {
//         flag=ark.getnext(data);
//         if (flag)
//            printf("Loaded record: longitude=%g, latitude=%g\n",data.longitude,data.latitude);
//      }
//   }
//   
//   // Look for a record with longitude=-180, latitude=-90:
//
//   data.longitude=-180;
//   data.latitude=-90;
//   success=ark.getindex(data);
//   if (success) printf("Found it!\n");
//   else printf("Not found\n");
//
//   ark.close();


struct GlobalNitrogenDepositionRCP85 {

	// Index part

	double longitude;
	double latitude;

	// Data part

	double NHxDry[132];
	double NHxWet[132];
	double NOyDry[132];
	double NOyWet[132];
};


const long GLOBALNITROGENDEPOSITIONRCP85_NRECORD=59191;
const int GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH=1617;
const int GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH=7;
const int GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE=670;
unsigned char GLOBALNITROGENDEPOSITIONRCP85_HEADER[GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE-4]={
	0x01,0x02,0x9E,0x00,0x00,0x00,0x07,0x00,0x02,0x0A,0x6C,0x6F,0x6E,0x67,0x69,0x74,0x75,0x64,0x65,0x00,
	0x2D,0x31,0x38,0x30,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x31,0x38,0x30,0x00,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0x30,0x2E,0x32,0x35,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x0B,0x09,0x6C,0x61,
	0x74,0x69,0x74,0x75,0x64,0x65,0x00,0x2D,0x39,0x30,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x39,
	0x30,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x30,0x2E,0x32,0x35,0x00,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0x0A,0x00,0x00,0x06,0x51,0x00,0x04,0x07,0x4E,0x48,0x78,0x44,0x72,0x79,0x00,0x30,0x00,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x32,0x35,0x38,0x2E,0x31,0x37,0x32,0x00,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0x31,0x65,0x2D,0x30,0x30,0x35,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x19,0x00,0x00,0x00,0x84,0x07,
	0x4E,0x48,0x78,0x57,0x65,0x74,0x00,0x30,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x38,
	0x38,0x34,0x2E,0x31,0x34,0x33,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x31,0x65,0x2D,0x30,0x30,0x35,0x00,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0x1B,0x00,0x00,0x00,0x84,0x07,0x4E,0x4F,0x79,0x44,0x72,0x79,0x00,0x30,0x00,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x35,0x33,0x2E,0x31,0x34,0x39,0x36,0x00,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0x31,0x65,0x2D,0x30,0x30,0x35,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x17,0x00,0x00,0x00,0x84,0x07,0x4E,0x4F,
	0x79,0x57,0x65,0x74,0x00,0x30,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x35,0x30,0x2E,
	0x34,0x30,0x35,0x34,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x31,0x65,0x2D,0x30,0x30,0x35,0x00,0xCC,0xCC,0xCC,0xCC,
	0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	0xCC,0x17,0x00,0x00,0x00,0x84};


class GlobalNitrogenDepositionRCP85Archive {

private:

	FILE* pfile;
	long recno;
	long datano;
	unsigned char pindex[GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH];
	unsigned char pdata[GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH];
	bool iseof;

	long readbin(int nbyte) {

		unsigned char buf[4];
		long mult[4]={0x1000000,0x10000,0x100,1},val;
		int i;

		fread(buf,nbyte,1,pfile);

		val=0;
		for (i=0;i<nbyte;i++) {
			val+=buf[i]*mult[4-nbyte+i];
		}

		return val;
	}

	void getindex(long n) {

		fseek(pfile,GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH*(n-recno),SEEK_CUR);
		fread(pindex,GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH,1,pfile);
		datano=pindex[GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH-4]*0x1000000+pindex[GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH-3]*0x10000+
			pindex[GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH-2]*0x100+pindex[GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH-1];
		recno=n+1;
		iseof=(recno==GLOBALNITROGENDEPOSITIONRCP85_NRECORD);
	}

	void getdata() {

		fseek(pfile,GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH*-recno+(datano-GLOBALNITROGENDEPOSITIONRCP85_NRECORD)*GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,SEEK_CUR);
		fread(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,1,pfile);
		fseek(pfile,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH*(GLOBALNITROGENDEPOSITIONRCP85_NRECORD-datano-1)+GLOBALNITROGENDEPOSITIONRCP85_INDEX_LENGTH*recno,SEEK_CUR);
	}

	double popreal(unsigned char* bits,int nbyte,int nbit,double scalar,double offset) {

		unsigned char buf;
		int nb=nbit/8,i;
		double rval=0.0;
		long mult[4]={1,0x100,0x10000,0x1000000};

		for (i=0;i<4;i++) {
			if (i<nb) rval+=bits[nbyte-i-1]*mult[i];
			else if (i==nb) {
				buf=bits[nbyte-i-1]<<(8-nbit%8);
				buf>>=8-nbit%8;
				rval+=buf*mult[i];
			}
		}

		for (i=nbyte-1;i>=0;i--) {
			if (i>=nb)
				bits[i]=bits[i-nb];
			else
				bits[i]=0;
		}

		nb=nbit%8;

		for (i=nbyte-1;i>=0;i--) {
			bits[i]>>=nb;
			if (i>0) {
				buf=bits[i-1];
				buf<<=8-nb;
				bits[i]|=buf;
			}
		}

		rval=rval*scalar+offset;

		return rval;
	}

	void bitify(unsigned char buf[4],double fval,double offset,double scalar) {

		long ival = (long)((fval-offset)/scalar + 0.5);
		buf[0]=(unsigned char)(ival/0x1000000);
		ival-=buf[0]*0x1000000;
		buf[1]=(unsigned char)(ival/0x10000);
		ival-=buf[1]*0x10000;
		buf[2]=(unsigned char)(ival/0x100);
		ival-=buf[2]*0x100;
		buf[3]=(unsigned char)(ival);
	}

	void merge(unsigned char ptarget[3],unsigned char buf[4],int bits) {

		int nb=bits/8;
		int i;
		unsigned char nib;
		for (i=0;i<3;i++) {

			if (i<3-nb)
				ptarget[i]=ptarget[i+nb];
			else
				ptarget[i]=0;
		}
		nb=bits%8;
		for (i=0;i<3;i++) {
			ptarget[i]<<=nb;
			if (i<3-1) {
				nib=ptarget[i+1]>>(8-nb);
				ptarget[i]|=nib;
			}
		}

		nb=bits/8;
		if (bits%8) nb++;
		for (i=1;i<=nb;i++)
			ptarget[3-i]|=buf[4-i];
	}

	int compare_index(unsigned char* a,unsigned char* b) {

		int i;
		for (i=0;i<3;i++) {
			if (a[i]<b[i]) return -1;
			else if (a[i]>b[i]) return +1;
		}

		return 0;
	}

	bool initialise(const char* filename) {

		int i;
		unsigned char* pheader;

		if (pfile) fclose(pfile);
		pfile=fopen(filename,"rb");
		if (!pfile) {
			printf("Could not open %s for input\n",filename);
			return false;
		}

		pheader=new unsigned char[GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE-4];
		if (!pheader) {
			printf("Out of memory\n");
			fclose(pfile);
			pfile=NULL;
			return false;
		}
		::rewind(pfile);
		fread(pheader,GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE-4,1,pfile);
		for (i=0;i<GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE-4;i++) {
			if (pheader[i]!=GLOBALNITROGENDEPOSITIONRCP85_HEADER[i]) {
				printf("Format of %s incompatible with this version of GlobalNitrogenDepositionRCP85.h\n",filename);
				fclose(pfile);
				pfile=NULL;
				delete pheader;
				return false;
			}
		}
		delete[] pheader;

		::rewind(pfile);
		fseek(pfile,GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE+GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH*GLOBALNITROGENDEPOSITIONRCP85_NRECORD,SEEK_CUR);
		recno=0;
		iseof=false;

		return true;
	}

public:

	GlobalNitrogenDepositionRCP85Archive() {
		pfile=NULL;
	}

	~GlobalNitrogenDepositionRCP85Archive() {
		if (pfile) fclose(pfile);
	}

	bool open(const char* filename) {
		return initialise(filename);
	}

	bool open() {
		return open("GlobalNitrogenDepositionRCP85.bin");
	}

	void close() {
		if (pfile) {
			fclose(pfile);
			pfile=NULL;
		}
	}

	bool rewind() {

		if (!pfile) return false;

		::rewind(pfile);
		fseek(pfile,GLOBALNITROGENDEPOSITIONRCP85_HEADERSIZE+GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH*GLOBALNITROGENDEPOSITIONRCP85_NRECORD,SEEK_CUR);
		recno=0;
		iseof=false;

		return true;
	}

	bool getnext(GlobalNitrogenDepositionRCP85& obj) {

		if (!pfile || iseof) return false;

		int i;

		getindex(recno);
		getdata();

		obj.latitude=popreal(pindex,3,10,0.25,-90);
		obj.longitude=popreal(pindex,3,11,0.25,-180);

		for (i=131;i>=0;i--) obj.NOyWet[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,23,1e-005,0);
		for (i=131;i>=0;i--) obj.NOyDry[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,23,1e-005,0);
		for (i=131;i>=0;i--) obj.NHxWet[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,27,1e-005,0);
		for (i=131;i>=0;i--) obj.NHxDry[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,25,1e-005,0);

		return true;
	}

	bool getindex(GlobalNitrogenDepositionRCP85& obj) {

		if (!GLOBALNITROGENDEPOSITIONRCP85_NRECORD || !pfile) return false;

		// else

		unsigned char ptarget[3]={0,0,0};
		unsigned char buf[4];
		bitify(buf,obj.longitude,-180,0.25);
		merge(ptarget,buf,11);
		bitify(buf,obj.latitude,-90,0.25);
		merge(ptarget,buf,10);

		long begin = 0;
		long end = GLOBALNITROGENDEPOSITIONRCP85_NRECORD;

		while (begin < end) {
			long middle = (begin+end)/2;

			getindex(middle);
			getdata();

			int c = compare_index(pindex, ptarget);

			if (c < 0) {
				begin = middle + 1;
			}
			else if (c > 0) {
				end = middle;
			}
			else {

				for (int i=131;i>=0;i--) obj.NOyWet[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,23,1e-005,0);
				for (int i=131;i>=0;i--) obj.NOyDry[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,23,1e-005,0);
				for (int i=131;i>=0;i--) obj.NHxWet[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,27,1e-005,0);
				for (int i=131;i>=0;i--) obj.NHxDry[i]=popreal(pdata,GLOBALNITROGENDEPOSITIONRCP85_DATA_LENGTH,25,1e-005,0);

				return true;
			}
		}

		return false;
	}
};
