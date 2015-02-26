//Ursache Andrei - 312CA

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<math.h>

/* DEFINE-uri */

#define L 512
#define l 2
#define d 10

/* STRUCTURI DE DATE SI UNIUNI */

typedef struct comanda{ // comanda primita
	char *nume;
	char *arg[l];
}COMANDA;

typedef union record { //bloc
	char charptr[L];
	struct header {
		char name[100];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char typeflag;
		char linkname[100];
		char magic[8];
		char uname[32];
		char gname[32];
		char devmajor[8];
		char devminor[8];
	} header;
}RECORD;

typedef struct date{ // data - pentru mtime
	int year, mon, day, hour, min;
	double sec; 
}DATE;

/* ANTET FUNCTII */

void citeste_comanda(COMANDA *com); // citeste comanda

int numar_fisiere(FILE *file); // numara liniile (din file_ls) 

void null(char v[L]); // initializeaza vector cu \0

void load(char *arch_name, FILE *user, FILE *file); // load

int octal_decimal(int octal); // conversie octal - decimal

void list(char *arch_name); // list

void get(char *arch_name, char *file_name); // get

/* PROGRAM */

int main(){
	FILE *user, *file; // fisierele
	COMANDA com; // comanda
	int test=0;

	//Verificare existenta fisiere
	if( (user=fopen("usermap.txt","r")) == NULL ){
		fprintf(stderr,"%s\n","usermap.txt doesn't exist");
		test=1;
	}
	if( (file=fopen("file_ls","r")) == NULL ){
		fprintf(stderr,"%s\n","file_ls doesn't exist");
		return -1;
	}
	if(test) return -1;

	while(1){
		citeste_comanda(&com); // citeste comanda
		test=0;
		if(strcmp(com.nume, "load") == 0){
			load(com.arg[0],user,file);
			test=1;
		}
		if(strcmp(com.nume, "list") == 0){
			list(com.arg[0]);
			test=1;
		}
		if(strcmp(com.nume, "get") == 0){
			get(com.arg[0], com.arg[1]);
			test=1;
		}
		if(strcmp(com.nume, "quit") == 0){
			break;
		}
		if(!test)fprintf(stderr,"%s is not a command\n",com.nume);
	}
	
	fclose(user);
	fclose(file);
	return 0;
}

/* FUNCTII */

void citeste_comanda(COMANDA *com){ // citeste comanda
	char date[L], *sep;
	int i=0;
	fgets(date,L,stdin);
	sep=strtok(date, " \n");
	while(sep != NULL){
		if(i == 0){
			com->nume=strdup(sep); // numele comenzii
		}
		else{
			com->arg[i-1]=strdup(sep); // argumente	
		}
		i++;
		sep=strtok(NULL, " \n");
	}
}

int numar_fisiere(FILE *file){ // numara liniile (din file_ls) 
	int ch, nr=0;
	do {
		ch = fgetc(file);
		if(ch == '\n')
			nr++;
	} while (ch != EOF);

	return nr;
}

void null(char v[L]){ // initializeaza cu \0
	int i;
	for(i=0 ; i<L ; i++)
		v[i]=0;
	return;
}

