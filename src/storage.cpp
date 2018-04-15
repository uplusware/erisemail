/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "storage.h"
#include "util/general.h"
#include "base.h"
#include "util/md5.h"
#include "util/trace.h"
#include "letter.h"
#include "posixname.h"

#define CODE_KEY "qazWSX#$%123"
#define MYSQL_TIMEOUT 5

static void show_error(MYSQL *mysql, const char* sqlcmd, const char* tag = "", BOOL printit = FALSE)
{
    if(!CMailBase::m_close_stderr || printit)   
    {
        fprintf(stderr, "%sMySQL error(%d) [%s] \"%s\" <%s>\n", tag, mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql), sqlcmd);
    }
    else
    {
        CUplusTrace* t = new CUplusTrace(ERISEMAIL_MYSQLERR_LOGNAME, ERISEMAIL_MYSQLERR_LCKNAME);
        t->Write(Trace_Error, "%sMySQL error(%d) [%s] \"%s\" <%s>", tag, mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql), sqlcmd);
        delete t;
    }
}

int inline _mysql_real_query_(MYSQL *mysql, const char *stmt_str, unsigned long length)
{  
#ifdef _DEBUG_MYSQL_API_        
    printf("(%08d): %s\n", length, stmt_str);
#endif /* _DEBUG_MYSQL_API_ */
    return mysql_real_query(mysql, stmt_str, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class MailStorage

BOOL MailStorage::m_lib_inited = FALSE;

void MailStorage::LibInit()
{
    if(!m_lib_inited)
    {
        mysql_library_init(0, NULL, NULL);
        m_lib_inited = TRUE;
    }
}

void MailStorage::LibEnd()
{
    if(m_lib_inited)
    {
        mysql_library_end();
        m_lib_inited = FALSE;
    }
}
    
void MailStorage::SqlSafetyString(string& strInOut)
{
	char * szOut = new char[strInOut.length()* 2 + 1];
    memset(szOut, 0, strInOut.length()* 2 + 1);
	mysql_real_escape_string(&m_hMySQL, szOut, strInOut.c_str(), strInOut.length());
	strInOut = szOut;
	delete szOut;
}

MailStorage::MailStorage(const char* encoding, const char* private_path, memcached_st * memcached)
{
    m_encoding = encoding;
    m_private_path = private_path;
    
    m_is_opened = FALSE;
    m_is_inited = FALSE;
    m_is_in_transcation = FALSE;
    m_memcached = memcached;
    srandom(time(NULL));

#ifdef _WITH_LDAP_    
    m_ldap = NULL;
    m_is_ldap_binded = FALSE;
#endif /*_WITH_LDAP_*/ 
}

MailStorage::~MailStorage()
{
	Close();
    pthread_rwlock_destroy(&m_userpwd_cache_lock);
}

static unsigned int timeout_val = MYSQL_TIMEOUT;

int MailStorage::Connect(const char * host, const char* username, const char* password, const char* database, unsigned short port, const char* sock_file)
{
    m_host = host;
    m_username = username;
    m_password = password;
    m_port = port;
    m_sock_file = sock_file;
    m_database = database != NULL ? database: "";
    
    if(!m_is_inited)
    {
        mysql_init(&m_hMySQL);
        m_is_inited = TRUE;
    }
    mysql_optionsv(&m_hMySQL, MYSQL_OPT_CONNECT_TIMEOUT, (const void*)&timeout_val);
    mysql_optionsv(&m_hMySQL, MYSQL_OPT_READ_TIMEOUT, (const void*)&timeout_val);
    mysql_optionsv(&m_hMySQL, MYSQL_OPT_WRITE_TIMEOUT, (const void*)&timeout_val);

    if(mysql_real_connect(&m_hMySQL, host, username, password, database, port, sock_file && sock_file[0] != '\0' ? sock_file : NULL, 0) != NULL)
    {
        m_is_opened = TRUE;
    }
    else
    {
        show_error(&m_hMySQL, "CONNECT", "Connect: ");
        return -1;	
    }
    
#ifdef _WITH_LDAP_
    m_ldap_server_uri = CMailBase::m_ldap_sever_uri;
    
    ldap_initialize(&m_ldap, m_ldap_server_uri.c_str());
    
    int protocol_version = CMailBase::m_ldap_sever_version;
    int rc = ldap_set_option(m_ldap, LDAP_OPT_PROTOCOL_VERSION, &protocol_version);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_set_option: %s\n", ldap_err2string(rc));
        return(1);
    }
#endif /*_WITH_LDAP_*/ 

    return 0;
}

void MailStorage::Close()
{
    if(m_is_inited)
    {
        mysql_close(&m_hMySQL);
        m_is_inited = FALSE;
    }
    m_is_opened = FALSE;
#ifdef _WITH_LDAP_
    if(m_ldap)
    {
        ldap_unbind(m_ldap);
        m_ldap = NULL;
        m_is_ldap_binded = FALSE;
    }
#endif /*_WITH_LDAP_*/ 
}

int MailStorage::Ping()
{
	if(m_is_opened)
	{
	    int rc = mysql_ping(&m_hMySQL);
		return rc;
	}
    
    return 0;
}

void MailStorage::KeepLive()
{
	if(Ping() != 0)
	{
		Close();
		Connect(m_host.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), m_port, m_sock_file == "" ? NULL : m_sock_file.c_str());
	}
}

int MailStorage::Query(const char *stmt_str, unsigned long length)
{
	if(m_is_opened)
	{
	    KeepLive();
        
        int r_val = _mysql_real_query_(&m_hMySQL, stmt_str, length);
		
        if(r_val != 0 && m_is_in_transcation
            && strncasecmp(stmt_str, "BEGIN", 5) != 0
            && strncasecmp(stmt_str, "START TRANSACTION", 17) != 0
            && strncasecmp(stmt_str, "COMMIT", 6) != 0
            && strncasecmp(stmt_str, "ROLLBACK", 8) != 0)
        {
            _mysql_real_query_(&m_hMySQL, "ROLLBACK", 8);
        }
        
        return r_val;
	}
	else
	{
		return -1;
	}
}

int MailStorage::StartTransaction()
{
    if(m_is_opened)
	{
	    KeepLive();
        
        if(!m_is_in_transcation)
        {
            if(_mysql_real_query_(&m_hMySQL, "BEGIN", 5) != 0)
            {
                show_error(&m_hMySQL, "BEGIN");
                return -1;
            }
            
            m_is_in_transcation = TRUE;
        }
        return 0;
	}
	else
	{
		return -1;
	}
}

int MailStorage::CommitTransaction()
{
    if(m_is_opened)
	{
	    KeepLive();
        
        if(m_is_in_transcation)
        {
            m_is_in_transcation = FALSE;
            
            if(_mysql_real_query_(&m_hMySQL, "COMMIT", 6) != 0)
            {
                show_error(&m_hMySQL, "COMMIT");
                return -1;
            }
            
        }
        return 0;
	}
	else
	{
		return -1;
	}
}

int MailStorage::RollbackTransaction()
{
    if(m_is_opened)
	{
	    KeepLive();
        
        if(m_is_in_transcation)
        {
            m_is_in_transcation = FALSE;
            
            if(_mysql_real_query_(&m_hMySQL, "ROLLBACK", 8) != 0)
            {
                show_error(&m_hMySQL, "ROLLBACK");
                return -1;
            }            
        }
        return 0;
	}
	else
	{
		return -1;
	}
}
 
int MailStorage::Install(const char* database)
{    
	char sqlcmd[1024];
	if(strcasecmp(m_encoding.c_str(),"GB2312") == 0)
	{
		sprintf(sqlcmd,"CREATE DATABASE IF NOT EXISTS %s DEFAULT CHARACTER SET gb2312 COLLATE gb2312_chinese_ci", database);
	}
	else if(strcasecmp(m_encoding.c_str(),"UTF-8") == 0)
	{
		sprintf(sqlcmd,"CREATE DATABASE IF NOT EXISTS %s DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci", database);
	}
	else if(strcasecmp(m_encoding.c_str(),"UCS2") == 0)
	{
		sprintf(sqlcmd,"CREATE DATABASE IF NOT EXISTS %s DEFAULT CHARACTER SET ucs2 COLLATE ucs2_general_ci", database);
	}
	else
	{
		fprintf(stderr, "Only support UTF-8, GB2312 and UCS2 encoding.\n");
		return -1;
	}
    
    //start a transaction
    if(StartTransaction() != 0)
        return -1;
    
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}

	sprintf(sqlcmd, "USE %s", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    	
	//Create level table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`leveltbl` ("
		"`lid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`lname` VARCHAR( 64 ) NOT NULL ,"
		"`ldescription` LONGTEXT NOT NULL ,"
		"`lmailmaxsize` INT UNSIGNED NOT NULL ,"
		"`lboxmaxsize` INT UNSIGNED NOT NULL ,"
		"`lenableaudit` INT NOT NULL ,"
		"`lmailsizethreshold` INT UNSIGNED NOT NULL ,"
		"`lattachsizethreshold` INT UNSIGNED NOT NULL ,"
		"`ldefault` INT NOT NULL ,"
		"`ltime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `lid` ) ) ENGINE = InnoDB",
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX leveltbl1a ON %s.leveltbl (ldefault)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
	//Create dir table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`dirtbl` ("
		"`did` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`dname` VARCHAR( 256 ) NOT NULL ,"
		"`downer` VARCHAR( 64 ) NOT NULL ,"
		"`dusage` INT NOT NULL DEFAULT '0' ,"
		"`dparent` INT NOT NULL DEFAULT '-1' ,"
		"`dstatus` INT UNSIGNED NULL DEFAULT '0' ,"
		"`dtime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `did` ) ) ENGINE = InnoDB",
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
    //Create index dir table
	sprintf(sqlcmd, "CREATE INDEX dirtbl3a ON %s.dirtbl (downer, dname, dparent)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX dirtbl2a ON %s.dirtbl (downer, did)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX dirtbl1a ON %s.dirtbl (downer)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
	//Crate User table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`usertbl` ("
		"`uid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`uname` VARCHAR( 64 ) NOT NULL ,"
		"`upasswd` BLOB NOT NULL ,"
		"`ualias` VARCHAR( 256 ) NOT NULL ,"
        "`uhost` VARCHAR( 256 ) NULL ,"
		"`utype` INT UNSIGNED NOT NULL ,"
		"`urole` INT UNSIGNED NOT NULL ,"
		"`usize` INT UNSIGNED NOT NULL DEFAULT %d ,"
		"`ustatus` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`ulevel` INT NOT NULL DEFAULT '0' ,"
		"`utime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `uid` ) ) ENGINE = InnoDB",
		database, MAX_EMAIL_LEN);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
    sprintf(sqlcmd, "CREATE INDEX usertbl2a ON %s.usertbl (ustatus, utype)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX usertbl2b ON %s.usertbl (uname, utype)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX usertbl1a ON %s.usertbl (uname)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
	//Create group table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`grouptbl` ("
		"`gid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`groupname` VARCHAR( 64 ) NOT NULL ,"
		"`membername` VARCHAR( 64 ) NOT NULL ,"
		"`gtime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `gid` ) ) ENGINE = InnoDB",
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    sprintf(sqlcmd, "CREATE INDEX grouptbl1a ON %s.grouptbl (groupname)", database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
	//Create mail table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`mailtbl` ("
		"`mid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`muniqid` VARCHAR( 256 ) NOT NULL ,"
		"`mfrom` VARCHAR( 256 ) NULL ,"
		"`mto` VARCHAR( 256 ) NULL ,"
		"`mbody` VARCHAR( 256 ) NOT NULL ,"
		"`msize` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtime` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtx` INT UNSIGNED NOT NULL ,"
		"`mstatus` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`mdirid` INT NOT NULL DEFAULT -1 ,"
		"PRIMARY KEY ( `mid` ) ) ENGINE = InnoDB", 
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
    //Create extern mail table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`extmailtbl` ("
		"`mid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`muniqid` VARCHAR( 256 ) NOT NULL ,"
		"`mhost` VARCHAR( 256 ) NULL ,"
        "`mfrom` VARCHAR( 256 ) NULL ,"
		"`mto` VARCHAR( 256 ) NULL ,"
		"`mbody` VARCHAR( 256 ) NOT NULL ,"
		"`msize` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtime` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtx` INT UNSIGNED NOT NULL ,"
		"`mstatus` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`mdirid` INT NOT NULL DEFAULT -1 ,"
        "`tried_count` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`next_fwd_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
		"PRIMARY KEY ( `mid` ) ) ENGINE = InnoDB", 
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    //Create mail body table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`mbodytbl` ("
		"`mid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`mbody` VARCHAR( 256 ) NOT NULL ,"
		"`mfragment` LONGTEXT NOT NULL ,"
        "`mtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
		"PRIMARY KEY ( `mid` ) ) ENGINE = InnoDB",
		database);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    //Create MTA list table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`mtatbl` ("
		"`mid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`mta` VARCHAR( 256 ) NOT NULL ,"
		"`active_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
		"PRIMARY KEY ( `mid` ) ) ENGINE = InnoDB",
		database);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    //Create xmpp table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`xmpptbl` ("
		"`xid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`xfrom` VARCHAR( 256 ) NOT NULL ,"
		"`xto` VARCHAR( 256 ) NOT NULL ,"
		"`xmessage` LONGTEXT NOT NULL,"
        "`xtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
		"PRIMARY KEY ( `xid` ) ) ENGINE = InnoDB",
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    //Create xmpp susbcribe table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`xmppbuddytbl` ("
		"`xid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`xusr1` VARCHAR( 256 ) NOT NULL ,"
		"`xusr2` VARCHAR( 256 ) NOT NULL ,"
        "`xtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
		"PRIMARY KEY ( `xid` ) ) ENGINE = InnoDB",
		database);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
    
    unsigned int lid;
	if(AddLevel("default", "The system's default level", 5000*1024, 500000*1024, eaFalse, 5000*1024, 5000*1024, lid) == 0)
	{
		SetDefaultLevel(lid);
	}
	else
	{
        printf("Add level wrong\n");
        RollbackTransaction();
		return -1;
	}	
