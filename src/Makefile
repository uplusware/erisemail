CC = gcc

LIBSTG = liberisestorage.so
OBJSTG = storage.o

LIBJKS = liberiseantijunk.so
OBJJKS = antijunk.o

EXESVR = erisemaild
OBJSVR = mainsrv.o service.o session.o base.o smtp.o pop.o imap.o calendar.o\
	letter.o smtpclient.o mime.o spool.o fstring.o http.o email.o cache.o stgengine.o pool.o\
	util/md5.o	\
	util/DES.o	\
	util/escape.o	\
	util/trace.o	\
	tinyxml/tinyxmlparser.o	\
	tinyxml/tinyxmlerror.o \
	tinyxml/tinyxml.o \
	tinyxml/tinystr.o
	

EXEUTL = eriseutil
OBJUTL = maintools.o base.o util/DES.o stgengine.o

ifdef SOLARIS
MDEF = _SOLARIS_OS_
INCDIR = -I/usr/mysql/5.0/include/mysql/
LDDIRS = -L/usr/mysql/5.0/lib/mysql/
LDLIST_STG = -lstdc++ -lrt -lmysqlclient_r
LDLIST_JKS = -lstdc++
LDLIST_SVR = -lstdc++ -lrt -lssl -ldl -lsocket -lnsl -lcrypto -lpthread
LDLIST_UTL = -lstdc++ -lssl -lcrypto -lnsl
else ifdef CYGWIN
MDEF = _CYGWIN_
INCDIR = -I/usr/include/mysql/ 
LDDIRS = 
LDLIST_STG = -lstdc++ -lrt -lmysqlclient_r -lpthread
LDLIST_JKS = -lstdc++
LDLIST_SVR = -lstdc++ -lrt -ldl -lssl -lcrypto -liconv -lpthread
LDLIST_UTL = -lstdc++ -lssl -lcrypto
else
MDEF = _LINUX_OS_
INCDIR = -I/usr/include/mysql/ 
LDDIRS = -L/usr/lib64/mysql/ 
LDLIST_STG = -lstdc++ -lrt -lmysqlclient_r -lpthread
LDLIST_JKS = -lstdc++
LDLIST_SVR = -lstdc++ -lrt -ldl -lssl -lcrypto -lpthread
LDLIST_UTL = -lstdc++ -lssl -lcrypto -lpthread
endif

FLAGS = -O2 -fPIC -w

all: stg svr utl jks

svr: $(EXESVR)
utl: $(EXEUTL)
stg: $(LIBSTG)
jks: $(LIBJKS)

$(LIBSTG): $(OBJSTG)
	$(CC) -shared -W -fPIC -o $@ $(OBJSTG) $(LDDIRS) $(LDLIST_STG)

$(LIBJKS): $(OBJJKS)
	$(CC) -shared -W -fPIC -o $@ $(OBJJKS) $(LDLIST_JKS)
	
$(EXESVR): $(OBJSVR)
	$(CC) -o $@ $(OBJSVR) $(LDDIRS) $(LIBSTG) $(LDLIST_SVR)
	
$(EXEUTL): $(OBJUTL)
	$(CC) -o $@ $(OBJUTL) $(LDDIRS) $(LIBSTG) $(LDLIST_UTL)
	
%.o : %.cpp
	$(CC) -D$(MDEF) $(INCDIR) $(FLAGS) -c $< -o $@

clean:
	-rm -f $(LIBSTG) $(LIBJKS) $(EXESVR) $(EXEUTL) $(OBJSVR) $(OBJJKS) $(OBJSTG) *.elf *.gdb *.o *.a *.so
