CC = gcc

LIBSTG = liberisestorage.so
OBJSTG = storage.o util/trace.o

LIBJKS = liberiseantijunk.so
OBJJKS = antijunk.o
SQLUDF = postudf.so

EXESVR = erisemaild
OBJSVR = mainsrv.o sslapi.o service.o session.o base.o smtp.o pop.o imap.o calendar.o\
	letter.o smtpclient.o mime.o mta.o fstring.o http.o email.o webmail.o cache.o stgengine.o pool.o\
	util/md5.o	\
	util/DES.o	\
	util/escape.o	\
    util/trace.o    \
	tinyxml/tinyxmlparser.o	\
	tinyxml/tinyxmlerror.o \
	tinyxml/tinyxml.o \
	tinyxml/tinystr.o
	

EXEUTL = eriseutil
OBJUTL = maintools.o base.o util/DES.o stgengine.o
OBJUDF = postudf.o

MDEF = _LINUX_OS_

OPENSSL_DIR=openssl

INCDIR := $(INCDIR) -I../$(OPENSSL_DIR)/include
LDDIRS := $(LDDIRS) -L.
    
ifdef GSSAPI
    GSS_DEF = _WITH_GSSAPI_
    INCDIR := $(INCDIR) -I/usr/local/include/mariadb/
    LDDIRS := $(LDDIRS) -L/usr/local/lib/mariadb/
    LDLIST_SVR = ../$(OPENSSL_DIR)/libssl.a ../$(OPENSSL_DIR)/libcrypto.a -lstdc++ -lrt -ldl -lpthread -lmemcached -lgss
else
    GSS_DEF = _WITHOUT_GSSAPI_
    INCDIR := $(INCDIR) -I/usr/local/include/mariadb/
    LDDIRS := $(LDDIRS) -L/usr/local/lib/mariadb/
    LDLIST_SVR = ../$(OPENSSL_DIR)/libssl.a ../$(OPENSSL_DIR)/libcrypto.a -lstdc++ -lrt -ldl -lpthread -lmemcached
endif

LDLIST_STG = -lstdc++ -lrt -lmariadb -lmemcached
LDLIST_JKS = -lstdc++
LDLIST_UTL = -lstdc++ -lpthread -ldl
LDLIST_UDF = -lstdc++ -lrt

FLAGS = -O2 -fPIC -w

all: stg svr utl jks udf

svr: $(EXESVR)
utl: $(EXEUTL)
stg: $(LIBSTG)
jks: $(LIBJKS)
udf: $(SQLUDF)

$(LIBSTG): $(OBJSTG)
	$(CC) -shared -W -fPIC -o $@ $(OBJSTG) $(LDDIRS) $(LDLIST_STG)

$(LIBJKS): $(OBJJKS)
	$(CC) -shared -W -fPIC -o $@ $(OBJJKS) $(LDLIST_JKS)
	
$(EXESVR): $(OBJSVR)
	$(CC) -o $@ $(OBJSVR) $(LDDIRS) $(LIBSTG) $(LDLIST_SVR)
	
$(EXEUTL): $(OBJUTL)
	$(CC) -o $@ $(OBJUTL) $(LDDIRS) $(LIBSTG) $(LDLIST_UTL)

$(SQLUDF): $(OBJUDF)
	$(CC) -shared -W -fPIC -o $@ $(OBJUDF) $(LDLIST_UDF)
	
%.o : %.cpp
	$(CC) -D$(MDEF) -D$(GSS_DEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(LIBSTG) $(LIBJKS) $(EXESVR) $(EXEUTL) $(SQLUDF) $(OBJSVR) $(OBJJKS) $(OBJSTG) $(OBJUDF) *.elf *.gdb *.o *.a *.so