#ifdef _WITH_DIST_
	if(CMailBase::m_is_master)
	{
		if(AddID("admin", "admin", "Administrator", CMailBase::m_master_hostname.c_str(), utMember, urAdministrator, MAX_EMAIL_LEN, -1) == -1)
#else
		if(AddID("admin", "admin", "Administrator", "", utMember, urAdministrator, MAX_EMAIL_LEN, -1) == -1)
#endif /*  _WITH_DIST_ */
		{
			printf("Add admin id wrong\n");
			RollbackTransaction();
			return -1;
		}
#ifdef _WITH_DIST_
	}
#endif /*  _WITH_DIST_ */

#ifdef POSTMAIL_NOTIFY    
	sprintf(sqlcmd, "DROP FUNCTION IF EXISTS post_notify");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
	sprintf(sqlcmd, "CREATE FUNCTION post_notify RETURNS STRING SONAME \"postudf.so\"");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
	sprintf(sqlcmd, "DROP TRIGGER IF EXISTS postmail_notify_insert");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
	sprintf(sqlcmd, "DROP TRIGGER IF EXISTS postmail_notify_update");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
	sprintf(sqlcmd, "CREATE TRIGGER postmail_notify_insert AFTER INSERT ON extmailtbl FOR EACH ROW BEGIN SET @rs = post_notify(); END");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
	
	sprintf(sqlcmd, "CREATE TRIGGER postmail_notify_update AFTER UPDATE ON extmailtbl FOR EACH ROW BEGIN SET @rs = post_notify(); END");
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd, "Install: ", TRUE);
		return -1;
	}
#endif /* POSTMAIL_NOTIFY */

    if(CommitTransaction() != 0)
        return -1;
    
	return 0;
        
}

int MailStorage::Uninstall(const char* database)
{
	char sqlcmd[1024];
	sprintf(sqlcmd,"DROP DATABASE %s", database);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	return -1;
}

int MailStorage::SetMailSize(unsigned int mid, unsigned int msize)
{
	char sqlcmd[1024];
	sprintf(sqlcmd,"UPDATE mailtbl SET msize=%d WHERE mid=%d", msize, mid);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
	return 0;
}