void load(char *arch_name, FILE *user, FILE *file){
	FILE *archive, *f; // arhiva
	RECORD current; // fisier curent
	DATE date; // data ultimei modoificari
	
	int u=0,g=0,o=0,total; // drepturile utilizatorilor
	int n=numar_fisiere(file); // numaul de fisiere
	int gid, uid; // GID si UID
	
	char temp[L], *sep, mode[d]; // auxiliar
	int size, m, i, j, len; // auxiliare
	
	struct tm info; // pentru mtime
	int sec;

	archive=fopen(arch_name, "w");

	fseek(file, 0, SEEK_SET);

	// pentru fiecare fisier de arhivat
	for(i=0 ; i<n ; i++){
		null(current.charptr);
		// parsare file_ls
		fseek(file, 1, SEEK_CUR);
		fscanf(file, "%s\t", mode); // drepturile utilizatorilor
		fseek(file, 2, SEEK_CUR);
		fscanf(file, "%s\t",current.header.uname); // uname
		fscanf(file, "%s\t%d\t",current.header.gname, &size); // gname, size
		fscanf(file, "%d-%d-%d\t",&date.year,&date.mon,&date.day); // date
		fscanf(file, "%d:%d:%lf\t",&date.hour,&date.min,&date.sec); // date
		fseek(file, 6, SEEK_CUR);
		fscanf(file, "%s",current.header.name); // name

		strcpy(current.header.linkname,current.header.name); // linkname
		sprintf(current.header.size,"%011o",size); // size
		current.header.typeflag='0'; // typeflag
		strcpy(current.header.magic, "GNUtar "); // magic
		
		// aflare nr. secunde de la 1 - 1 - 1970 ( cu mktime() )
		info.tm_year = date.year - 1900;
		info.tm_mon = date.mon - 1;
		info.tm_mday = date.day;
		info.tm_hour = date.hour;
		info.tm_min = date.min;
		info.tm_sec = (int)date.sec;
		info.tm_isdst = -1;
		sec=(int)mktime(&info); // numarul de sec 
		sprintf(current.header.mtime,"%011o",sec); // mtime
		
		// cautare user in usermap.txt
		while(fgets(temp, L, user) != NULL) { 
			if((strstr(temp, current.header.uname)) != NULL) {
				break;
			}
		}
		fseek(user,0,SEEK_SET);
		j=0; 
		sep=strtok(temp, ":");
		while(sep != NULL){
			if(j == 2){
				strcpy(current.header.uid, sep); // UID
			}
			if(j == 3){
				strcpy(current.header.gid, sep); // GID
				break;
			}
			j++;
			sep=strtok(NULL, ":");
		}
		uid=atoi(current.header.uid);
		gid=atoi(current.header.gid);
		sprintf(current.header.uid,"%07o",uid); // uid 
		sprintf(current.header.gid,"%07o",gid); // gid 

		// drepturi
		u=0;g=0;o=0;
		if(mode[0]=='r')u+=4;
		if(mode[1]=='w')u+=2;
		if(mode[2]=='x')u+=1;
		if(mode[3]=='r')g+=4;
		if(mode[4]=='w')g+=2;
		if(mode[5]=='x')g+=1;
		if(mode[6]=='r')o+=4;
		if(mode[7]=='w')o+=2;
		if(mode[8]=='x')o+=1;
		total=u*100+g*10+o;
		sprintf(current.header.mode,"%07d",total); // mode

		// calculare chksum
		for(j=0;j<8;j++){
			current.header.chksum[j]=' ';
		}
		len=0;
		for(j=0;j<L;j++){
			len+=current.charptr[j];
		}
		sprintf(current.header.chksum,"%06o",len); // checksum

		// scriere in arhiva
		f=fopen(current.header.name,"r");
		// scrie header
		fwrite(current.charptr,sizeof(char),L,archive);
		null(current.charptr);
		m=size;
		do {
			null(current.charptr);
			// citeste bloc
			m=fread(current.charptr,sizeof(char),L,f);
			//scrie bloc
			fwrite(current.charptr,sizeof(char),L,archive);
		}while(m==L);
		fclose(f);

		// urmatorul fisier de arhivat
		fseek(file, 1, SEEK_CUR); // urmatoarea linie din file_ls
	}
	fclose(archive);
}

int octal_decimal(int octal){ // conversie octal - decimal
	int decimal=0, i=0, rem;
	while (octal != 0){
		rem=octal%10;
		octal/=10;
		decimal+=rem*pow(8,i);
		++i;
	}
	return decimal;
}


void list(char *arch_name){
	FILE *archive;
	archive=fopen(arch_name, "r");
	RECORD current;
	int cat, m;
	do {
		// citeste header
		m=fread(current.charptr,sizeof(char), L, archive);
		if(m == 0)break;
		// afiseaza nume fisier
		printf("%s\n", current.header.name);
		cat=atoi(current.header.size);
		cat=octal_decimal(cat);
		cat=(int)(cat/L);cat++;
		// sari la urmatorul header
		fseek(archive, L*cat, SEEK_CUR);
	}while(SEEK_CUR != SEEK_END);
	
	fclose(archive);

}

void get(char *arch_name, char *file_name){
	FILE *archive;
	archive=fopen(arch_name, "r");
	RECORD current;
	int cat, m, i=0;
	do {
		// citeste header
		m=fread(current.charptr,sizeof(char), L, archive);
		if(m == 0)break;
		cat=atoi(current.header.size);
		cat=octal_decimal(cat);
		cat=(int)(cat/L);cat++;
		// daca numele fisierului este cel potrivit
		if(strcmp(current.header.name, file_name) == 0){
			// afiseaza blocurile de continut
			for(i=0 ; i<cat ; i++){
				fread(current.charptr,sizeof(char), L-1, archive);
				printf("%s", current.charptr);
			}
			// iesi din bucla
			break;
		}
		// daca numele fsierului nu corespunde
		// sari la urmatorul header
		else	fseek(archive, L*cat, SEEK_CUR);
	}while(SEEK_CUR != SEEK_END);

	fclose(archive);
}
