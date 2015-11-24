#!/usr/bin/python

import sys
import cx_Oracle

from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.proto import rfc1902
from Conf import dbargs, comString, oidTuple, sqlQuery
#example for using dbargs
#dbargs = dict()
#dbargs['user'] = 'user'
#dbargs['password'] = 'password'
#dbargs['dsn'] = 'host.example.com/dbname'
con = cx_Oracle.connect(**dbargs)
cursor = con.cursor()
if not con:
    print 'no db con'
    exit

#example for oidTuple:
#oidTuple =   (
#(".1.3.6.1.4.1.1429....",rfc1902.Integer(1)),
#(".1.3.6.1.4.1.1429....",rfc1902.OctetString("SomeString"))
# )

# Wait for responses or errors
def cbFun(sendRequestHandle, errorIndication, errorStatus, errorIndex,
          varBinds, cbCtx):
    (authData, transportTarget) = cbCtx
    print('%s via %s' % (authData, transportTarget))
    if errorIndication:
        print(errorIndication)
        return 1
    if errorStatus:
        print('%s at %s' % (
            errorStatus.prettyPrint(),
            errorIndex and varBinds[int(errorIndex)-1] or '?'
            )
        )
        return 1

    for oid, val in varBinds:
        if val is None:
            print(oid.prettyPrint())
        else:
            print('%s = %s' % (oid.prettyPrint(), val.prettyPrint()))


def getIPs():
    ips = list()
    res = cursor.execute(sqlQuery)
    for item in res:
        ips.append(item[0])
    con.commit()
    con.close()

    return ips

def createTargetList(ips):
    targets = list()
    for ip in ips:
        if ":" in ip:
            dev = (
            ( cmdgen.CommunityData(comString),
            cmdgen.Udp6TransportTarget((ip, 161),timeout=2,retries=5),
            oidTuple
                )
                )
            targets.append(dev)
        else:
            dev = (
            ( cmdgen.CommunityData(comString),
            cmdgen.UdpTransportTarget((ip, 161),timeout=2,retries=5),
            oidTuple
            )
            )
            targets.append(dev)
    return targets



def main(**kwargs):
    ips = getIPs()
    #this breaks down requests to 100 at a time.
    chunks=[ips[x:x+100] for x in xrange(0, len(ips), 100)]
    for chunk in chunks:
        print "Starting new chunk\n"
        #start a new cmdGen for each chunk
        cmdGen  = cmdgen.AsynCommandGenerator()
        targets = createTargetList(chunk)
        for authData, transportTarget, varNames in targets:
            cmdGen.setCmd(
                authData, transportTarget, varNames,
                # User-space callback function and its context
                (cbFun, (authData, transportTarget)),
                lookupNames=True, lookupValues=True
            )
        #This runDispatcher() blocks until the chunk is finished.
        cmdGen.snmpEngine.transportDispatcher.runDispatcher()

if __name__ == "__main__":
    main()