int MailStorage::CheckAdmin(const char* username, const char* password)
{
#ifdef _WITH_DIST_
    if(!CMailBase::m_is_master)
    {
        return -1;
    }
#endif /* _WITH_DIST_ */
    
    if(strcmp(username, "") == 0 || strcmp(password, "") == 0)
        return -1;
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
#ifdef _WITH_DIST_ 
    string strSafetyHost = CMailBase::m_localhostname;
    SqlSafetyString(strSafetyHost);
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND DECODE(upasswd,'%s') = '%s' AND ustatus = %d AND urole=%d AND utype = %d and uhost IN ('%s', '')", strSafetyUsername.c_str(), CODE_KEY, password, usActive, urAdministrator, utMember, strSafetyHost.c_str());
#else
    sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND DECODE(upasswd,'%s') = '%s' AND ustatus = %d AND urole=%d AND utype = %d", strSafetyUsername.c_str(), CODE_KEY, password, usActive, urAdministrator, utMember);
#endif /* _WITH_DIST_ */
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if( mysql_num_rows(query_result) > 0 )
			{
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::CheckLogin(const char* username, const char* password)
{
    if(strcmp(username, "") == 0 || strcmp(password, "") == 0)
        return -1;
        
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
    
#ifdef _WITH_LDAP_
	if(!m_is_ldap_binded)
	{
		int rc = ldap_simple_bind(m_ldap, CMailBase::m_ldap_manager.c_str(), CMailBase::m_ldap_manager_passwd.c_str());
		
		if(rc == -1)
		{
			fprintf(stderr, "ldap_simple_bind: %s\n", ldap_err2string(rc));
			return -1;
		}
	}
	
	m_is_ldap_binded = TRUE;
	
	int msgid = -1;
	
	const char* attrs[2];
	attrs[0] = CMailBase::m_ldap_search_attribute_user_password.c_str();
	attrs[1] = 0;
	
	char szFilter[1025];
	snprintf(szFilter, 1024, CMailBase::m_ldap_search_filter_user.c_str(), username);
	szFilter[1024] = '\0';
	
	int rc = ldap_search_ext(m_ldap, CMailBase::m_ldap_search_base.c_str(), LDAP_SCOPE_SUBTREE, szFilter, NULL, 0, NULL, NULL, NULL, 1, &msgid);
	
	if( rc != LDAP_SUCCESS )
	{
		fprintf(stderr, "ldap_search_ext_s: %s\n", ldap_err2string(rc));
		return( rc );
	}
	
	struct berval  ** uid_password = NULL;
	LDAPMessage *search_result = NULL;
	ldap_result(m_ldap, msgid, 0, NULL, &search_result);
	
	int entries = ldap_count_entries(m_ldap, search_result);

	if(search_result && entries > 0)
	{
		for( LDAPMessage * single_entry = ldap_first_entry( m_ldap, search_result ); single_entry != NULL; single_entry = ldap_next_entry( m_ldap, search_result ))
		{
			uid_password = ldap_get_values_len(m_ldap, single_entry, CMailBase::m_ldap_search_attribute_user_password.c_str());
			if(uid_password && *uid_password)
			{
				char* ldap_password = (char*)malloc((*uid_password)->bv_len + 1);
				memcpy(ldap_password, (*uid_password)->bv_val, (*uid_password)->bv_len);
				ldap_password[(*uid_password)->bv_len] = '\0';
				ldap_value_free_len(uid_password);
                
				if(strcmp(password, ldap_password) == 0)
				{
                    CheckRequiredDir(username);
					free(ldap_password);
                    return 0;
				}
				else
				{
					free(ldap_password);
					return -1;
				}
			}
		}
		ldap_msgfree(search_result);
	}
	else
	{
		return -1;
	}
#endif /* _WITH_LDAP_ */
	
#ifdef _WITH_DIST_ 
	string strSafetyHost = CMailBase::m_localhostname;
	SqlSafetyString(strSafetyHost);
	sprintf(sqlcmd, "SELECT DECODE(upasswd,'%s') FROM usertbl WHERE uname = '%s' AND ustatus = %d AND utype = %d and uhost IN ('%s', '')",
		CODE_KEY, username, usActive, utMember, strSafetyHost.c_str());
#else
	sprintf(sqlcmd, "SELECT DECODE(upasswd,'%s') FROM usertbl WHERE ustatus = %d AND utype = %d", CODE_KEY, username, usActive, utMember);
#endif /* _WITH_DIST_ */
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if((row = mysql_fetch_row(query_result)) && strcmp(row[0], password) == 0)
			{
				mysql_free_result(query_result);
                CheckRequiredDir(username);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetPassword(const char* username, string& password)
{
	string strpwd;
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
#ifdef _WITH_DIST_ 
	string strSafetyHost = CMailBase::m_localhostname;
	SqlSafetyString(strSafetyHost);
	sprintf(sqlcmd, "SELECT DECODE(upasswd,'%s') FROM usertbl WHERE uname='%s' AND utype=%d AND uhost='%s'", CODE_KEY, strSafetyUsername.c_str(), utMember, strSafetyHost.c_str());
#else
    sprintf(sqlcmd, "SELECT DECODE(upasswd,'%s') FROM usertbl WHERE uname='%s' AND utype=%d", CODE_KEY, strSafetyUsername.c_str(), utMember);
#endif /* _WITH_DIST_ */	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if( mysql_num_rows(query_result) == 1 )
			{
				MYSQL_ROW row;
				row = mysql_fetch_row(query_result);
				if(row == NULL)
				{
					mysql_free_result(query_result);
					return -1;
				}
				else
				{
					password = row[0];
#ifdef _WITH_LDAP_
					if(!m_is_ldap_binded)
					{
						int rc = ldap_simple_bind(m_ldap, CMailBase::m_ldap_manager.c_str(), CMailBase::m_ldap_manager_passwd.c_str());
						
						if(rc == -1)
						{
							fprintf(stderr, "ldap_simple_bind: %s\n", ldap_err2string(rc));
							return -1;
						}
					}
					
					m_is_ldap_binded = TRUE;
					
					int msgid = -1;
					
					const char* attrs[2];
					attrs[0] = CMailBase::m_ldap_search_attribute_user_password.c_str();
					attrs[1] = 0;
					
					char szFilter[1025];
					snprintf(szFilter, 1024, CMailBase::m_ldap_search_filter_user.c_str(), username);
					szFilter[1024] = '\0';
					
					int rc = ldap_search_ext(m_ldap, CMailBase::m_ldap_search_base.c_str(), LDAP_SCOPE_SUBTREE, szFilter, NULL, 0, NULL, NULL, NULL, 1, &msgid);
					
					if( rc != LDAP_SUCCESS )
					{
						fprintf(stderr, "ldap_search_ext_s: %s\n", ldap_err2string(rc));
						return( rc );
					}

					
					struct berval  ** uid_password = NULL;
					LDAPMessage *search_result = NULL;
					ldap_result(m_ldap, msgid, 0, NULL, &search_result);
					
					int entries = ldap_count_entries(m_ldap, search_result);

					if(search_result && entries > 0)
					{
						for( LDAPMessage * single_entry = ldap_first_entry( m_ldap, search_result ); single_entry != NULL; single_entry = ldap_next_entry( m_ldap, search_result ))
						{
							uid_password = ldap_get_values_len(m_ldap, single_entry, CMailBase::m_ldap_search_attribute_user_password.c_str());
							if(uid_password && *uid_password)
							{
								char* ldap_password = (char*)malloc((*uid_password)->bv_len + 1);
								memcpy(ldap_password, (*uid_password)->bv_val, (*uid_password)->bv_len);
								ldap_password[(*uid_password)->bv_len] = '\0';
								ldap_value_free_len(uid_password);
								
								password = ldap_password;
								free(ldap_password);
							}
						}
						ldap_msgfree(search_result);
					}
					else
					{
						return -1;
					}
#endif /* _WITH_LDAP_ */
					mysql_free_result(query_result);
					return 0;
				}
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetHost(const char* username, string& host)
{
#ifdef _WITH_DIST_    
	string strpwd;
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT uhost FROM usertbl WHERE uname='%s' AND utype=%d", strSafetyUsername.c_str(), utMember);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if( mysql_num_rows(query_result) == 1 )
			{
				MYSQL_ROW row;
				row = mysql_fetch_row(query_result);
				if(row == NULL)
				{
					mysql_free_result(query_result);
					return -1;
				}
				else
				{
					host = row[0] ? row[0] : "";
					mysql_free_result(query_result);
					return 0;
				}
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
#else
    host = "";
    return 0;
#endif /*_WITH_DIST_ */
}

int MailStorage::VerifyUser(const char* username)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND utype=%d", strSafetyUsername.c_str(), utMember);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if( mysql_num_rows(query_result) > 0 )
			{
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::VerifyGroup(const char* groupname)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND utype=%d", groupname, utGroup);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			if( mysql_num_rows(query_result) > 0 )
			{	
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::AddLevel(const char* lname, const char* ldescription, unsigned long long mailmaxsize, unsigned long long boxmaxsize, EnableAudit lenableaudit, unsigned int mailsizethreshold, unsigned int attachsizethreshold, unsigned int& lid)
{
	char sqlcmd[1024];

	string strSafetyLevelname;
	strSafetyLevelname = lname;
	SqlSafetyString(strSafetyLevelname);
	
	string strSafetyDescription;
	strSafetyDescription = ldescription;
	SqlSafetyString(strSafetyDescription);

	if(strSafetyLevelname == "")
	{
		return -1;
	}

	if(strSafetyDescription == "")
	{
		strSafetyDescription = strSafetyLevelname;
	}
	
	sprintf(sqlcmd, "INSERT INTO leveltbl(lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime) VALUES('%s', '%s', %llu, %llu, %d, %d, %d, %d, %d)",
		strSafetyLevelname.c_str(), strSafetyDescription.c_str(), mailmaxsize, boxmaxsize, lenableaudit, mailsizethreshold, attachsizethreshold, ldFalse, time(NULL));

	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		lid = mysql_insert_id(&m_hMySQL);
		return 0;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::UpdateLevel(unsigned int lid, const char* lname, const char* ldescription, unsigned long long mailmaxsize, unsigned long long boxmaxsize, EnableAudit lenableaudit, unsigned int mailsizethreshold, unsigned int attachsizethreshold)
{
	char sqlcmd[1024];
	string strSafetyLevelname;
	strSafetyLevelname = lname;
	SqlSafetyString(strSafetyLevelname);
	if(strSafetyLevelname == "")
		return -1;
		
	string strSafetyDescription;
	strSafetyDescription = ldescription;
	SqlSafetyString(strSafetyDescription);
	if(strSafetyDescription == "")
		strSafetyDescription = strSafetyLevelname;
		
	sprintf(sqlcmd, "UPDATE leveltbl SET lname='%s', ldescription='%s', lmailmaxsize=%llu, lboxmaxsize=%llu, lenableaudit=%d, lmailsizethreshold=%d, lattachsizethreshold=%d WHERE lid=%d",
		strSafetyLevelname.c_str(), strSafetyDescription.c_str(), mailmaxsize, boxmaxsize, lenableaudit, mailsizethreshold, attachsizethreshold, lid);

	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::DelLevel(unsigned int lid)
{
	char sqlcmd[1024];
	int defaultLid;
	if(GetDefaultLevel(defaultLid) != 0)
        return -1;
	
    //start a transaction
    if(StartTransaction() != 0)
        return -1;
    
	sprintf(sqlcmd, "UPDATE usertbl SET ulevel=%d WHERE ulevel=%d", defaultLid, lid);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{	
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
	
	sprintf(sqlcmd, "DELETE FROM leveltbl WHERE lid=%d AND ldefault <> %d", lid, ldTrue);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
    
    if(CommitTransaction() != 0)
        return -1;
    
	return 0;
}

int MailStorage::SetDefaultLevel(unsigned int lid)
{	
	char sqlcmd[1024];

    //start a transaction
    if(StartTransaction() != 0)
        return -1;
    
	sprintf(sqlcmd, "UPDATE leveltbl SET ldefault = %d WHERE ldefault = %d", ldFalse, ldTrue);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
	
	sprintf(sqlcmd, "UPDATE leveltbl SET ldefault = %d WHERE lid = %d", ldTrue, lid);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
    
    if(CommitTransaction() != 0)
        return -1;
    
	return 0;
}

int MailStorage::GetUserLevel(const char* username, Level_Info& linfo)
{
	User_Info uinfo;
	if(GetID(username, uinfo) == -1)
		return -1;

	if(GetLevel(uinfo.level, linfo) == -1)
		return -1;

	return 0;
}

int MailStorage::SetUserLevel(const char* username, int lid)
{
	char sqlcmd[1024];
	string strSafetyUsername;
	strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	
	sprintf(sqlcmd, "UPDATE usertbl SET ulevel=%d WHERE uname='%s'", lid, strSafetyUsername.c_str());
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::GetDefaultLevel(int& lid)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid FROM leveltbl WHERE ldefault = %d", ldTrue);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				lid = row[0] == NULL ? 0 : atoi(row[0]);
			}
			else
			{
				show_error(&m_hMySQL, sqlcmd);
				mysql_free_result(query_result);
				return -1;
			}

			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDefaultLevel(Level_Info& liinfo)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid, lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime  FROM leveltbl WHERE ldefault = %d", ldTrue);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				liinfo.lid = atoi(row[0]);
				liinfo.lname = row[1];
				liinfo.ldescription = row[2];
				liinfo.mailmaxsize = atollu(row[3]);
				liinfo.boxmaxsize = atollu(row[4]);
				liinfo.enableaudit = (EnableAudit)atoi(row[5]);
				liinfo.mailsizethreshold = atoi(row[6]);
				liinfo.attachsizethreshold = atoi(row[7]);
				liinfo.ldefault = (LevelDeault)atoi(row[8]);
				liinfo.ltime = atoi(row[9]);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}

			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListLevel(vector<Level_Info>& litbl)
{
	litbl.clear();
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid, lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime FROM leveltbl ORDER BY ltime");
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{		
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{			
			while((row = mysql_fetch_row(query_result)))
			{
				Level_Info li;

				li.lid = atoi(row[0]);
				li.lname = row[1];
				li.ldescription = row[2];
				li.mailmaxsize = atollu(row[3]);
				li.boxmaxsize = atollu(row[4]);
				li.enableaudit = (EnableAudit)atoi(row[5]);
				li.mailsizethreshold = atoi(row[6]);
				li.attachsizethreshold = atoi(row[7]);
				li.ldefault = (LevelDeault)atoi(row[8]);
				li.ltime = atoi(row[9]);
				litbl.push_back(li);
			}

			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
		
}

int MailStorage::GetLevel(int lid, Level_Info& linfo)
{
	char sqlcmd[1024];
	if(lid != -1)
	{
		sprintf(sqlcmd, "SELECT lid, lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime FROM leveltbl WHERE lid=%d", lid);
	}
	else
	{
		return -1;
	}
	
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{		
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				linfo.lid = atoi(row[0]);
				linfo.lname = row[1];
				linfo.ldescription = row[2];
				linfo.mailmaxsize = atollu(row[3]);
				linfo.boxmaxsize = atollu(row[4]);
				linfo.enableaudit = (EnableAudit)atoi(row[5]);
				linfo.mailsizethreshold = atoi(row[6]);
				linfo.attachsizethreshold = atoi(row[7]);
				linfo.ldefault = (LevelDeault)atoi(row[8]);
				linfo.ltime = atoi(row[9]);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
		
}

int MailStorage::CheckRequiredDir(const char* username)
{
    char sqlcmd[2048];
    string strSafetyUsername;
    strSafetyUsername = username;
    SqlSafetyString(strSafetyUsername);

    sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) SELECT 'Inbox','%s',-1, %d, %d, %d FROM dual WHERE NOT EXISTS (SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d)",
        strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duInbox, time(NULL), strSafetyUsername.c_str(), duInbox);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }
        
    sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) SELECT 'Sent','%s',-1, %d, %d, %d FROM dual WHERE NOT EXISTS (SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d)",
        strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duSent, time(NULL), strSafetyUsername.c_str(), duSent);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }

    sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) SELECT 'Drafts','%s',-1, %d, %d, %d FROM dual WHERE NOT EXISTS (SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d)",
        strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duDrafts, time(NULL), strSafetyUsername.c_str(), duDrafts);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }

    sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) SELECT 'Trash','%s',-1, %d, %d, %d FROM dual WHERE NOT EXISTS (SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d)",
        strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duTrash, time(NULL), strSafetyUsername.c_str(), duTrash);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }

    sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) SELECT 'Junk','%s',-1, %d, %d, %d FROM dual WHERE NOT EXISTS (SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d)",
        strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duJunk, time(NULL), strSafetyUsername.c_str(), duJunk);
    if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }
    
    return 0;
}

int MailStorage::AddID(const char* username, const char* password, const char* alias, const char* host, UserType type, UserRole role, unsigned int size, int level)
{
	if(strcasecmp(username, "postmaster") != 0)
	{
        for(int x = 0; x < strlen(username); x++)
        {
            if((username[x] >= 'a' && username[x] <= 'z') 
                || (username[x] >= 'A' && username[x] <= 'Z')
                || (username[x] >= '0' && username[x] <= '9')
                || (username[x] == '_' || username[x] == '.'))
            {
                //works fine
            }
            else
            {
                return -1;
            }
        }
        
        for(int x = 0; x < strlen(password); x++)
        {
            if(password[x] > 126 || password[x] < 32)
            {
                return -1;
            }
        }
        
		if((VerifyUser(username) == -1) && (VerifyGroup(username) == -1))
		{
			char sqlcmd[2048];

			string strSafetyUsername;
			strSafetyUsername = username;
			SqlSafetyString(strSafetyUsername);

			string strSafetyPassword;
			strSafetyPassword = password;
			SqlSafetyString(strSafetyPassword);

			string strSafetyAlias;
			strSafetyAlias = alias;
			SqlSafetyString(strSafetyAlias);

			if(strSafetyUsername == "" || strSafetyPassword == "")
			{
				return -1;
			}

			if(strSafetyAlias == "")
			{
				strSafetyAlias = strSafetyUsername;
			}
			
			int defaultLevel = -1;

			//printf("%d\n", level);
			if(level == -1)
			{
				if(GetDefaultLevel(defaultLevel) != 0)
					defaultLevel = -1;
			}
			else
            {
				defaultLevel = level;
            }
            
            //start a transaction
            if(StartTransaction() != 0)
                return -1;
            
#ifdef _WITH_DIST_
            string strSafetyHost;  
			strSafetyHost = host;
			SqlSafetyString(strSafetyHost);
            
            sprintf(sqlcmd, "INSERT INTO usertbl(uname, upasswd, ualias, uhost, utype, urole, usize, ustatus, ulevel, utime) VALUES('%s', ENCODE('%s','%s'), '%s', '%s', %d, %d, %d, 0, %d, %d)",
				strSafetyUsername.c_str(), strSafetyPassword.c_str(), CODE_KEY, strSafetyAlias.c_str(), strSafetyHost.c_str(), type, role, size, defaultLevel, time(NULL));
                
#else
            sprintf(sqlcmd, "INSERT INTO usertbl(uname, upasswd, ualias, utype, urole, usize, ustatus, ulevel, utime) VALUES('%s', ENCODE('%s','%s'), '%s', %d, %d, %d, 0, %d, %d)",
				strSafetyUsername.c_str(), strSafetyPassword.c_str(), CODE_KEY, strSafetyAlias.c_str(), type, role, size, defaultLevel, time(NULL));
#endif /* _WITH_DIST_ */
				
			if(Query(sqlcmd, strlen(sqlcmd)) != 0)
			{
				show_error(&m_hMySQL, sqlcmd);
				return -1;
			}
            
            if(CommitTransaction() != 0)
                return -1;
            
            return 0;
		}
	}
	else
	{
		return -1;
	}
    
    return -1;
}

int MailStorage::UpdateID(const char* username, const char* alias, const char* host, UserStatus status, int level)
{
	char sqlcmd[1024];

	string strSafetyUsername;
	strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);

	if(strSafetyUsername == "")
	{
		return -1;
	}
	
	string strSafetyAlias;
	strSafetyAlias = alias;
	SqlSafetyString(strSafetyAlias);
	if(strSafetyAlias == "")
		strSafetyAlias = strSafetyUsername;
		
	int realLevel = -1;

	if(level == -1)
	{
		if(GetDefaultLevel(realLevel) != 0)
			realLevel = -1;
	}
	else
		realLevel = level;

