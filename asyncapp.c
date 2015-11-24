/*
 * NET-SNMP demo
 *
 * This program demonstrates different ways to query a list of hosts
 * for a list of variables.
 *
 * It would of course be faster just to send one query for all variables,
 * but the intention is to demonstrate the difference between synchronous
 * and asynchronous operation.
 *
 * Niels Baggesen (Niels.Baggesen@uni-c.dk), 1999.
 */

 #include <net-snmp/net-snmp-config.h>

 #if HAVE_STDLIB_H
 #include <stdlib.h>
 #endif
 #if HAVE_UNISTD_H
 #include <unistd.h>
 #endif
 #if HAVE_STRING_H
 #include <string.h>
 #else
 #include <strings.h>
 #endif
 #include <sys/types.h>
 #if HAVE_NETINET_IN_H
 #include <netinet/in.h>
 #endif
 #include <stdio.h>
 #include <ctype.h>
 #if TIME_WITH_SYS_TIME
 # include <sys/time.h>
 # include <time.h>
 #else
 # if HAVE_SYS_TIME_H
 #  include <sys/time.h>
 # else
 #  include <time.h>
 # endif
 #endif
 #if HAVE_SYS_SELECT_H
 #include <sys/select.h>
 #endif
 #if HAVE_NETDB_H
 #include <netdb.h>
 #endif
 #if HAVE_ARPA_INET_H
 #include <arpa/inet.h>
 #endif

 #include <net-snmp/utilities.h>

 #include <net-snmp/net-snmp-includes.h>

 #define NETSNMP_DS_APP_DONT_FIX_PDUS 0
/*
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
*/
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

/*
 * a list of hosts to query
 */
struct host {
  const char *name;
  const char *community;
} hosts[] = {
  { "udp6:[::1]:161",    "public" },
  { NULL }
};

int callbacks = 0;
int maxpoll = 1000;
char myhosts[1000][51];

const char *mycomm = "public";
/*
 * a list of variables to query for
 */
struct oid {
  const char *Name;
  oid Oid[MAX_OID_LEN];
  int OidLen;
} oids[] = {
  { "1.2.3.4.5....1" },
  { NULL }
};

/*
 * initialize
 */
void initialize (void)
{
  struct oid *op = oids;

  /* Win32: init winsock */
  SOCK_STARTUP;

  /* initialize library */
  init_snmp("asynchapp");

  /* parse the oids */
  while (op->Name) {
    op->OidLen = sizeof(op->Oid)/sizeof(op->Oid[0]);
    if (!read_objid(op->Name, op->Oid, &op->OidLen)) {
      snmp_perror("read_objid");
      exit(1);
    }
    op++;
  }
}

/*
 * simple printing of returned data
 */
int print_result (int status, struct snmp_session *sp, struct snmp_pdu *pdu2)
{
  char buf[1024];
  struct variable_list *vp;
  int ix;
  struct timeval now;
  struct timezone tz;
  struct tm *tm;

  gettimeofday(&now, &tz);
  tm = localtime(&now.tv_sec);
  fprintf(stdout, "%.2d:%.2d:%.2d.%.6d ", tm->tm_hour, tm->tm_min, tm->tm_sec,
          now.tv_usec);
  switch (status) {
  case STAT_SUCCESS:
    vp = pdu2->variables;
    if (pdu2->errstat == SNMP_ERR_NOERROR) {
      while (vp) {
        snprint_variable(buf, sizeof(buf), vp->name, vp->name_length, vp);
        fprintf(stdout, "%s: %s\n", sp->peername, buf);
  vp = vp->next_variable;
      }
    }
    else {
      for (ix = 1; vp && ix != pdu2->errindex; vp = vp->next_variable, ix++)
        ;
      if (vp) snprint_objid(buf, sizeof(buf), vp->name, vp->name_length);
      else strcpy(buf, "(none)");
      fprintf(stdout, "%s: %s: %s\n",
        sp->peername, buf, snmp_errstring(pdu2->errstat));
    }
    return 1;
  case STAT_TIMEOUT:
    fprintf(stdout, "%s: Timeout\n", sp->peername);
    return 0;
  case STAT_ERROR:
    snmp_perror(sp->peername);
    return 0;
  }
  return 0;
}

/*****************************************************************************/


/*****************************************************************************/

/*
 * poll all hosts in parallel
 */
struct session {
  struct snmp_session *sess;    /* SNMP session data */
  struct oid *current_oid;    /* How far in our poll are we */
//} sessions[sizeof(hosts)/sizeof(hosts[0])];
} sessions[1000];
int active_hosts;      /* hosts that we have not completed */


