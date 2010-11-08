#ifndef __SCANTO_H__
#define __SCANTO_H__
struct scantonotes {
	char display[256];
	char host[256];
	int notsetup;
	int adf;
};

int getScanToNotes(struct scantonotes *, char *);
int getScanToDest(int, char *); 
int delAllScanToDest(char *);
int addScanToDest(char *, char *);
int delScanToDest(char *, char *);

#endif /* __SCANTOD_H__ */