#ifdef _WITH_DIST_
    string strSafetyHost;          
    strSafetyHost = host;
    SqlSafetyString(strSafetyHost);
    sprintf(sqlcmd, "UPDATE usertbl SET ualias='%s', ustatus=%d, ulevel=%d, uhost='%s' WHERE uname='%s'",
        strSafetyAlias.c_str(), status, realLevel, strSafetyHost.c_str(), strSafetyUsername.c_str());

#else
    sprintf(sqlcmd, "UPDATE usertbl SET ualias='%s', ustatus=%d, ulevel=%d WHERE uname='%s'",
        strSafetyAlias.c_str(), status, realLevel, strSafetyUsername.c_str());
#endif /* _WITH_DIST_ */


	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::DelAllMailOfDir(int mdirid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=(mstatus|%d) WHERE mdirid=%d", MSG_ATTR_DELETED, mdirid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			mysql_free_result(query_result);
		}
		else
		{
			return -1;
		}

		return 0;
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::DelAllMailOfID(const char* username)
{
	char sqlcmd[1024];
    
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s'", username);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
            //start a transaction
            if(StartTransaction() != 0)
                return -1;
    
			while((row = mysql_fetch_row(query_result)))
			{
				DelAllMailOfDir(atoi(row[0]));
			}
            
            if(CommitTransaction() != 0)
                return -1;
    
			mysql_free_result(query_result);
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
		return 0;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;	
	}
}

int MailStorage::DelID(const char* username)
{
	if(strcasecmp(username ,"admin") == 0)
		return -1;
	
    int idType = -1;
    if(VerifyUser(username) == 0)
    {
        idType = 1; //user
    }
    else if(VerifyGroup(username) == 0)
    {
        idType = 2; //group
    }
    
	    
	if(idType > 0)
	{
		char sqlcmd[1024];
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);
		
        //start a transaction
        if(StartTransaction() != 0)
            return -1;
    
		if(idType == 2)
		{
			sprintf(sqlcmd, "DELETE FROM grouptbl WHERE groupname='%s'", strSafetyUsername.c_str());
			if(Query(sqlcmd, strlen(sqlcmd)) != 0)
			{
				show_error(&m_hMySQL, sqlcmd);
				return -1;
			}	
		}
		else if(idType == 1)
		{
			sprintf(sqlcmd, "DELETE FROM grouptbl WHERE membername='%s'", strSafetyUsername.c_str());
			if(Query(sqlcmd, strlen(sqlcmd)) != 0)
			{
				show_error(&m_hMySQL, sqlcmd);
				return -1;
			}
			
			sprintf(sqlcmd, "DELETE FROM dirtbl WHERE downer='%s'", strSafetyUsername.c_str());
			if(Query(sqlcmd, strlen(sqlcmd)) != 0)
			{
				
				show_error(&m_hMySQL, sqlcmd);
				return -1;
			}

			
			if(DelAllMailOfID(strSafetyUsername.c_str()) != 0)
			{
                RollbackTransaction();
                
				return -1;
			}
		
		}
        
		sprintf(sqlcmd, "DELETE FROM usertbl WHERE uname='%s'", strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) != 0)
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
        
        if(CommitTransaction() != 0)
            return -1;
        
		return 0;
	}
	else
		return -1;
}

