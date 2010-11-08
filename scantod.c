#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <syslog.h>
#include <getopt.h>
#include <errno.h>

#include "scanto.h"

int dofork = 1;
int ruid = -1;
char *pidfile = "/var/run/scantod.pid";
char *inifile = "/etc/scantod.ini";

struct destinations {
	char *dst;
	char *cmd;
	char *adf;
};

void mylog(char *str, ...) {
        va_list arg;
        va_start(arg,str);
        if(dofork) {
                vsyslog(LOG_NOTICE, str, arg);
        } else {
                vfprintf(stderr, str, arg);
        }
        va_end(arg);
}

void usage() {
        fprintf(stderr, "Usage: scantod [-f] [-u user] [-c inifile] scanner\n");
}

int scanloop(char *prt, struct destinations *dests, int destcnt) {
	char *cmd;
	int i, ret;
	int online = 1;
	struct scantonotes *sn;

	sn = malloc(sizeof(struct scantonotes));

	while (1) {
		if(getScanToNotes(sn, prt) == 0) {
			if (online == 0) {
				mylog("Scanner %s became online\n", prt);
				online = 1;
			}
			if(sn->notsetup == 1) {
				for(i = 0; i < destcnt; i++) {
					addScanToDest(prt, dests[i].dst);
				}
			} else if (strlen(sn->display) > 0) {
				for(i = 0; i < destcnt; i++) {
					if (strcmp(sn->display, dests[i].dst) == 0) {
						if (sn->adf == 1) {
							cmd = dests[i].adf;
						} else {
							cmd = dests[i].cmd;
						}
						mylog("Starting Scan (command: %s)\n", cmd);

						ret = system(cmd);

						if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
							return(1);
						}
					}
				}
			}
		} else {
			if (online == 1) {
				mylog("Scanner %s seems to be offline (still polling)\n", prt);
				online = 0;
			}
		}
			
		sleep(3);
	}

	free(sn);

	return(0);
}

static void terminate() {
	seteuid(ruid);
	if (unlink(pidfile)) {
		mylog("Can't remove pidfile %s: %s", pidfile, strerror(errno)); 
	}
	exit(0);
}

int main(int argc, char *argv[]) {
	struct destinations *dests;

	GKeyFile *ini = g_key_file_new();
	char *prt;
	char **inigrp;
	char *user = NULL;
	char *pidfile = "/var/run/scantod.pid";
	struct passwd *pwd;
	gsize inigrpcnt;
	int i, c;
        pid_t pid;
        int fd = -1;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"config", 1, 0, 'c'},
			{"user", 1, 0, 'u'},
                        {"foreground", 0, 0, 'f'},
                        {"pidfile", 0, 0, 'p'},
                        {0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "c:u:p:f",
			long_options, &option_index);
                if (c == -1)
                        break;

                switch (c) {
                        case 'c':
				inifile = optarg;
				break;
                        case 'f':
                                dofork = 0;
                                break;
                        case 'u':
                                user = optarg;
                                break;
                        case 'p':
                                pidfile = optarg;
                                break;
                        default:
                                usage();
                                return(1);
		}
	}

	if (optind+1 != argc) {
                usage();
                return(1);
        }

	prt = argv[optind];

	if(g_key_file_load_from_file(ini, inifile, G_KEY_FILE_NONE, NULL) == FALSE) {
		fprintf(stderr, "inifile %s could not be loaded\n", inifile);
		return(1);
	}
	inigrp = g_key_file_get_groups(ini, &inigrpcnt);

	dests = malloc(inigrpcnt * sizeof(struct destinations));

	for(i = 0; i < inigrpcnt; i++) {
		dests[i].dst = strdup(g_key_file_get_value(ini, inigrp[i], "dest", NULL));
		if (strchr(dests[i].dst, ':') == NULL) {
			fprintf(stderr, "Destination in group %s doesn't contain a collon\n", inigrp[i]);
			return(1);
		}
		dests[i].cmd = strdup(g_key_file_get_value(ini, inigrp[i], "cmd", NULL));
		if (strlen(dests[i].cmd) <= 0) {
			fprintf(stderr, "No command in group %s found\n", inigrp[i]);
			return(1);
		}
		dests[i].adf = strdup(g_key_file_get_value(ini, inigrp[i], "adf", NULL));
		if (strlen(dests[i].adf) <= 0) {
			fprintf(stderr, "No ADF-command in group %s found\n", inigrp[i]);
			return(1);
		}
	}


	if (dofork) {
                setlogmask(LOG_UPTO (LOG_NOTICE));

                openlog("scantod", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

                pid = fork();
                if (pid == -1) {
                        fprintf(stderr, "fork() failed: %s", strerror(errno));
                        return(1);
                } else if (pid != 0) {
                        // parent
                        return(0);
                }

                /* Clean up */
                setsid();
                chdir("/");
                if((fd = open("/dev/null", O_RDWR)) < 0) {
                        mylog("open(\"/dev/null\") failed: %s", strerror(errno));
                        return(0);
                }

                for (i = 0; i < 3; ++i) {
                        if (dup2(fd, i) < 0) {
                                mylog("dup2() failed for %d: %s", i, strerror(errno));
                        }
                }
        }

	if (pidfile != NULL) {
		pid_t curpid;
		FILE *f;
		
		curpid = getpid();

		f = fopen(pidfile, "w");
		if(f == NULL) {
			mylog("Can't open pidfile %s", pidfile);
			return(1);
		}
		fprintf(f, "%i\n", curpid);
		fclose(f);
	}
		
	if (user != NULL) {
		ruid = getuid();
		pwd = getpwnam(user);
		if (pwd == NULL) {
			mylog("User %s not found!", user);
			return(1);
		}
		if (seteuid(pwd->pw_uid) < 0) {
			mylog("setuid failed");
			return(1);
		}
	}

        /* init signal processing */
        signal(SIGTERM, terminate);

	g_key_file_free(ini);

	delAllScanToDest(prt);

	scanloop(prt, dests, inigrpcnt);

	return(0);
}


