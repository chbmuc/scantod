#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/nanohttp.h>
#include <glib/gstrfuncs.h>
#include <unistd.h>
#include <string.h>

#include "scanto.h"

int getScanToNotes(struct scantonotes *sn, char *phost) {
	xmlDocPtr doc;
	xmlNodePtr cur1, cur2;
	char *key;

	char url[1024];
	snprintf(url, 1024, "http://%s/hp/device/notifications.xml", phost);

	if ((doc = xmlParseFile(url)) == NULL) {
		return(-1);
	}

	cur1 = xmlDocGetRootElement(doc);

	if (xmlStrcmp(cur1->name, (const xmlChar *) "Notifications")) {
		fprintf(stderr,"document of the wrong type, root node != Notifications");
		xmlFreeDoc(doc);
		return -1;
	}

	cur1 = cur1->xmlChildrenNode;
	while (cur1 != NULL) {
		if ((!xmlStrcmp(cur1->name, (const xmlChar *)"ScanToNotifications"))) {
			cur2 = cur1->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!xmlStrcmp(cur2->name, (const xmlChar *)"ScanToDeviceDisplay"))) {
					key = (char *) xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					if (key != NULL) {
						g_strlcpy(sn->display, key, 255);
					} else {
						sn->display[0] = '\0';
					}
					xmlFree(key);
				} else if ((!xmlStrcmp(cur2->name, (const xmlChar *)"ScanToHostID"))) {
					key = (char *) xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					if (key != NULL) {
						g_strlcpy(sn->host, key, 255);
					} else {
						sn->host[0] = '\0';
					}
					xmlFree(key);
				} else if ((!xmlStrcmp(cur2->name, (const xmlChar *)"ScanToNotSetup"))) {
					key = (char *) xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					sn->notsetup = atoi(key);
					xmlFree(key);
				} else if ((!xmlStrcmp(cur2->name, (const xmlChar *)"ADFLoaded"))) {
					key = (char *) xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					sn->adf = atoi(key);
					xmlFree(key);
				}
				cur2 = cur2->next;
			}
		}
		cur1 = cur1->next;
	}

	xmlFreeDoc(doc);
	return(0);
}

int getScanToDest(int mode, char *phost) {
	xmlDocPtr doc;
	xmlNodePtr cur1, cur2, cur3;
	xmlChar *key;
	int destcnt = 0;

	char url[1024];
	snprintf(url, 1024, "http://%s/hp/device/info_scanto_destinations.xml", phost);
	if ((doc = xmlParseFile(url)) == NULL) {
		return(-1);
	}

	cur1 = xmlDocGetRootElement(doc);
	
	if (xmlStrcmp(cur1->name, (const xmlChar *) "ScanToDestinations")) {
		fprintf(stderr,"document of the wrong type, root node != Notifications");
		xmlFreeDoc(doc);
		return -1;
	}

	cur1 = cur1->xmlChildrenNode;
	while (cur1 != NULL) {
		if ((!xmlStrcmp(cur1->name, (const xmlChar *)"ScanToDestinationList"))) {
			cur2 = cur1->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!xmlStrcmp(cur2->name, (const xmlChar *)"ScanToDestination"))) {
					cur3 = cur2->xmlChildrenNode;
					while (cur3 != NULL) {
						if ((!xmlStrcmp(cur3->name, (const xmlChar *)"DeviceDisplay"))) {
							key = xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
							if (key != NULL) {
								if (mode == 1) {
									delScanToDest(phost, key);
								} else {
									printf("Dest: %s\n", key);
								}
								destcnt++;
							}
							xmlFree(key);
						}
						cur3 = cur3->next;
					}
				}
				cur2 = cur2->next;
			}
		}
		cur1 = cur1->next;
	}
	xmlFreeDoc(doc);
	return(destcnt);
}

int delAllScanToDest(char *phost) {
	return(getScanToDest(1, phost));
}

int myScanToDest(int mode, char *phost, char *dest) {
	xmlNanoHTTPInit();

	char * pContentType = 0;
	void * Ctxt = 0;

	char url[1024];
	char host1[256];
	char *host2;
	gethostname(host1, 256);
	host2 = strtok(host1, ".");


	if(mode==1) {
		snprintf(url, 1024, "http://%s/hp/device/set_config.html?RemoveScanToDest_1=%s", phost, dest);
	} else {
		snprintf(url, 1024, "http://%s/hp/device/set_config.html?AddScanToDest_1=127.0.0.1-%s%%5e%s%%5eDestFolder", phost, host2, dest);
	}

	Ctxt = xmlNanoHTTPOpen(url, &pContentType);
	
	if (Ctxt == 0) {
		fprintf(stderr, "ERROR: xmlNanoHTTPOpen() == 0\n");
		return(-1);
	}

	if (xmlNanoHTTPReturnCode(Ctxt) != 200) {
		fprintf(stderr, "ERROR: HTTP Code != OK\n");
		return(-1);
	}
 
	xmlFree(pContentType);
	xmlNanoHTTPClose(Ctxt);
	xmlNanoHTTPCleanup();

	return(0);
}

int addScanToDest(char *phost, char *dest) {
	return(myScanToDest(0, phost, dest));
}

int delScanToDest(char *phost, char *dest) {
	return(myScanToDest(1, phost, dest));
}