int MailStorage::LoadMailFromFile(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid, int mdirid, unsigned int mstatus, const char* emlfilepath, int& mailid)
{
	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);
	
	char sqlfilepath[1024];
	sprintf(sqlfilepath, "/tmp/erisemail/%s_%08x_%08x_%016lx_%08x.sql", 
		muniqid, time(NULL), getpid(), pthread_self(), random());

	ofstream * sqlfile = new ofstream(sqlfilepath, ios_base::binary|ios::out|ios::trunc);
	chmod(sqlfilepath, 0666);
	if(sqlfile)
	{		
		if(sqlfile->is_open())
		{
			ifstream * emlfile = new ifstream(emlfilepath, ios_base::binary);
			if(emlfile)
			{
				if(emlfile->is_open())
				{
					char rbuf[MEMORY_BLOCK_SIZE + 1];
					int rlen = 0;
					
					//insert
					sprintf(rbuf, "\"%s\",\"%s\",%u,%u,\"%s\",%d,%u,\"", 
						strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus);	
					
					sqlfile->write(rbuf, strlen(rbuf));
	
					while(1)
					{
						if(emlfile->eof())
							break;
						
						if(emlfile->read(rbuf, MEMORY_BLOCK_SIZE) < 0)
							break;
						rlen = emlfile->gcount();
						rbuf[rlen] = '\0';
						
						string strtmp = rbuf;
						SqlSafetyString(strtmp);
						
						if(sqlfile->write(strtmp.c_str(), strtmp.length()) < 0)
							break;
					}
					sqlfile->write("\"", 1);
					
					emlfile->close();
				}
				delete emlfile;
			}
			
			sqlfile->close();
		}
		delete sqlfile;
	}
					
	char sqlcmd[1024];
	sprintf(sqlcmd, "LOAD DATA LOCAL INFILE '%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '\"' (mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody)",
        sqlfilepath,
        mtx == mtExtern ? "extmailtbl" : "mailtbl");
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		mailid = mysql_insert_id(&m_hMySQL);
		unlink(sqlfilepath);
		return 0;
	}
	else
	{
		unlink(sqlfilepath);
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::UpdateMailFromFile(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid, int mdirid, unsigned int mstatus, const char* emlfilepath, int mailid)
{
	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);
	
	char sqlfilepath[1024];
	sprintf(sqlfilepath, "/tmp/erisemail/%s_%08x_%08x_%016lx_%08x.sql", 
		muniqid, time(NULL), getpid(), pthread_self(), random());

	ofstream * sqlfile = new ofstream(sqlfilepath, ios_base::binary|ios::out|ios::trunc);
	chmod(sqlfilepath, 0666);
	if(sqlfile)
	{		
		if(sqlfile->is_open())
		{
			ifstream * emlfile = new ifstream(emlfilepath, ios_base::binary);
			if(emlfile)
			{
				if(emlfile->is_open())
				{
					char rbuf[MEMORY_BLOCK_SIZE + 1];
					int rlen = 0;
					
					//update
					sprintf(rbuf, "%d,\"%s\",\"%s\",%u,%u,\"%s\",%d,%u,\"",
						mailid, strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus);
					
					sqlfile->write(rbuf, strlen(rbuf));
	
					while(1)
					{
						if(emlfile->eof())
							break;
						
						if(emlfile->read(rbuf, MEMORY_BLOCK_SIZE) < 0)
							break;
						rlen = emlfile->gcount();
						rbuf[rlen] = '\0';
						
						string strtmp = rbuf;
						SqlSafetyString(strtmp);
						
						if(sqlfile->write(strtmp.c_str(), strtmp.length()) < 0)
							break;
					}
					sqlfile->write("\"", 1);
					
					emlfile->close();
				}
				delete emlfile;
			}
			
			sqlfile->close();
		}
		delete sqlfile;
	}
	
	char sqlcmd[1024];
	sprintf(sqlcmd, "LOAD DATA LOCAL INFILE '%s' REPLACE INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '\"' (mid,mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody)",
        sqlfilepath,
        mtx == mtExtern ? "extmailtbl" : "mailtbl");
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		unlink(sqlfilepath);
		return 0;
	}
	else
	{
		unlink(sqlfilepath);
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::InsertMail(const char* mfrom, const char* mto, unsigned int mtime, unsigned int mtx, const char* muniqid, int mdirid, unsigned int mstatus, const char* mbody, unsigned int  msize, int& mailid)
{
	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);

	string strSafetyBody = mbody;
	SqlSafetyString(strSafetyBody);
	
	char* sqlcmd = (char*)malloc(strSafetyBody.length() + strSafetyTo.length() + strSafetyFrom.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "INSERT INTO %s(mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody,msize) VALUES('%s','%s',%u,%u,'%s',%d,%u,'%s',%u)",
            mtx == mtExtern ? "extmailtbl" : "mailtbl",
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize);
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			mailid = mysql_insert_id(&m_hMySQL);
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::InsertMailIndex(const char* mfrom, const char* mto, const char* mhost, unsigned int mtime,unsigned int mtx,const char* muniqid, int mdirid, unsigned int mstatus, const char* mpath, unsigned int msize, int& mailid)
{
	if(mdirid != -1)
	{
		unsigned int commonMailNumber;
		unsigned int deletedMailNumber;
		unsigned int commonMailSize;
		unsigned int deletedMailSize;
		
		string username;
		Level_Info linfo;
		if(GetDirOwner(mdirid, username) == 0 
		&& GetUserStorage(username.c_str(), commonMailNumber, deletedMailNumber, commonMailSize, deletedMailSize) == 0
		&& GetUserLevel(username.c_str(), linfo) == 0)
		{
			if(linfo.boxmaxsize < (commonMailSize + msize))
				return -1;
		}
	}

	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);

	string strSafetyBody = mpath;
	SqlSafetyString(strSafetyBody);
    
	char* sqlcmd = (char*)malloc(strSafetyBody.length() + strSafetyTo.length() + strSafetyFrom.length() + 1024);
	if(sqlcmd)
	{
        
        if(strcmp(mhost, "") == 0 || strcasecmp(mhost, CMailBase::m_localhostname.c_str()) == 0)
        {
            sprintf(sqlcmd, "INSERT INTO %s.%s (mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody,msize) VALUES('%s','%s',%u,%u,'%s',%d,%u,'%s','%u')", m_database.c_str(),
                mtx == mtExtern ? "extmailtbl" : "mailtbl",
                strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize);
        }
        else
        {
            string strSafetyHost = mhost;
            SqlSafetyString(strSafetyHost);
    
            sprintf(sqlcmd, "INSERT INTO %s.%s (mfrom,mto,mhost,mtime,mtx,muniqid,mdirid,mstatus,mbody,msize) VALUES('%s','%s','%s',%u,%u,'%s',%d,%u,'%s','%u')", m_database.c_str(),
                mtx == mtExtern ? "extmailtbl" : "mailtbl",
                strSafetyFrom.c_str(), strSafetyTo.c_str(), strSafetyHost.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize);
        }
        
		if(Query(sqlcmd, strlen(sqlcmd) + 1) == 0)
		{
			mailid = mysql_insert_id(&m_hMySQL);
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::UpdateMail(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* mbody, unsigned int  msize, int mailid)
{
	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);

	string strSafetyBody = mbody;
	SqlSafetyString(strSafetyBody);
	
	char* sqlcmd = (char*)malloc(strSafetyBody.length() + strSafetyTo.length() + strSafetyFrom.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "UPDATE %s SET mfrom='%s',mto='%s',mtime=%u,mtx=%u,muniqid='%s',mdirid=%d,mstatus=%u,mbody='%s',msize='%u' WHERE mid=%d",
            mtx == mtExtern ? "extmailtbl" : "mailtbl",
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize, mailid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::UpdateMailIndex(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* mpath, unsigned int msize, int mailid)
{
	string strpath;
	GetMailIndex(mailid, strpath);

	string strTmp = m_private_path.c_str();
	strTmp += "/eml/";
	strTmp += strpath;
	if(unlink(strTmp.c_str()) < 0)
		return -1;
    
    string strCache = m_private_path.c_str();
	strCache += "/cache/";
    strCache += strpath;
	strCache += POSTFIX_CACHE;
	unlink(strCache.c_str());
	
	string strSafetyFrom = mfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = mto;
	SqlSafetyString(strSafetyTo);

	string strSafetyBody = mpath;
	SqlSafetyString(strSafetyBody);
	
	char* sqlcmd = (char*)malloc(strSafetyBody.length() + strSafetyTo.length() + strSafetyFrom.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "UPDATE %s SET mfrom='%s',mto='%s',mtime=%u,mtx=%u,muniqid='%s',mdirid=%d,mstatus=%u,mbody='%s', msize=%u WHERE mid=%d",
            mtx == mtExtern ? "extmailtbl" : "mailtbl",
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize, mailid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::ChangeMailDir(int mdirid, int mailid)
{
	char sqlcmd[1024];
	if(sqlcmd)
	{
		sprintf(sqlcmd, "UPDATE mailtbl SET mdirid=%d WHERE mid=%d", mdirid, mailid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::ListMailByDir(const char* username, vector<Mail_Info>& listtbl, const char* dirref)
{
	listtbl.clear();
	char sqlcmd[1024];
	int did;
	if(GetDirID(username, dirref, did) == -1)
	{
		return -1;
	}
	sprintf(sqlcmd, "SELECT mbody, muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mdirid=%d AND mstatus&%d<>%d ORDER BY mid", did , MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				Mail_Info mi;
				
				string tmpfile = m_private_path.c_str();
				tmpfile += "/eml/";
				tmpfile += row[0];
				
				int mfd = 0;
				mfd= open(tmpfile.c_str(), O_RDONLY);
				if(mfd > 0)
				{
			  		struct stat file_stat;
			  		fstat(mfd, &file_stat);
					mi.length = file_stat.st_size;

					close(mfd);
				}
				
				strcpy(mi.uniqid, row[1]);
				mi.mid = atoi(row[2]);
				mi.mtime = atoi(row[3]);
				mi.mstatus= atoi(row[4]);
				mi.mailfrom = row[5];
				mi.rcptto = row[6];
				mi.mtype = (MailType)atoi(row[7]);
				mi.mdid = atoi(row[8]);

				//just for pop delete cahce
				mi.reserve = 0;
				
				listtbl.push_back(mi);
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody, muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mdirid=%d AND mstatus&%d<>%d ORDER BY mtime desc", dirid , MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				Mail_Info mi;

				string tmpfile = m_private_path.c_str();
				tmpfile += "/eml/";
				tmpfile += row[0];
				
				int mfd = 0;
				mfd= open(tmpfile.c_str(), O_RDONLY);
				if(mfd > 0)
				{
			  		struct stat file_stat;
			  		fstat(mfd, &file_stat);
					mi.length = file_stat.st_size;

					close(mfd);
				}
				
				strcpy(mi.uniqid, row[1]);
				mi.mid = atoi(row[2]);
				mi.mtime = atoi(row[3]);
				mi.mstatus= atoi(row[4]);
				mi.mailfrom = row[5];
				mi.rcptto = row[6];
				mi.mtype = (MailType)atoi(row[7]);
				mi.mdid = atoi(row[8]);
				
				//just for pop delete cahce
				mi.reserve = 0;
				listtbl.push_back(mi);
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::LimitListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid, int beg, int rows)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mdirid=%d AND mstatus&%d<>%d ORDER BY mtime desc limit %d, %d", dirid , MSG_ATTR_DELETED, MSG_ATTR_DELETED, beg, rows);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
            unsigned int num_fields;
            unsigned int i;
            num_fields = mysql_num_fields(query_result);
			while((row = mysql_fetch_row(query_result)))
			{                
                Mail_Info mi;
				
				string tmpfile = m_private_path.c_str();
				tmpfile += "/eml/";
				tmpfile += row[0];
				
				int mfd = 0;
				mfd= open(tmpfile.c_str(), O_RDONLY);
				if(mfd > 0)
				{
			  		struct stat file_stat;
			  		fstat(mfd, &file_stat);
					mi.length = file_stat.st_size;

					close(mfd);
				}
				strcpy(mi.uniqid, row[1]);
				mi.mid = atoi(row[2]);
				mi.mtime = atoi(row[3]);
				mi.mstatus= atoi(row[4] == NULL ? 0 : row[4]);
				mi.mailfrom = row[5];
				mi.rcptto = row[6];
				mi.mtype = (MailType)atoi(row[7]);
				mi.mdid = atoi(row[8]);
				
				//just for pop delete cahce
				mi.reserve = 0;
				listtbl.push_back(mi);
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::LimitListUnauditedExternMailByDir(const char* username, vector<Mail_Info>& listtbl, int beg, int rows)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM extmailtbl WHERE mstatus&%d<>%d AND mstatus&%d=%d ORDER BY mtime limit %d, %d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED, beg, rows);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				Mail_Info mi;
				
				string tmpfile = m_private_path.c_str();
				tmpfile += "/eml/";
				tmpfile += row[0];
				
				int mfd = 0;
				mfd= open(tmpfile.c_str(), O_RDONLY);
				if(mfd > 0)
				{
			  		struct stat file_stat;
			  		fstat(mfd, &file_stat);
					mi.length = file_stat.st_size;

					close(mfd);
				}
				
				strcpy(mi.uniqid, row[1]);
				mi.mid = atoi(row[2]);
				mi.mtime = atoi(row[3]);
				mi.mstatus= atoi(row[4]);
				mi.mailfrom = row[5];
				mi.rcptto = row[6];
				mi.mtype = (MailType)atoi(row[7]);
				mi.mdid = atoi(row[8]);
				
				//just for pop delete cahce
				mi.reserve = 0;
				listtbl.push_back(mi);
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListAllMail(vector<Mail_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mstatus&%d<>%d ORDER BY mtime desc", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				Mail_Info mi;

				string tmpfile = m_private_path.c_str();
				tmpfile += "/eml/";
				tmpfile += row[0];
				
				int mfd = 0;
				mfd= open(tmpfile.c_str(), O_RDONLY);
				if(mfd > 0)
				{
			  		struct stat file_stat;
			  		fstat(mfd, &file_stat);
					mi.length = file_stat.st_size;

					close(mfd);
				}
				
				strcpy(mi.uniqid, row[1]);
				mi.mid = atoi(row[2]);
				mi.mtime = atoi(row[3]);
				mi.mstatus= atoi(row[4]);
				mi.mailfrom = row[5];
				mi.rcptto = row[6];
				mi.mtype = (MailType)atoi(row[7]);
				mi.mdid = atoi(row[8]);
				
				//just for pop delete cahce
				mi.reserve = 0;
				listtbl.push_back(mi);
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListAvailableExternMail(vector<Mail_Info>& listtbl, unsigned int mta_index, unsigned int mta_count,unsigned int max_num)
{
	listtbl.clear();
	char sqlcmd[1024];
#ifdef _WITH_DIST_  
	sprintf(sqlcmd, "SELECT mfrom, mto, mhost, muniqid, mid FROM extmailtbl WHERE mtx='%d' AND mstatus&%d<>%d AND mstatus&%d<>%d AND mid%%%d=%d and TIMESTAMPDIFF(SECOND, next_fwd_time, CURRENT_TIMESTAMP) > 0 and tried_count<%d ORDER BY mid",
        mtExtern, MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED, mta_count, mta_index, MAX_TRY_FORWARD_COUNT);
#else
    sprintf(sqlcmd, "SELECT mfrom, mto, muniqid, mid FROM extmailtbl WHERE mtx='%d' AND mstatus&%d<>%d AND mstatus&%d<>%d AND mid%%%d=%d and TIMESTAMPDIFF(SECOND, next_fwd_time, CURRENT_TIMESTAMP) > 0 and tried_count<%d ORDER BY mid",
        mtExtern, MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED, mta_count, mta_index, MAX_TRY_FORWARD_COUNT);
#endif /* _WITH_DIST_ */
    max_num = (max_num == 0 ? 0x7FFFFFFFFU : max_num);
	
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			
			while((row = mysql_fetch_row(query_result)))
			{
                if(max_num == 0)
                    break;
				Mail_Info mi;
#ifdef _WITH_DIST_
				mi.mailfrom = row[0];	
				mi.rcptto = row[1];
				mi.host = row[2] ? row[2] : "";
				strcpy(mi.uniqid, row[3]);
				mi.mid = atoi(row[4]);
#else
				mi.mailfrom = row[0];
				mi.rcptto = row[1];
                mi.host = "";
				strcpy(mi.uniqid, row[2]);
				mi.mid = atoi(row[3]);
#endif /* _WITH_DIST_ */
				listtbl.push_back(mi);
                max_num--;
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
        {
			show_error(&m_hMySQL, sqlcmd);
            return -1;
        }
	}
	else
	{
        show_error(&m_hMySQL, sqlcmd);
	    return -1;
	}
}

int MailStorage::ForwardingExternMail(int mid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE extmailtbl SET next_fwd_time=TIMESTAMPADD(SECOND, (tried_count+1)*300, CURRENT_TIMESTAMP), tried_count=tried_count+1 WHERE mid=%d", mid);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::GetExternMailForwardedCount(int mid)
{
    int r = -1;
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT tried_count FROM extmailtbl WHERE mid=%d", mid);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
        MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
            row = mysql_fetch_row(query_result);
            if(row)
            {
                r = atoi(row[0]);
            }
            mysql_free_result(query_result);
			return r;
		}
        
		return r;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return r;
    }
}

int MailStorage::ListMemberOfGroup(const char* group, vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT membername FROM grouptbl WHERE groupname='%s'", group);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				User_Info uinfo;
				if(GetID(row[0],uinfo) == 0)
				{
					listtbl.push_back(uinfo);
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
				
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListID(vector<User_Info>& listtbl, string orderby, BOOL desc)
{
	listtbl.clear();
	char sqlcmd[1024];
#ifdef _WITH_DIST_
    sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus, ulevel, uhost FROM usertbl ORDER BY %s %s", orderby == "" ? "utime" : orderby.c_str(), desc ? "desc" : "");
#else
	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus, ulevel FROM usertbl ORDER BY %s %s", orderby == "" ? "utime" : orderby.c_str(), desc ? "desc" : "");
#endif /* _WITH_DIST_ */

	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				User_Info ui;
				strcpy(ui.username, row[0]);
				strcpy(ui.alias, row[1]);
				ui.type = UserType(atoi(row[2]));
				ui.role = UserRole(atoi(row[3]));
				ui.size = atoi(row[4]);
				ui.status = (UserStatus)atoi(row[5]);
				ui.level = atoi(row[6]);
#ifdef _WITH_DIST_
                strcpy(ui.host, row[7] ? row[7] : "");
#else
                strcpy(ui.host, CMailBase::m_localhostname.c_str());
#endif /* _WITH_DIST_ */
				listtbl.push_back(ui);
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetID(const char* uname, User_Info& uinfo)
{
	char sqlcmd[1024];
	if(uname == NULL)
		return -1;
	else
	{
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);
		sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus, ulevel FROM usertbl WHERE uname='%s'", strSafetyUsername.c_str());
	}
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			
			if(row)
			{
				strcpy(uinfo.username, row[0]);
				strcpy(uinfo.alias, row[1]);
				uinfo.type = UserType(atoi(row[2]));
				uinfo.role = UserRole(atoi(row[3]));
				uinfo.size = atoi(row[4]);
				uinfo.status = (UserStatus)atoi(row[5]);
				uinfo.level = atoi(row[6]);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::ListGroup(vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus FROM usertbl WHERE utype=%d ORDER BY uname", utGroup);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				User_Info ui;
				strcpy(ui.username, row[0]);
				strcpy(ui.alias, row[1]);
				ui.type = UserType(atoi(row[2]));
				ui.role = UserRole(atoi(row[3]));
				ui.size = atoi(row[4]);
				ui.status = (UserStatus)atoi(row[5]);
				listtbl.push_back(ui);
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::ListMember(vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus FROM usertbl WHERE utype=%d ORDER BY uname", utMember);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				User_Info ui;
				strcpy(ui.username, row[0]);
				strcpy(ui.alias, row[1]);
				ui.type = UserType(atoi(row[2]));
				ui.role = UserRole(atoi(row[3]));
				ui.size = atoi(row[4]);
				ui.status = (UserStatus)atoi(row[5]);
				listtbl.push_back(ui);
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::Passwd(const char* uname, const char* password)
{
	if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);

		string strSafetyPassword = password;
		SqlSafetyString(strSafetyPassword);
		
		sprintf(sqlcmd, "UPDATE usertbl SET upasswd=ENCODE('%s','%s') WHERE uname='%s'", strSafetyPassword.c_str(), CODE_KEY, strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
#ifdef _WITH_LDAP_
			if(!m_is_ldap_binded)
			{
				int rc = ldap_simple_bind(m_ldap, CMailBase::m_ldap_manager.c_str(), CMailBase::m_ldap_manager_passwd.c_str());
				
				if(rc == -1)
				{
					fprintf(stderr, "ldap_simple_bind: %s\n", ldap_err2string(rc));
					return -1;
				}
			}
			
			m_is_ldap_binded = TRUE;
			
			int msgid = -1;
			
			char szDN[1025];
			snprintf(szDN, 1024, CMailBase::m_ldap_user_dn.c_str(), uname);
			szDN[1024] = '\0';
			
			char* passwd_attr = (char*) malloc(CMailBase::m_ldap_search_attribute_user_password.length() + 1);
            strcpy(passwd_attr, CMailBase::m_ldap_search_attribute_user_password.c_str());
            
            char* passwd_val = (char*) malloc(strlen(password) + 1);
            strcpy(passwd_val, password);
            
            LDAPMod mod;
            mod.mod_op = LDAP_MOD_REPLACE;
            mod.mod_type = passwd_attr;
            
            char* vals[2];
            vals[0] = passwd_val;
            vals[1] = NULL;
            mod.mod_vals.modv_strvals = vals;
             
            LDAPMod* mods[2];
            mods[0] = &mod;
            mods[1] = NULL;
            
			int rc = ldap_modify_ext(m_ldap, szDN, mods, NULL, NULL, &msgid);
			
            free(passwd_attr);
            free(passwd_val);
            
			if( rc != LDAP_SUCCESS )
			{
				fprintf(stderr, "ldap_search_ext_s: %s\n", ldap_err2string(rc));
				return( rc );
			}
			
#endif /* _WITH_LDAP_ */

			return 0;
		}
		else
		{
            show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::Alias(const char* uname, const char* alias)
{
	if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);

		string strSafetyAlias = alias;
		SqlSafetyString(strSafetyAlias);
		
		sprintf(sqlcmd, "UPDATE usertbl SET ualias='%s' WHERE uname='%s'", strSafetyAlias.c_str(), strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
            show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::Host(const char* uname, const char* host)
{
#ifdef _WITH_DIST_ 
    if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);

		string strSafetyHost = host;
		SqlSafetyString(strSafetyHost);
		
		sprintf(sqlcmd, "UPDATE usertbl SET uhost='%s' WHERE uname='%s'", strSafetyHost.c_str(), strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
            show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
#else
    return 0;
#endif /* _WITH_DIST_ */
}

int MailStorage::SetUserStatus(const char* uname, UserStatus status)
{
	if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);

		sprintf(sqlcmd, "UPDATE usertbl SET ustatus=%d WHERE uname='%s'", status, strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
            show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::SetUserSize(const char* uname, unsigned int size)
{
	if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);
		
		sprintf(sqlcmd, "UPDATE usertbl SET usize=%d WHERE uname='%s'", size, strSafetyUsername.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
            show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::GetUserSize(const char* uname, unsigned long long& size)
{
	if(VerifyUser(uname) == 0)
	{
		Level_Info linfo;
		
		if(GetUserLevel(uname, linfo) == 0)
		{
			size = linfo.mailmaxsize;
			return 0;
		}
		else
			return -1;
	}
	else
		return -1;
}

int MailStorage::DumpMailToFile(int mid, string& dumpfile)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "/tmp/erisemail/%08x_%08x_%08x_%016lx_%08x.dat",  time(NULL), getpid(), pthread_self(), random(), mid);
	dumpfile = sqlcmd;
	sprintf(sqlcmd, "SELECT mbody into DUMPFILE '%s' FROM mailtbl WHERE mid='%d' AND mstatus&%d<>%d", dumpfile.c_str(), mid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::GetMailBody(int mid, char* body)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody FROM mailtbl WHERE mid='%d' AND mstatus&%d<>%d", mid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				strcpy(body, row[0]);
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetMailIndex(int mid, string& path, unsigned int mtx)
{	
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody FROM %s WHERE mid='%d' AND mstatus&%d<>%d;",
        mtx == mtExtern ? "extmailtbl" : "mailtbl",
        mid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{	
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				path = row[0];
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);	
		return -1;
	}
}

int MailStorage::GetMailDir(int mid, int & dirid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM mailtbl WHERE mid='%d'", mid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetMailOwner(int mid, string & owner)
{
	char sqlcmd[1024];
	int dirid;
	
	if(GetMailDir(mid, dirid) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		sprintf(sqlcmd, "SELECT downer FROM dirtbl WHERE did='%d'", dirid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{					
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				row = mysql_fetch_row(query_result);
				if(row)
				{
					owner = row[0];
					mysql_free_result(query_result);
					return 0;
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::GetMailFromAndTo(int mid, string & from, string &to)
{
	char sqlcmd[1024];
	
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT mfrom, mto FROM mailtbl WHERE mid='%d'", mid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				from = row[0];
				to = row[0];
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::IsAdmin(const char* username)
{
	char sqlcmd[1024];
	
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND urole=%d", username, urAdministrator);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDirOwner(int dirid, string & owner)
{
	char sqlcmd[1024];
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT downer FROM dirtbl WHERE did='%d'", dirid);

    
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				owner = row[0];
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetMailLen(int mid, int& mlen)
{

	string strpath;
	GetMailIndex(mid, strpath);

	string strTmp = m_private_path.c_str();
	strTmp += "/eml/";
	strTmp += strpath;

	int mfd = 0;
	mfd= open(strTmp.c_str(), O_RDONLY);
	if(mfd > 0)
	{
  		struct stat file_stat;
  		fstat(mfd, &file_stat);
		mlen = file_stat.st_size;
		close(mfd);
	}
}

int MailStorage::ShiftDelMail(int mid, unsigned int mtx)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "DELETE FROM %s WHERE mid='%d'",
        mtx == mtExtern ? "extmailtbl" : "mailtbl",
        mid);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		char mailpath[512];
		GetMailBody(mid, mailpath);
		
		string strTmp = m_private_path.c_str();
		strTmp += "/eml/";
		strTmp += mailpath;
		unlink(strTmp.c_str());
        
        string strCache = m_private_path.c_str();
		strCache += "/cache/";
		strCache += mailpath;        
		strCache += POSTFIX_CACHE;
		unlink(strCache.c_str());
	
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::DelMail(const char* username, int mid, unsigned int mtx)
{
	char sqlcmd[1024];
	
	string owner = "";
	if(IsAdmin(username) == 0 || (GetMailOwner(mid, owner) == 0 && strcasecmp(owner.c_str(), username) == 0))
	{
		sprintf(sqlcmd, "UPDATE %s SET mstatus=(mstatus|%d) WHERE mid='%d'",
            mtx == mtExtern ? "extmailtbl" : "mailtbl",
            MSG_ATTR_DELETED, mid);
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::AppendUserToGroup(const char* username, const char* groupname)
{
	if((VerifyUser(username) == 0)&&(VerifyGroup(groupname) == 0))
	{
		char sqlcmd[1024];
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);

		string strSafetyGroupname = groupname;
		SqlSafetyString(strSafetyGroupname);
		
		sprintf(sqlcmd, "SELECT gid FROM grouptbl WHERE groupname='%s' AND membername='%s'", strSafetyGroupname.c_str(), strSafetyUsername.c_str());
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				int count = mysql_num_rows(query_result);
				mysql_free_result(query_result);
				if(count > 0)
					return 0;
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
		
		sprintf(sqlcmd, "INSERT INTO grouptbl(groupname, membername, gtime) VALUES('%s', '%s', %d)",
		strSafetyGroupname.c_str(), strSafetyUsername.c_str(), time(NULL));
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
        {
            show_error(&m_hMySQL, sqlcmd);
			return -1;
        }
	}
	else
		return -1;
}

int MailStorage::RemoveUserFromGroup(const char* username, const char* groupname)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);

	string strSafetyGroupname = groupname;
	SqlSafetyString(strSafetyGroupname);
		
	sprintf(sqlcmd, "DELETE FROM grouptbl WHERE groupname='%s'and membername='%s'", strSafetyGroupname.c_str(), strSafetyUsername.c_str());
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::CreateDir(const char* username, const char* dirref)
{	
	char sqlcmd[1024];

	//exist?
	if(IsDirExist(username, dirref) == 0)
		return -1;

	string strDirRef = dirref;
	vector<string> vDirRef;
	vSplitString(strDirRef, vDirRef, "/", TRUE, 0x7FFFFFFFU);

	int vDirRefLen = vDirRef.size();
	if(vDirRefLen == 0)
		return -1;
	
	string strNewDir = vDirRef[vDirRefLen - 1];

	SqlSafetyString(strNewDir);
	
	int parentID;
	if(GetDirParentID(username, dirref, parentID) == 0)
	{
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);
		sprintf(sqlcmd, "INSERT INTO dirtbl(dname,downer,dparent,dstatus,dtime) VALUES('%s','%s',%d,%d,%d)",
			strNewDir.c_str(), strSafetyUsername.c_str(), parentID, DIR_ATTR_SUBSCRIBED|DIR_ATTR_MARKED|DIR_ATTR_NOSELECT, time(NULL));
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
        {
            show_error(&m_hMySQL, sqlcmd);
			return -1;
        }
	}
	else
	{
		return -1;
	}
}

int MailStorage::CreateDir(const char* username, const char* dirname, int parentid)
{	
	char sqlcmd[1024];

	if(parentid != -1)
	{
		//parent is exist?
		if(IsDirExist(username, parentid) != 0)
			return -1;
	}

	//dir is exist
	if(IsSubDirExist(username, parentid, dirname) == 0)
		return -1;
	
	string strNewDir = dirname;
	SqlSafetyString(strNewDir);
	
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	
	sprintf(sqlcmd, "INSERT INTO dirtbl(dname,downer,dparent,dstatus,dtime) VALUES('%s','%s',%d,%d,%d)",
		strNewDir.c_str(), strSafetyUsername.c_str(), parentid, DIR_ATTR_SUBSCRIBED|DIR_ATTR_MARKED|DIR_ATTR_NOSELECT, time(NULL));
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
}

int MailStorage::DeleteDir(const char* username, int dirid)
{
	char sqlcmd[1024];
    
    //start a transaction
    if(StartTransaction() != 0)
        return -1;
    
	//delete mail of ownself
	sprintf(sqlcmd, "DELETE FROM mailtbl WHERE mdirid=%d", dirid);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
		return -1;

	//delete self
	sprintf(sqlcmd, "DELETE FROM dirtbl WHERE did=%d", dirid);
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
		return -1;

	//delete subdir of ownself
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while(row = mysql_fetch_row(query_result))
			{
				DeleteDir(username, atoi(row[0]));
			}
			mysql_free_result(query_result);
            
            if(CommitTransaction() != 0)
                return -1;
            
			return 0;
		}
		else
        {
            RollbackTransaction();
            
			return -1;
        }
	}
	else
	{
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::DeleteDir(const char* username, const char* dirref)
{	
	int DirID;
	if(GetDirID(username, dirref, DirID) == -1)
		return -1;
	return DeleteDir(username, DirID);
}

int MailStorage::GetDirParentID(const char* username, const char* dirref, int& parentid)
{
	char sqlcmd[1024];

	string parentdir, dirname;
	if(SplitDir(dirref, parentdir, dirname) == -1)
	{
		return -1;
	}
	if(parentdir == "")
	{
		parentid = -1;
		return 0;
	}
	else
	{
		return GetDirID(username, parentdir.c_str(), parentid);
	}
}

int MailStorage::GetDirParentID(const char* username, const char* dirref, vector<int>& vparentid)
{
	char sqlcmd[1024];

	string parentdir, dirname;
	if(SplitDir(dirref, parentdir, dirname) == -1)
	{
		return -1;
	}
	if(parentdir == "")
	{
		vparentid.push_back(-1);
		return 0;
	}
	else
	{
		//printf("#: %s %s\n", parentdir.c_str(), dirname.c_str());
		return GetDirID(username, parentdir.c_str(), vparentid);
	}
}


int MailStorage::GetDirParentID(const char* username, int dirid, int& parentid)
{
	string owner;
	GetDirOwner(dirid, owner);
	if(strcasecmp(username, owner.c_str()) != 0)
		return -1;

	char sqlcmd[1024];
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT dparent FROM dirtbl WHERE did='%d'", dirid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				parentid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::IsDirExist(const char* username, const char* dirref)
{
	char sqlcmd[1024];
	string strDirRef = dirref;
	vector<string> vDirRef;
	vSplitString(strDirRef, vDirRef, "/", TRUE, 0x7FFFFFFFU);
	
	int vDirRefLen = vDirRef.size();

	if(vDirRefLen == 0)
		return -1;
	
	int parentid = -1;
	for(int x = 0; x < vDirRefLen; x++)
	{
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);

		string strSafetyDirname = vDirRef[x];
		SqlSafetyString(strSafetyDirname);
		Replace(strSafetyDirname, "%", "_%");
		sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
            strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				//can not find the current dir via its name
				row = mysql_fetch_row(query_result);
				if(row == NULL)
				{
					mysql_free_result(query_result);
					return -1;
				}
				else
				{
					parentid = atoi(row[0]);
					mysql_free_result(query_result);
				}
			}
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	return 0;
}

int MailStorage::IsDirExist(const char* username, int dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);

	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND did=%d", 
		strSafetyUsername.c_str(), dirid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			//can not find the current dir via its name
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				return -1;
			}
			else
			{
				mysql_free_result(query_result);
				return 0;
			}
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::IsSubDirExist(const char* username, int parentid, const char* dirname)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);

	string strSafetyDirname = dirname;
	SqlSafetyString(strSafetyDirname);
	Replace(strSafetyDirname, "%", "_%");

	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
        strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			//can not find the current dir via its name
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				return -1;
			}
			else
			{
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::GetDirID(const char* username, const char* dirref, vector<int>& vdirid)
{
	char sqlcmd[1024];
	string strDirRef = dirref;
	vector<string> vDirRef;
	vSplitString(strDirRef, vDirRef, "/", TRUE, 0x7FFFFFFFU);
	
	int vDirRefLen = vDirRef.size();
	if(vDirRefLen == 0)
	{
		return -1;
	}
	
	vector<int> vparentid;
	vector<int> vchildid;
	vchildid.push_back(-1);
	for(int x = 0; x < vDirRefLen; x++)
	{
		vparentid.clear();
		vparentid.assign(vchildid.begin(), vchildid.end());
		vchildid.clear();
		for(int i = 0; i < vparentid.size(); i++)
		{
		    string strSafetyUsername = username;
		    SqlSafetyString(strSafetyUsername);

		    string strSafetyDirname = vDirRef[x];
		    SqlSafetyString(strSafetyDirname);
		    if(strSafetyDirname.find("%") == string::npos)
			    sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
				    strSafetyUsername.c_str(), strSafetyDirname.c_str(), vparentid[i]);
		    else
			    sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname like '%s' AND dparent=%d", 
				    strSafetyUsername.c_str(), strSafetyDirname.c_str(), vparentid[i]);
		    
		    if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		    {
			    MYSQL_RES *query_result;
			    MYSQL_ROW row;
			    query_result = mysql_store_result(&m_hMySQL);
			    
			    if(query_result)
			    {
				    //can not find the current dir via its name
				    while((row = mysql_fetch_row(query_result)))
				    {
					    vchildid.push_back(atoi(row[0]));
				    }
				    mysql_free_result(query_result);
			    }
			    else
			    {
				    show_error(&m_hMySQL, sqlcmd);
				    return -1;
			    }
		    }
		    else
		    {
		        
			    show_error(&m_hMySQL, sqlcmd);
			    return -1;
		    }
		}
	}
	vdirid.assign(vchildid.begin(), vchildid.end());
	return 0;
}

int MailStorage::GetDirID(const char* username, const char* dirref, int& dirid)
{
	char sqlcmd[1024];
	string strDirRef = dirref;
	vector<string> vDirRef;
	vSplitString(strDirRef, vDirRef, "/", TRUE, 0x7FFFFFFFU);
	
	int vDirRefLen = vDirRef.size();
	if(vDirRefLen == 0)
	{
		return -1;
	}
	
	int parentid = -1;
	for(int x = 0; x < vDirRefLen; x++)
	{
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);

		string strSafetyDirname = vDirRef[x];
		SqlSafetyString(strSafetyDirname);
		Replace(strSafetyDirname, "%", "_%");

		sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
            strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				//can not find the current dir via its name
				row = mysql_fetch_row(query_result);
				if(row == NULL)
				{
					mysql_free_result(query_result);
					show_error(&m_hMySQL, sqlcmd);
					return -1;
				}
				else
				{
					parentid = atoi(row[0]);
					mysql_free_result(query_result);
				}
			}
			else
			{
				show_error(&m_hMySQL, sqlcmd);
				return -1;
			}
		}
		else
		{
		    
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	dirid = parentid;
	return 0;
}

int MailStorage::GetInboxID(const char* username, int &dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d",
			strSafetyUsername.c_str(), duInbox);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Inbox','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duInbox, time(NULL));
				if(Query(sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duInbox, strSafetyUsername.c_str(), "Inbox");
					if(Query(sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetJunkID(const char* username, int &dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d",
			strSafetyUsername.c_str(), duJunk);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);

				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Junk','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duJunk, time(NULL));
				if(Query(sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duJunk, strSafetyUsername.c_str(), "Junk");
					if(Query(sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDraftsID(const char* username, int &dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d",
			strSafetyUsername.c_str(), duDrafts);
	//printf("%s\n",sqlcmd );
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Drafts','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duDrafts, time(NULL));
				if(Query(sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duDrafts, strSafetyUsername.c_str(), "Drafts");
					if(Query(sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetSentID(const char* username, int &dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d",
			strSafetyUsername.c_str(), duSent);
	//printf("%s\n",sqlcmd );
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Sent','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duSent, time(NULL));
				if(Query(sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duSent, strSafetyUsername.c_str(), "Sent");
					if(Query(sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				
					return 0;
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetTrashID(const char* username, int &dirid)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dusage=%d",
			strSafetyUsername.c_str(), duTrash);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Trash','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duTrash, time(NULL));
				if(Query(sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duTrash, strSafetyUsername.c_str(), "Trash");
					if(Query(sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}


int MailStorage::GetDirMailCount(const char* username, int dirid, unsigned int& count)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM mailtbl WHERE mdirid =%d AND mstatus&%d<>%d", dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			count = mysql_num_rows(query_result);
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetUnauditedExternMailCount(const char* username, unsigned int& count)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM extmailtbl WHERE mstatus&%d<>%d AND mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			count = mysql_num_rows(query_result);
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDirStatus(const char* username, const char* dirref, unsigned int& status)
{
	int dirID;
	if(GetDirID(username, dirref, dirID) == -1)
		return -1;
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT dstatus FROM dirtbl WHERE did=%d", dirID);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				return -1;
			}
			else
			{
				status = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::SetDirStatus(const char* username, const char* dirref,unsigned int status)
{
	int dirID;
	if(GetDirID(username, dirref, dirID) == -1)
		return -1;
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE dirtbl SET dstatus=%d WHERE did=%d", status, dirID);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::GetMailStatus(int mid, unsigned int& status)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mstatus FROM mailtbl WHERE mid=%d", mid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				return -1;
			}
			else
			{
				status = atoi(row[0]);
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetMailUID(int mid, string uid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT muniqid FROM mailtbl WHERE mid=%d", mid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row == NULL)
			{
				mysql_free_result(query_result);
				return -1;
			}
			else
			{
				uid = row[0];
				mysql_free_result(query_result);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::SetMailStatus(const char* username, int mid, unsigned int status)
{
	char sqlcmd[1024];
	string owner = "";
	if(IsAdmin(username) == 0 || (GetMailOwner(mid, owner) == 0 && strcasecmp(owner.c_str(), username) == 0))
	{
		sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=%d WHERE mid=%d", status, mid);
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
			return -1;
	}
	else
	{
		return -1;
	}
}

int MailStorage::RenameDir(const char* username, const char* oldname, const char* newname)
{
	char sqlcmd[1024];
	int dirID;
	if(IsDirExist(username, newname) == 0)
	{
		return -1;
	}
	if(GetDirID(username, oldname, dirID) == -1)
	{
		return -1;
	}
	
	string strNewDirRef = newname;
	vector<string> vNewDirRef;
	vSplitString(strNewDirRef, vNewDirRef, "/", TRUE, 0x7FFFFFFFU);
	int vNewDirRefLen = vNewDirRef.size();
	if(vNewDirRefLen == 0)
		return -1;
	int newParentID;
	if(GetDirParentID(username, newname, newParentID) == -1)
	{
		return -1;
	}
	
	string strNewDir = vNewDirRef[vNewDirRefLen - 1];

	sprintf(sqlcmd, "UPDATE dirtbl SET dname='%s',dparent=%d WHERE did=%d", strNewDir.c_str(), newParentID, dirID);

	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
    {
        show_error(&m_hMySQL, sqlcmd);
		return -1;
    }
	
}

int MailStorage::TraversalListDir(const char* username, const char* dirref, vector<Dir_Tree_Ex>& listtbl)
{
	char sqlcmd[1024];
	string strDirRef = dirref;
	vector<int> dirParentID;
    
	Replace(strDirRef, "*", "%");
	
	if(GetDirParentID(username, dirref, dirParentID) == -1)
		return -1;
    
	for(int x = 0; x < dirParentID.size(); x++)
	{
		string parentdir, dirname;
		SplitDir(strDirRef.c_str(), parentdir, dirname);

		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);
		sprintf(sqlcmd, "SELECT dname, dstatus, did FROM dirtbl WHERE downer='%s' AND dparent=%d AND dname like '%s' ORDER BY dtime",
		strSafetyUsername.c_str(), dirParentID[x], dirname.c_str());
		string nextdir = "";
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				while(row = mysql_fetch_row(query_result))
				{
					nextdir = parentdir;
					if(nextdir == "")
					{
						nextdir += row[0];
					}
					else
					{
						nextdir += "/";
						nextdir += row[0];
					}
					
					Dir_Tree_Ex di;
					di.did = atoi(row[2]);
	
					strcpy(di.name, row[0]);
					di.path = nextdir.c_str();
					di.status = atoi(row[1]);
					di.parentid = dirParentID[x];
					strcpy(di.owner, username);
					listtbl.push_back(di);
					nextdir +="/*";
					TraversalListDir(username, nextdir.c_str(), listtbl);
				}
				mysql_free_result(query_result);
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	return 0;
}

int MailStorage::ListSubDir(const char* username, int pid, vector<Dir_Info>& listtbl)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	
	sprintf(sqlcmd, "SELECT dname, dstatus, did FROM dirtbl WHERE downer='%s' AND dparent=%d ORDER BY did", strSafetyUsername.c_str(), pid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while(row = mysql_fetch_row(query_result))
			{
				Dir_Info di;
				di.did = atoi(row[2]);
				strcpy(di.name, row[0]);
				di.status = atoi(row[1]);
				di.parentid = pid;
				strcpy(di.owner, username);
				di.childrennum = 0;
				
				sprintf(sqlcmd, "SELECT count(*) FROM dirtbl WHERE dparent=%d", di.did);
				
				if(Query(sqlcmd, strlen(sqlcmd)) == 0)
				{
					MYSQL_RES *qResult2;
					MYSQL_ROW row2;
					qResult2 = mysql_store_result(&m_hMySQL);
					
					if(qResult2)
					{
						row2 = mysql_fetch_row(qResult2);
						di.childrennum = atoi(row2[0]);
						mysql_free_result(qResult2);
					}
				}
				else
				{
				    
				}
				listtbl.push_back(di);
			}
			mysql_free_result(query_result);
			return 0;
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::SplitDir(const char* dirref, string& parentdir, string& dirname)
{
	string strDirRef = dirref;
	vector<string> vDirRef;
	vSplitString(strDirRef, vDirRef, "/", TRUE, 0x7FFFFFFFU);
	
	int vDirRefLen = vDirRef.size();
	if(vDirRefLen == 0)
		return -1;
	parentdir = "";
	for(int i = 0; i < vDirRefLen - 1; i++ )
	{
		if(parentdir == "")
			parentdir += vDirRef[i];
		else
		{
			parentdir += "/";
			parentdir += vDirRef[i];
		}
	}
	dirname = vDirRef[vDirRefLen - 1];
	return 0;
}

int MailStorage::GetUnseenMail(int dirid, int& num)
{
	char sqlcmd[1024];
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT COUNT(*) FROM mailtbl WHERE mdirid='%d' AND mstatus&%d<>%d AND mstatus&%d<>%d", dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_SEEN, MSG_ATTR_SEEN);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				
				num += atoi(row[0]);
				mysql_free_result(query_result);

				sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
				
				if(Query(sqlcmd, strlen(sqlcmd)) == 0)
				{					
					query_result = mysql_store_result(&m_hMySQL);
					
					if(query_result)
					{
						while((row = mysql_fetch_row(query_result)))
						{
							int pid = atoi(row[0]);
							GetUnseenMail(pid, pid);
						}
						mysql_free_result(query_result);
						return 0;
					}
					else
					{
						return -1;
					}
				}
				else
				{
				    
					return -1;
				}
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::EmptyDir(const char* username, int dirid)
{
	string owner;
	if(GetDirOwner(dirid, owner) < 0)
		return -1;
	if(strcasecmp(username, owner.c_str()) != 0)
		return -1;
		
	char sqlcmd[1024];
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=(mstatus|%d) WHERE mdirid='%d' AND mstatus&%d<>%d", MSG_ATTR_DELETED, dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
    
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			mysql_free_result(query_result);

			sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
			
			if(Query(sqlcmd, strlen(sqlcmd)) == 0)
			{					
				query_result = mysql_store_result(&m_hMySQL);
				
				if(query_result)
				{
					while((row = mysql_fetch_row(query_result)))
					{
						int pid = atoi(row[0]);
						EmptyDir(username, pid);
					}
					mysql_free_result(query_result);
					return 0;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDirName(const char * username, int dirid, string& dirname)
{
	string owner;
	GetDirOwner(dirid, owner);
	if(strcasecmp(username, owner.c_str()) != 0)
		return -1;

	char sqlcmd[1024];
	MYSQL_RES *query_result;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT dname FROM dirtbl WHERE did='%d'", dirid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{					
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				dirname = row[0];
				mysql_free_result(query_result);
				return 0;
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetDirPath(const char * username, int dirid, string& globalpath)
{
	string owner;
	GetDirOwner(dirid, owner);
	if(strcasecmp(username, owner.c_str()) != 0)
		return -1;
		
	string dirname;
	if(GetDirName(username, dirid, dirname) == -1)
	{
		return -1;
	}
	globalpath += dirname;
	globalpath += "/";
	int parentid;
	if(GetDirParentID(username, dirid, parentid) == 0)
	{
		if(parentid == -1)
		{
			return 0;
		}
		else
		{
			GetDirPath(username, dirid, globalpath);
		}
	}
	else
		return -1;
}

int MailStorage::GetGlobalStorage(unsigned int& commonMailNumber, unsigned int& deletedMailNumber, unsigned int& commonMailSize, unsigned int& deletedMailSize )
{
	
	char sqlcmd[1024];

	commonMailNumber = 0;
	deletedMailNumber = 0;
	commonMailSize = 0;
	deletedMailSize = 0;
	
	sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d<>%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);

	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
	
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				commonMailNumber = atoi(row[0]);
				mysql_free_result(query_result);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
	
	sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
	
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				deletedMailNumber = atoi(row[0]);
				mysql_free_result(query_result);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}

	sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d<>%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
	
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				commonMailSize = row[0]== NULL ? 0 :  atoi(row[0]);
				mysql_free_result(query_result);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}

	sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
	
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			row = mysql_fetch_row(query_result);
			if(row)
			{
				deletedMailSize =  row[0]== NULL ? 0 :  atoi(row[0]);
				mysql_free_result(query_result);
			}
			else
			{
				mysql_free_result(query_result);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}

	return 0;
}

int MailStorage::GetAllDirOfID(const char* username, vector<int>& didtbl)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s'", username);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
				didtbl.push_back(row[0] == NULL ? 0 : atoi(row[0]));
			}
			mysql_free_result(query_result);
		}
		else
		{
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
		return 0;
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::GetUserStorage(const char* username, unsigned int& commonMailNumber, unsigned int& deletedMailNumber, unsigned int& commonMailSize, unsigned int& deletedMailSize )
{
	vector<int> didtbl;
	GetAllDirOfID(username, didtbl);
	
	commonMailNumber = 0;
	deletedMailNumber = 0;
	commonMailSize = 0;
	deletedMailSize = 0;

	char sqlcmd[1024];
	
	for(int x = 0; x < didtbl.size(); x++)
	{
		sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d<>%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
		
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				row = mysql_fetch_row(query_result);
				if(row)
				{
					commonMailNumber += atoi(row[0]);
					mysql_free_result(query_result);
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
		
		sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d=%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
		
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				row = mysql_fetch_row(query_result);
				if(row)
				{
					deletedMailNumber += atoi(row[0]);
					mysql_free_result(query_result);
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}

		sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d<>%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
		
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				row = mysql_fetch_row(query_result);
				if(row)
				{
					commonMailSize += row[0] == NULL ? 0 : atoi(row[0]);
					mysql_free_result(query_result);
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}

		sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d=%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *query_result;
			MYSQL_ROW row;
		
			query_result = mysql_store_result(&m_hMySQL);
			
			if(query_result)
			{
				row = mysql_fetch_row(query_result);
				if(row)
				{
					deletedMailSize += row[0] == NULL ? 0 : atoi(row[0]);
					mysql_free_result(query_result);
				}
				else
				{
					mysql_free_result(query_result);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}

	return 0;
}

int MailStorage::MTALock()
{
   
    char sqlcmd[1024];

	sprintf(sqlcmd, "LOCK TABLES mailtbl write");
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::MTAUnlock()
{
    
    char sqlcmd[1024];

	sprintf(sqlcmd, "UNLOCK TABLES");
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::SaveMailBodyToDB(const char* emlfile, const char* fragment)
{
    string strSafetyEmlfile= emlfile;
	SqlSafetyString(strSafetyEmlfile);
    
    string strSafetyFragment = fragment;
	SqlSafetyString(strSafetyFragment);
	
	char* sqlcmd = (char*)malloc(strSafetyEmlfile.length() + strSafetyFragment.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "INSERT INTO mbodytbl(mbody, mfragment) VALUES('%s','%s')", 
			strSafetyEmlfile.c_str(), strSafetyFragment.c_str());

		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::LoadMailBodyToFile(const char* emlfile, const char* fullpath)
{
    char sqlcmd[1024];
    
    string strSafetyEmlfile= emlfile;
	SqlSafetyString(strSafetyEmlfile);
    
	sprintf(sqlcmd, "SELECT mfragment FROM mbodytbl WHERE mbody='%s' ORDER BY mtime", strSafetyEmlfile.c_str());
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{		
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
            ofstream * ofile = new ofstream(fullpath, ios_base::binary|ios::out|ios::trunc);			
            if(ofile)
            {		
                if(!ofile->is_open())
                {
                    delete ofile;
                    return -1;
                }
                
                while((row = mysql_fetch_row(query_result)))
                {
                    unsigned long* lengths = mysql_fetch_lengths(query_result);
                    ofile->write(row[0], lengths[0]);
                }

                mysql_free_result(query_result);         
                
                ofile->close();
                delete ofile;
                
                return 0;
            }
            
            return -1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::InsertMTA(const char* mta)
{
    string strSafetyMTA = mta;
	SqlSafetyString(strSafetyMTA);
    
    char sqlcmd[1024];

	sprintf(sqlcmd, "DELETE FROM mtatbl WHERE mta='%s'", strSafetyMTA.c_str());
	
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }
	
	sprintf(sqlcmd, "INSERT INTO mtatbl(mta, active_time) VALUES('%s',CURRENT_TIMESTAMP)", 
        strSafetyMTA.c_str());

    if(Query(sqlcmd, strlen(sqlcmd)) == 0)
    {
        return 0;
    }
    else
    {
        show_error(&m_hMySQL, sqlcmd);
        return -1;
    }
}

int MailStorage::DeleteMTA(const char* mta)
{
    string strSafetyMTA= mta;
	SqlSafetyString(strSafetyMTA);
    
    char sqlcmd[1024];

	sprintf(sqlcmd, "DELETE FROM mtatbl WHERE mta='%s'", strSafetyMTA.c_str());
	
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
	
	return 0;
}

int MailStorage::UpdateMTA(const char* mta)
{
    string strSafetyMTA= mta;
	SqlSafetyString(strSafetyMTA);
    
    char sqlcmd[1024];

	sprintf(sqlcmd, "UPDATE mtatbl SET active_time=CURRENT_TIMESTAMP WHERE mta='%s'", strSafetyMTA.c_str());
	
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
    return 0;
}

int MailStorage::GetMTAIndex(const char* mta, unsigned int live_sec, unsigned int& mta_index, unsigned int& mta_count)
{
    
    char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT mta FROM mtatbl WHERE TIMESTAMPDIFF(SECOND, active_time, CURRENT_TIMESTAMP) < %d ORDER BY mta", live_sec);
    
	if(Query(sqlcmd, strlen(sqlcmd)) != 0)
	{
        show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
    else
    {
        mta_index = 0;
        MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
            unsigned int l = 0;
			while((row = mysql_fetch_row(query_result)))
			{
                if(strcmp(row[0], mta) == 0)
                {
                    mta_index = l;
                    l++;
                }
			}
            mta_count = l;
			mysql_free_result(query_result);
			return 0;
		}
		else
		{
			return -1;
		}
    }
    return 0;
}

int MailStorage::ListMyMessage(const char* to, string & message_text, vector<int>& xids)
{    
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT xid, xmessage FROM xmpptbl WHERE xto = '%s' OR xto like '%s/%%' order by xtime", to, to);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
                xids.push_back(atoi(row[0]));
                
                message_text += row[1];
                message_text += "\r\n";
				
			}
			mysql_free_result(query_result);
            
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::RemoveMyMessage(int xid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "DELETE FROM xmpptbl WHERE xid=%d", xid);
	
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			mysql_free_result(query_result);
		}
		else
		{
			return -1;
		}

		return 0;
	}
	else
	{
	    
		show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}

int MailStorage::InsertMyMessage(const char* xfrom, const char* xto, const char* xmessage)
{
	string strSafetyFrom = xfrom;
	SqlSafetyString(strSafetyFrom);
	
	string strSafetyTo = xto;
	SqlSafetyString(strSafetyTo);

	string strSafetyMessage = xmessage;
	SqlSafetyString(strSafetyMessage);
	
	char* sqlcmd = (char*)malloc(strSafetyMessage.length() + strSafetyTo.length() + strSafetyFrom.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "INSERT INTO xmpptbl(xfrom, xto, xmessage) VALUES('%s','%s', '%s')",
			strSafetyFrom.c_str(), strSafetyTo.c_str(), strSafetyMessage.c_str());
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::InsertBuddy(const char* usr1, const char* usr2)
{
	string strSafetyUser1 = usr1;
	SqlSafetyString(strSafetyUser1);
	
	string strSafetyUser2 = usr2;
	SqlSafetyString(strSafetyUser2);

	
	char* sqlcmd = (char*)malloc(strSafetyUser1.length() + strSafetyUser2.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "INSERT INTO xmppbuddytbl(xusr1, xusr2) VALUES('%s','%s')",
			strSafetyUser1.c_str(), strSafetyUser2.c_str());
            
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::RemoveBuddy(const char* usr1, const char* usr2)
{
	string strSafetyUser1 = usr1;
	SqlSafetyString(strSafetyUser1);
	
	string strSafetyUser2 = usr2;
	SqlSafetyString(strSafetyUser2);

	
	char* sqlcmd = (char*)malloc(strSafetyUser1.length() + strSafetyUser2.length() + 1024);
	if(sqlcmd)
	{
		sprintf(sqlcmd, "DELETE FROM xmppbuddytbl WHERE (xusr1 = '%s' AND xusr2 = '%s') OR (xusr1 = '%s' AND xusr2 = '%s') ",
			strSafetyUser1.c_str(), strSafetyUser2.c_str(), strSafetyUser2.c_str(), strSafetyUser1.c_str());
            
		if(Query(sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL, sqlcmd);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::ListBuddys(const char* selfid, vector<string>& buddys)
{    
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT xusr1, xusr2 FROM xmppbuddytbl WHERE xusr1 = '%s' OR xusr2 = '%s'", selfid, selfid);
	if(Query(sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *query_result;
		MYSQL_ROW row;
		
		query_result = mysql_store_result(&m_hMySQL);
		
		if(query_result)
		{
			while((row = mysql_fetch_row(query_result)))
			{
                if(strcmp(row[0], selfid) != 0 )
                {
                    buddys.push_back(row[0]);
                }
                else if(strcmp(row[1], selfid) != 0 )
                {
                    buddys.push_back(row[1]);
                }
			}
			mysql_free_result(query_result);
            
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    show_error(&m_hMySQL, sqlcmd);
		return -1;
	}
}