netsnmp_pdu    *toppdu;
int oidcount;
size_t          name_length;
//char           *names[SNMP_MAX_CMDLINE_OIDS];
oid             name[MAX_OID_LEN];
char* names[] =
{ "1.2.3.4.5...1" ,
 "1.2.3.4.5...2" };




/*
 * response handler
 */
int asynch_response(int operation, struct snmp_session *sp, int reqid,
        struct snmp_pdu *pdu, void *magic)
{
  struct session *host = (struct session *)magic;
  struct snmp_pdu *req;

  if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
    if (print_result(STAT_SUCCESS, host->sess, pdu)) {
      //host->current_oid++;      /* send next GET (if any) */
  /*    if (host->current_oid->Name) {
  //req = snmp_pdu_create(SNMP_MSG_GET);
  //snmp_add_null_var(req, host->current_oid->Oid, host->current_oid->OidLen);
  req = snmp_clone_pdu(toppdu);
  if (snmp_send(host->sess, req))
    return 1;
  else {
    snmp_perror("snmp_send");
    snmp_free_pdu(req);
  }
    } */
    }
  }
  else
    print_result(STAT_TIMEOUT, host->sess, pdu);

  /* something went wrong (or end of variables)
   * this host not active any more
   */
  active_hosts--;
  return 1;
}

void asynchronous(void)
{
  struct session *hs;
  struct host *hp;

  /* startup all hosts */
  int i;
  //hp = hosts;
  //hp++;
  for (i = 0, hs = sessions; i<maxpoll; hs++, i++) {
    struct snmp_pdu *req;
    struct snmp_session sess;
    snmp_sess_init(&sess);			/* initialize session */
    sess.version = SNMP_VERSION_2c;
    //sess.peername = strdup(hp->name);
    sess.peername = myhosts[i];
    sess.community = mycomm;
    sess.community_len = strlen(sess.community);
    sess.callback = asynch_response;		/* default callback */
    sess.callback_magic = hs;
    if (!(hs->sess = snmp_open(&sess))) {
      snmp_perror("snmp_open");
      continue;
    }
    //hs->current_oid = oids;
    //req = snmp_pdu_create(SNMP_MSG_GET);	/* send the first GET */
    req = snmp_clone_pdu(toppdu);
    //snmp_add_null_var(req, hs->current_oid->Oid, hs->current_oid->OidLen);
    if (snmp_send(hs->sess, req))
      active_hosts++;
    else {
      snmp_perror("snmp_send");
      snmp_free_pdu(req);
    }
  }
  //snmp_free_pdu(req);
  /* loop while any active hosts */

  while (active_hosts) {
    int fds = 0, block = 1;
    fd_set fdset;
    struct timeval timeout;

    FD_ZERO(&fdset);
    snmp_select_info(&fds, &fdset, &timeout, &block);
    fds = select(fds, &fdset, NULL, NULL, block ? NULL : &timeout);
    if (fds < 0) {
        perror("select failed");
        exit(1);
    }
    if (fds)
        snmp_read(&fdset);
    else
        snmp_timeout();
  }

  /* cleanup */

  for (i = 0, hs = sessions;  i<maxpoll; hs++, i++) {
    if (hs->sess) snmp_close(hs->sess);
  }
}

void readfile(char* fname) {
  int i;
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  fp = fopen(fname, "r");
  if (fp == NULL)
      exit(1);
  int c = 0;
  while ((read = getline(&line, &len, fp)) != -1) {
      i = strlen(line)-1;
      if( line[ i ] == '\n')
        line[i] = '\0';
      strcpy(myhosts[c],line);
      //printf("%s\n", myhosts[c]);
      c++;
      if (c == maxpoll) {
        c = 0;
        asynchronous();
      }
  }
  // Need to add some calls to async() to cleanup odd number of entries left.
  if (ferror(fp)) {
      /* handle error */
  }
  free(line);
  fclose(fp);

}
/*****************************************************************************/

int main (int argc, char **argv)
{
  initialize();
  toppdu = snmp_pdu_create(SNMP_MSG_GET);

// the next line needs to be modified for the actual length of names[].
  for (oidcount = 0; oidcount < 9; oidcount++) {
      name_length = MAX_OID_LEN;
      if (!snmp_parse_oid(names[oidcount], name, &name_length)) {
          snmp_perror(names[oidcount]);
      } else
          snmp_add_null_var(toppdu, name, name_length);
  }

  printf("---------- asynchronous -----------\n");
  //asynchronous();
  readfile(argv[1]);
  return 0;
}
