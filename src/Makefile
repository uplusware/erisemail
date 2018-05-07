CC = gcc

LIBLDAPASN1 = libldapasn1.so
LIBSTG = liberisestorage.so
OBJSTG = storage.o util/trace.o

LIBJKS = liberiseantispam.so
OBJJKS = antispam.o

EXESVR = erisemaild
OBJSVR = mainsrv.o sslapi.o service.o session.o xmpp_sessions.o base.o smtp.o pop.o imap.o xmpp.o calendar.o \
	letter.o smtpclient.o mime.o mta.o fstring.o http.o simple_ldap.o asn_1.o dns.o email.o webmail.o cache.o stgengine.o pool.o postnotify.o \
	util/md5.o	\
	util/DES.o	\
	util/escape.o	\
    util/trace.o    \
    util/digcalc.o  \
	util/sha384-512.o  \
	util/usha.o  \
	util/sha224-256.o	\
	util/sha.o	\
	util/hmac.o	\
	tinyxml/tinyxmlparser.o	\
	tinyxml/tinyxmlerror.o \
	tinyxml/tinyxml.o \
	tinyxml/tinystr.o
	

EXEUTL = eriseutil
OBJUTL = maintools.o base.o util/DES.o stgengine.o

SQLUDF = postudf.so
OBJUDF = postudf.o

EXENTY = erisenotify
OBJNTY = erisenotify.o postnotify.o

MDEF = _LINUX_OS_

OPENSSL_DIR = openssl
HADOOP_DIR = hadoop
LIBJVM_DIR = /usr/lib/jvm/default-java/jre/lib/amd64/server/

INCDIR = -I../$(OPENSSL_DIR)/include -I/usr/local/include/mariadb/ -I./ldap_asn1/
LDDIRS = -L. -L/usr/local/lib/mariadb/

LDLIST_SVR = ../$(OPENSSL_DIR)/libssl.a ../$(OPENSSL_DIR)/libcrypto.a -lstdc++ -lrt -ldl -lpthread -lmemcached
LDLIST_STG = -lstdc++ -lrt -lmariadb -lmemcached
LDLIST_JKS = -lstdc++
LDLIST_UTL = -lstdc++ -lpthread -ldl
LDLIST_UDF = -lstdc++ -lpthread
LDLIST_NTY = -lstdc++ -lpthread

ifdef GSSAPI
    GSS_DEF = _WITH_GSSAPI_
    LDLIST_SVR := $(LDLIST_SVR) -lgss
else
    GSS_DEF = _WITHOUT_GSSAPI_
endif

ifdef HDFS
    HDFS_DEF = _WITH_HDFS_
    INCDIR := $(INCDIR) -I../$(HADOOP_DIR)/include
    LDDIRS := $(LDDIRS) -L../$(HADOOP_DIR)/lib/native -L$(LIBJVM_DIR)
    LDLIST_SVR := $(LDLIST_SVR) -lhdfs -ljvm
else
    HDFS_DEF = _WITHOUT_HDFS_
endif

ifdef DIST
    DIST_DEF = _WITH_DIST_
else
    DIST_DEF = _WITHOUT_DIST_
endif

ifdef LDAP
    LDAP_DEF = _WITH_LDAP_
    LDLIST_STG := $(LDLIST_STG) -lldap -llber
else
    LDAP_DEF = _WITHOUT_LDAP_
endif

FLAGS = -O2 -fPIC -w

all: asn1 stg svr utl jks udf nty

asn1: $(LIBLDAPASN1)
svr: $(EXESVR)
utl: $(EXEUTL)
stg: $(LIBSTG)
jks: $(LIBJKS)
udf: $(SQLUDF)
nty: $(EXENTY)

$(LIBLDAPASN1):
	$(MAKE) -C ldap_asn1/

$(LIBSTG): $(OBJSTG)
	$(CC) -shared -W -fPIC -o $@ $(OBJSTG) $(LDDIRS) $(LDLIST_STG)

$(LIBJKS): $(OBJJKS)
	$(CC) -shared -W -fPIC -o $@ $(OBJJKS) $(LDLIST_JKS)
	
$(EXESVR): $(OBJSVR)
	$(CC) -o $@ $(OBJSVR) $(LDDIRS) $(LIBSTG) $(LIBLDAPASN1) $(LDLIST_SVR)
	
$(EXEUTL): $(OBJUTL)
	$(CC) -o $@ $(OBJUTL) $(LDDIRS) $(LIBSTG) $(LDLIST_UTL)

$(SQLUDF): $(OBJUDF)
	$(CC) -shared -W -fPIC -o $@ $(OBJUDF) $(LDLIST_UDF)

$(EXENTY): $(OBJNTY)
	$(CC) -o $@ $(OBJNTY) $(LDDIRS) $(LDLIST_NTY)
	
%.o : %.cpp
	$(CC) -D$(MDEF) -D$(GSS_DEF) -D$(HDFS_DEF) -D$(DIST_DEF) -D$(LDAP_DEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	$(MAKE) -C ldap_asn1/ clean
	-rm -f $(LIBSTG) $(LIBJKS) $(EXESVR) $(EXEUTL) $(SQLUDF) $(EXENTY) $(OBJSVR) $(OBJJKS) $(OBJSTG) $(OBJUDF) $(OBJNTY) *.elf *.gdb *.o *.a *.so
	
