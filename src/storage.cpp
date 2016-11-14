/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include "storage.h"
#include "util/general.h"
#include "base.h"
#include "util/md5.h"
#include "util/trace.h"
#include <time.h>
#include "letter.h"

#define CODE_KEY "qazWSX#$%123"

static void show_error(MYSQL *mysql)
{
    fprintf(stderr, "MySQL error(%d) [%s] \"%s\"\n", mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql));
}

BOOL MailStorage::m_userpwd_cache_updated = TRUE;

void MailStorage::SqlSafetyString(string& strInOut)
{
	char * szOut = new char[strInOut.length()* 2 + 1];
    memset(szOut, 0, strInOut.length()* 2 + 1);
	mysql_escape_string(szOut, strInOut.c_str(), strInOut.length());
	strInOut = szOut;
	delete szOut;
}

MailStorage::MailStorage(const char* encoding, const char* private_path, memcached_st * memcached)
{
    pthread_rwlock_init(&m_userpwd_cache_lock, NULL);
    m_userpwd_cache.clear();
    m_userpwd_cache_update_time = 0;
    m_userpwd_cache_updated = TRUE;
    m_encoding = encoding;
    m_private_path = private_path;
    
    m_bOpened = FALSE;
    mysql_init(&m_hMySQL);
    
    m_memcached = memcached;
    srandom(time(NULL));
}

MailStorage::~MailStorage()
{
	Close();
    pthread_rwlock_destroy(&m_userpwd_cache_lock);
}

int MailStorage::Connect(const char * host, const char* username, const char* password, const char* database, unsigned short port, const char* sock_file)
{
    unsigned int timeout_val = 20;
    mysql_options(&m_hMySQL, MYSQL_OPT_CONNECT_TIMEOUT, (const void*)&timeout_val);
    mysql_options(&m_hMySQL, MYSQL_OPT_READ_TIMEOUT, (const void*)&timeout_val);
    mysql_options(&m_hMySQL, MYSQL_OPT_WRITE_TIMEOUT, (const void*)&timeout_val);
     
    if(mysql_real_connect(&m_hMySQL, host, username, password, database, port, sock_file, 0) != NULL)
    {
        char arg_value = 1;
        mysql_options(&m_hMySQL, MYSQL_OPT_RECONNECT, &arg_value);
                
        m_host = host;
        m_username = username;
        m_password = password;
        m_port = port;
        m_sock_file = sock_file;
        if(database != NULL)
            m_database = database;
        
        m_bOpened = TRUE;
        return 0;
    }
    else
    {
        show_error(&m_hMySQL);
        return -1;	
    }
}

void MailStorage::Close()
{
    mysql_close(&m_hMySQL);
    
    if(m_bOpened)
    {
        m_bOpened = FALSE;
    }
}

int MailStorage::Ping()
{
	if(m_bOpened)
	{
	    int rc = mysql_ping(&m_hMySQL);
		return rc;
	}
	else
	{
		return -1;
	}
}

void MailStorage::KeepLive()
{
	if(Ping() != 0)
	{
		Close();
		Connect(m_host.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), m_port, m_sock_file.c_str());
	}
}

void MailStorage::EntryThread()
{
	mysql_thread_init();
}

void MailStorage::LeaveThread()
{
	mysql_thread_end();
}


int MailStorage::Install(const char* database)
{
	char sqlcmd[1024];
    //Transaction begin
	mysql_autocommit(&m_hMySQL, 0);
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
		show_error(&m_hMySQL);
		return -1;
	}
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));

	sprintf(sqlcmd, "USE %s", database);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
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
		"PRIMARY KEY ( `lid` ) ) ENGINE = MYISAM",
		database);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));

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
		"PRIMARY KEY ( `did` ) ) ENGINE = MYISAM",
		database);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
		
	//Crate User table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`usertbl` ("
		"`uid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`uname` VARCHAR( 64 ) NOT NULL ,"
		"`upasswd` BLOB NOT NULL ,"
		"`ualias` VARCHAR( 256 ) NOT NULL ,"
		"`utype` INT UNSIGNED NOT NULL ,"
		"`urole` INT UNSIGNED NOT NULL ,"
		"`usize` INT UNSIGNED NOT NULL DEFAULT %d ,"
		"`ustatus` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`ulevel` INT NOT NULL DEFAULT '0' ,"
		"`utime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `uid` ) ) ENGINE = MYISAM",
		database, MAX_EMAIL_LEN);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
			
	//Create group table
	sprintf(sqlcmd,
		"CREATE TABLE IF NOT EXISTS `%s`.`grouptbl` ("
		"`gid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`groupname` VARCHAR( 64 ) NOT NULL ,"
		"`membername` VARCHAR( 64 ) NOT NULL ,"
		"`gtime` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"PRIMARY KEY ( `gid` ) ) ENGINE = MYISAM ",
		database);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
			
	//Create mail table
	sprintf(sqlcmd, 
		"CREATE TABLE IF NOT EXISTS `%s`.`mailtbl` ("
		"`mid` INT UNSIGNED NOT NULL AUTO_INCREMENT ,"
		"`muniqid` VARCHAR( 256 ) NOT NULL ,"
		"`mfrom` VARCHAR( 256 ) NULL ,"
		"`mto` VARCHAR( 256 ) NULL ,"
		"`mbody` LONGTEXT NOT NULL ,"
		"`msize` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtime` INT UNSIGNED NOT NULL DEFAULT '0',"
		"`mtx` INT UNSIGNED NOT NULL ,"
		"`mstatus` INT UNSIGNED NOT NULL DEFAULT '0' ,"
		"`mdirid` INT NOT NULL DEFAULT -1 ,"
		"PRIMARY KEY ( `mid` ) ) ENGINE = MYISAM ", 
		database);
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "DROP FUNCTION IF EXISTS post_notify");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "CREATE FUNCTION post_notify RETURNS STRING SONAME \"postudf.so\"");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "DROP TRIGGER IF EXISTS postmail_notify_insert");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "DROP TRIGGER IF EXISTS postmail_notify_update");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "CREATE TRIGGER postmail_notify_insert AFTER INSERT ON mailtbl FOR EACH ROW BEGIN SET @rs = post_notify(); END");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	sprintf(sqlcmd, "CREATE TRIGGER postmail_notify_update AFTER UPDATE ON mailtbl FOR EACH ROW BEGIN SET @rs = post_notify(); END");
	mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
	
	if(mysql_commit(&m_hMySQL) == 0)
	{
		mysql_autocommit(&m_hMySQL, 1);
	}
    else
	{
		show_error(&m_hMySQL);
		mysql_rollback(&m_hMySQL);
		mysql_autocommit(&m_hMySQL, 1);
		return -1;
	}
	
        unsigned int lid;
	if(AddLevel("default", "The system's default level", 5000*1024, 500000*1024, eaFalse, 5000*1024, 5000*1024, lid) == 0)
	{
		SetDefaultLevel(lid);
	}
	else
	{
		return -1;
	}	

	if(AddID("admin", "admin", "Administrator", utMember, urAdministrator, MAX_EMAIL_LEN, -1) == -1)
	{
		return -1;
	}
	return 0;
        
}

int MailStorage::Uninstall(const char* database)
{
	char sqlcmd[1024];
	sprintf(sqlcmd,"DROP DATABASE %s", database);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	return -1;
}

int MailStorage::SetMailSize(unsigned int mid, unsigned int msize)
{
	char sqlcmd[1024];
	sprintf(sqlcmd,"UPDATE mailtbl SET msize=%d WHERE mid=%d", msize, mid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL);
		return -1;
	}
	return 0;
}

int MailStorage::CheckAdmin(const char* username, const char* password)
{
    if(strcmp(username, "") == 0 || strcmp(password, "") == 0)
        return -1;
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND DECODE(upasswd,'%s') = '%s' AND ustatus = %d AND urole=%d AND utype = %d", strSafetyUsername.c_str(), CODE_KEY, password, usActive, urAdministrator, utMember);

	//printf("%s\r\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			if( mysql_num_rows(qResult) > 0 )
			{
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
		show_error(&m_hMySQL);
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
    
    pthread_rwlock_rdlock(&m_userpwd_cache_lock);
    if(m_userpwd_cache.size() == 0 || time(NULL) - m_userpwd_cache_update_time > 300 || m_userpwd_cache_updated == TRUE)
    {
        pthread_rwlock_unlock(&m_userpwd_cache_lock); //release read
        pthread_rwlock_wrlock(&m_userpwd_cache_lock); //acquire write
        m_userpwd_cache.clear();
        sprintf(sqlcmd, "SELECT uname, DECODE(upasswd,'%s') FROM usertbl WHERE ustatus = %d AND utype = %d", CODE_KEY, usActive, utMember);
        
        if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
        {
            MYSQL_RES *qResult;
            MYSQL_ROW row;
            
            qResult = mysql_store_result(&m_hMySQL);
            
            if(qResult)
            {           
                while((row = mysql_fetch_row(qResult)))
                {
                    m_userpwd_cache.insert(make_pair<string, string>(row[0], row[1]));
                }

                mysql_free_result(qResult);
                
                m_userpwd_cache_update_time = time(NULL);
                m_userpwd_cache_updated = FALSE;
                pthread_rwlock_unlock(&m_userpwd_cache_lock);
                return 0;
            }
            else
            {
                pthread_rwlock_unlock(&m_userpwd_cache_lock);
                return -1;
            }
        }
        else
        {
            show_error(&m_hMySQL);
            pthread_rwlock_unlock(&m_userpwd_cache_lock);
            return -1;
        }
	}
	
	map<string, string>::iterator it = m_userpwd_cache.find(username);
	if(it != m_userpwd_cache.end() && it->second == password)
    {
        pthread_rwlock_unlock(&m_userpwd_cache_lock);
        return 0;
    }
    else
    {
        pthread_rwlock_unlock(&m_userpwd_cache_lock);
        return -1;
    }
}

int MailStorage::GetPassword(const char* username, string& password)
{
	string strpwd;
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
		
	sprintf(sqlcmd, "SELECT DECODE(upasswd,'%s') FROM usertbl WHERE uname='%s' AND utype=%d", CODE_KEY, strSafetyUsername.c_str(), utMember);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			if( mysql_num_rows(qResult) == 1 )
			{
				MYSQL_ROW row;
				row = mysql_fetch_row(qResult);
				if(row == NULL)
				{
					mysql_free_result(qResult);
					return -1;
				}
				else
				{
					password = row[0];
					mysql_free_result(qResult);
					return 0;
				}
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		show_error(&m_hMySQL);
		return -1;
	}
}

int MailStorage::VerifyUser(const char* username)
{
	char sqlcmd[1024];
	string strSafetyUsername = username;
	SqlSafetyString(strSafetyUsername);
	
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND utype=%d", strSafetyUsername.c_str(), utMember);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			if( mysql_num_rows(qResult) > 0 )
			{
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::VerifyGroup(const char* groupname)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND utype=%d", groupname, utGroup);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			if( mysql_num_rows(qResult) > 0 )
			{	
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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

	//printf("%s\n", sqlcmd);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		lid = mysql_insert_id(&m_hMySQL);
		return 0;
	}
	else
	{
	    show_error(&m_hMySQL);
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

	//printf("%s\n", sqlcmd);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
	{
		show_error(&m_hMySQL);
		return -1;
	}
}

int MailStorage::DelLevel(unsigned int lid)
{
	char sqlcmd[1024];
	int defaultLid;
	GetDefaultLevel(defaultLid);
	
	sprintf(sqlcmd, "UPDATE usertbl SET ulevel=%d WHERE ulevel=%d", defaultLid, lid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
	{	
		show_error(&m_hMySQL);
		return -1;
	}
	
	sprintf(sqlcmd, "delete FROM leveltbl WHERE lid=%d AND ldefault <> %d", lid, ldTrue);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
	{
		show_error(&m_hMySQL);
		return -1;
	}
	return 0;
}

int MailStorage::SetDefaultLevel(unsigned int lid)
{	
	char sqlcmd[1024];

	sprintf(sqlcmd, "UPDATE leveltbl SET ldefault = %d WHERE ldefault = %d", ldFalse, ldTrue);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
	{
		return -1;
	}
	
	sprintf(sqlcmd, "UPDATE leveltbl SET ldefault = %d WHERE lid = %d", ldTrue, lid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
	{
		return -1;
	}
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::GetDefaultLevel(int& lid)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid FROM leveltbl WHERE ldefault = %d", ldTrue);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				lid = row[0] == NULL ? 0 : atoi(row[0]);
			}
			else
			{
				show_error(&m_hMySQL);
				mysql_free_result(qResult);
				return -1;
			}

			mysql_free_result(qResult);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetDefaultLevel(Level_Info& liinfo)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid, lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime  FROM leveltbl WHERE ldefault = %d", ldTrue);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
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
				mysql_free_result(qResult);
				return -1;
			}

			mysql_free_result(qResult);
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

int MailStorage::ListLevel(vector<Level_Info>& litbl)
{
	litbl.clear();
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT lid, lname, ldescription, lmailmaxsize, lboxmaxsize, lenableaudit, lmailsizethreshold, lattachsizethreshold, ldefault, ltime FROM leveltbl ORDER BY ltime");
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{		
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{			
			while((row = mysql_fetch_row(qResult)))
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

			mysql_free_result(qResult);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    
		show_error(&m_hMySQL);
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
	
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{		
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
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
				mysql_free_result(qResult);
				return -1;
			}
			mysql_free_result(qResult);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
	    
		show_error(&m_hMySQL);
		return -1;
	}
		
}


int MailStorage::AddID(const char* username, const char* password, const char* alias, UserType type, UserRole role, unsigned int size, int level)
{
	if(strcasecmp(username, "postmaster") != 0)
	{
		if((VerifyUser(username) == -1)&&(VerifyGroup(username) == -1))
		{
			char sqlcmd[1024];

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
				defaultLevel = level;
			mysql_autocommit(&m_hMySQL, 0);	
			sprintf(sqlcmd, "INSERT INTO usertbl(uname, upasswd, ualias, utype, urole, usize, ustatus, ulevel, utime) VALUES('%s', ENCODE('%s','%s'), '%s', %d, %d, %d, 0, %d, %d)",
				strSafetyUsername.c_str(), strSafetyPassword.c_str(), CODE_KEY, strSafetyAlias.c_str(), type, role, size, defaultLevel, time(NULL));
				
			mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
			
			if(type == utMember)
			{
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Inbox','%s',-1, %d, %d, %d)",
					strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duInbox, time(NULL));
				mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
					
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Sent','%s',-1, %d, %d, %d)",
					strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duSent, time(NULL));
				mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));

				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Drafts','%s',-1, %d, %d, %d)",
					strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duDrafts, time(NULL));
				mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));

				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Trash','%s',-1, %d, %d, %d)",
					strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duTrash, time(NULL));
				mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));

				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Junk','%s',-1, %d, %d, %d)",
					strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duJunk, time(NULL));
				mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd));
			}
			if(mysql_commit(&m_hMySQL) == 0)
			{
				m_userpwd_cache_update_time = 0;
				m_userpwd_cache_updated = TRUE;
				mysql_autocommit(&m_hMySQL, 1);
				return 0;
			}
			else
			{
				mysql_rollback(&m_hMySQL);
				mysql_autocommit(&m_hMySQL, 1);
			}
		}
	}
	else
	{
		return -1;
	}
}

int MailStorage::UpdateID(const char* username, const char* alias, UserStatus status, int level)
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
		
	sprintf(sqlcmd, "UPDATE usertbl SET ualias='%s', ustatus=%d, ulevel=%d WHERE uname='%s'",
		strSafetyAlias.c_str(), status, realLevel, strSafetyUsername.c_str());

	//printf("%s\n", sqlcmd);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::DelAllMailOfDir(int mdirid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=(mstatus|%d) WHERE mdirid=%d", MSG_ATTR_DELETED, mdirid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			mysql_free_result(qResult);
		}
		else
		{
			return -1;
		}

		return 0;
	}
	else
	{
	    
		show_error(&m_hMySQL);
		return -1;
	}
}


int MailStorage::DelAllMailOfID(const char* username)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s'", username);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
			{
				DelAllMailOfDir(atoi(row[0]));
			}
			mysql_free_result(qResult);
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
		return 0;
	}
	else
	{
	    
		return -1;	
	}
}

int MailStorage::DelID(const char* username)
{
	if(strcasecmp(username ,"admin") == 0)
		return -1;
	
	if((VerifyUser(username) == 0)||(VerifyGroup(username) == 0))
	{
		char sqlcmd[1024];
		string strSafetyUsername = username;
		SqlSafetyString(strSafetyUsername);
		
		if(VerifyGroup(strSafetyUsername.c_str()) == 0)
		{
			sprintf(sqlcmd, "delete FROM grouptbl WHERE groupname='%s'", strSafetyUsername.c_str());
			if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
			{
				show_error(&m_hMySQL);
				return -1;
			}	
		}
		else if(VerifyUser(strSafetyUsername.c_str()) == 0)
		{
			sprintf(sqlcmd, "delete FROM grouptbl WHERE membername='%s'", strSafetyUsername.c_str());
			if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
			{
				show_error(&m_hMySQL);
				return -1;
			}
			
			sprintf(sqlcmd, "delete FROM dirtbl WHERE downer='%s'", strSafetyUsername.c_str());
			if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
			{
				
				show_error(&m_hMySQL);
				return -1;
			}

			
			if(DelAllMailOfID(strSafetyUsername.c_str()) != 0)
			{
				return -1;
			}
		
		}
		else
		{
			return -1;
		}
		
		sprintf(sqlcmd, "delete FROM usertbl WHERE uname='%s'", strSafetyUsername.c_str());
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
		{
			show_error(&m_hMySQL);
			return -1;
		}
		m_userpwd_cache_update_time = 0;
		m_userpwd_cache_updated = TRUE;
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
	sprintf(sqlcmd, "LOAD DATA LOCAL INFILE '%s' INTO TABLE mailtbl FIELDS TERMINATED BY ',' ENCLOSED BY '\"' (mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody)", sqlfilepath);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		mailid = mysql_insert_id(&m_hMySQL);
		unlink(sqlfilepath);
		return 0;
	}
	else
	{
		unlink(sqlfilepath);
		show_error(&m_hMySQL);
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
	sprintf(sqlcmd, "LOAD DATA LOCAL INFILE '%s' REPLACE INTO TABLE mailtbl FIELDS TERMINATED BY ',' ENCLOSED BY '\"' (mid,mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody)", sqlfilepath);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		unlink(sqlfilepath);
		return 0;
	}
	else
	{
		unlink(sqlfilepath);
		show_error(&m_hMySQL);
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
		sprintf(sqlcmd, "INSERT INTO mailtbl(mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody, msize) VALUES('%s','%s',%u,%u,'%s',%d,%u,'%s', %u)", 
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize);
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			mailid = mysql_insert_id(&m_hMySQL);
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL);
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::InsertMailIndex(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid, int mdirid, unsigned int mstatus, const char* mpath, unsigned int msize, int& mailid)
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
		sprintf(sqlcmd, "INSERT INTO mailtbl(mfrom,mto,mtime,mtx,muniqid,mdirid,mstatus,mbody,msize) VALUES('%s','%s',%u,%u,'%s',%d,%u,'%s', %u)", 
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize);

		//printf("%s\n", sqlcmd);
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			mailid = mysql_insert_id(&m_hMySQL);
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL);
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
		sprintf(sqlcmd, "UPDATE mailtbl SET mfrom='%s',mto='%s',mtime=%u,mtx=%u,muniqid='%s',mdirid=%d,mstatus=%u,mbody='%s',msize='%u' WHERE mid=%d", 
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize, mailid);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL);
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
		sprintf(sqlcmd, "UPDATE mailtbl SET mfrom='%s',mto='%s',mtime=%u,mtx=%u,muniqid='%s',mdirid=%d,mstatus=%u,mbody='%s', msize=%u WHERE mid=%d", 
			strSafetyFrom.c_str(), strSafetyTo.c_str(), mtime, mtx, muniqid, mdirid, mstatus, strSafetyBody.c_str(), msize, mailid);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			free(sqlcmd);
			return 0;
		}
		else
		{
			free(sqlcmd);
			show_error(&m_hMySQL);
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

		//printf("%s\n",sqlcmd );
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
			//show_error(&m_hMySQL);
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
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

int MailStorage::ListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody, muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mdirid=%d AND mstatus&%d<>%d ORDER BY mtime desc", dirid , MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
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

int MailStorage::LimitListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid, int beg, int rows)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mdirid=%d AND mstatus&%d<>%d ORDER BY mtime desc limit %d, %d", dirid , MSG_ATTR_DELETED, MSG_ATTR_DELETED, beg, rows);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
            unsigned int num_fields;
            unsigned int i;
            num_fields = mysql_num_fields(qResult);
			while((row = mysql_fetch_row(qResult)))
			{
                /*unsigned long *lengths;
                lengths = mysql_fetch_lengths(qResult);
                for(i = 0; i < num_fields; i++)
                {
                    printf("[%.*s] ", (int) lengths[i],
                           row[i] ? row[i] : "NULL");
                }
                printf("\n");*/
                
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
			mysql_free_result(qResult);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::LimitListUnauditedMailByDir(const char* username, vector<Mail_Info>& listtbl, int beg, int rows)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mstatus&%d<>%d AND mstatus&%d=%d ORDER BY mtime limit %d, %d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED, beg, rows);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
			return 0;
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::ListAllMail(vector<Mail_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody,muniqid,mid,mtime,mstatus,mfrom,mto,mtx,mdirid FROM mailtbl WHERE mstatus&%d<>%d ORDER BY mtime desc", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
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

int MailStorage::ListExternMail(vector<Mail_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mfrom, mto, muniqid, mid FROM mailtbl WHERE mtx='%d' AND mstatus&%d<>%d AND mstatus&%d<>%d ORDER BY mid", mtExtern, MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
			{
				Mail_Info mi;
				mi.mailfrom = row[0];
				mi.rcptto = row[1];
				strcpy(mi.uniqid, row[2]);
				mi.mid = atoi(row[3]);
				listtbl.push_back(mi);
			}
			mysql_free_result(qResult);
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

int MailStorage::Prefoward(int mid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE mailtbl SET mtx='%d' WHERE mtx='%d' AND mid=%d", mtFowarding, mtExtern, mid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::CancelFoward(int mid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "UPDATE mailtbl SET mtx='%d' WHERE mtx='%d' AND mid=%d", mtExtern, mtFowarding, mid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::ListMemberOfGroup(const char* group, vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT membername FROM grouptbl WHERE groupname='%s'", group);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
			{
				User_Info uinfo;
				if(GetID(row[0],uinfo) == 0)
				{
					listtbl.push_back(uinfo);
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
				
			}
			mysql_free_result(qResult);
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

int MailStorage::ListID(vector<User_Info>& listtbl, string orderby, BOOL desc)
{
	listtbl.clear();
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus, ulevel FROM usertbl ORDER BY %s %s", orderby == "" ? "utime" : orderby.c_str(), desc ? "desc" : "");

	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
			{
				User_Info ui;
				strcpy(ui.username, row[0]);
				strcpy(ui.alias, row[1]);
				ui.type = UserType(atoi(row[2]));
				ui.role = UserRole(atoi(row[3]));
				ui.size = atoi(row[4]);
				ui.status = (UserStatus)atoi(row[5]);
				ui.level = atoi(row[6]);
				listtbl.push_back(ui);
			}
			mysql_free_result(qResult);
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			
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
				mysql_free_result(qResult);
				return -1;
			}
			mysql_free_result(qResult);
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


int MailStorage::ListGroup(vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus FROM usertbl WHERE utype=%d GROUP BY uname ORDER BY utime", utGroup);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
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

int MailStorage::ListMember(vector<User_Info>& listtbl)
{
	listtbl.clear();
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT uname, ualias, utype, urole, usize, ustatus FROM usertbl WHERE utype=%d GROUP BY uname ORDER BY utime", utMember);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
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
			mysql_free_result(qResult);
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
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			m_userpwd_cache_update_time = 0;
			m_userpwd_cache_updated = TRUE;
			return 0;
		}
		else
		{
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
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::SetUserStatus(const char* uname, UserStatus status)
{
	if(VerifyUser(uname) == 0)
	{
		char sqlcmd[1024];
		
		string strSafetyUsername = uname;
		SqlSafetyString(strSafetyUsername);

		sprintf(sqlcmd, "UPDATE usertbl SET ustatus=%d WHERE uname='%s'", status, strSafetyUsername.c_str());
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
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
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
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
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::GetMailBody(int mid, char* body)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody FROM mailtbl WHERE mid='%d' AND mstatus&%d<>%d", mid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				strcpy(body, row[0]);
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetMailIndex(int mid, string& path)
{	
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mbody FROM mailtbl WHERE mid='%d' AND mstatus&%d<>%d;", mid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{	
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				path = row[0];
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		show_error(&m_hMySQL);	
		return -1;
	}
}

int MailStorage::GetMailDir(int mid, int & dirid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM mailtbl WHERE mid='%d'", mid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				dirid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetMailOwner(int mid, string & owner)
{
	char sqlcmd[1024];
	int dirid;
	
	if(GetMailDir(mid, dirid) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		sprintf(sqlcmd, "SELECT downer FROM dirtbl WHERE did='%d'", dirid);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{					
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				row = mysql_fetch_row(qResult);
				if(row)
				{
					owner = row[0];
					mysql_free_result(qResult);
					return 0;
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}
	}
	else
		return -1;
}

int MailStorage::GetMailFromAndTo(int mid, string & from, string &to)
{
	char sqlcmd[1024];
	
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT mfrom, mto FROM mailtbl WHERE mid='%d'", mid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				from = row[0];
				to = row[0];
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::IsAdmin(const char* username)
{
	char sqlcmd[1024];
	
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT uname FROM usertbl WHERE uname='%s' AND urole=%d", username, urAdministrator);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetDirOwner(int dirid, string & owner)
{
	char sqlcmd[1024];
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	sprintf(sqlcmd, "SELECT downer FROM dirtbl WHERE did='%d'", dirid);

    
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				owner = row[0];
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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

int MailStorage::ShitDelMail(int mid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "delete FROM mailtbl WHERE mid='%d'", mid);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
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
		return -1;
}

int MailStorage::DelMail(const char* username, int mid)
{
	char sqlcmd[1024];
	
	string owner = "";
	if(IsAdmin(username) == 0 || (GetMailOwner(mid, owner) == 0 && strcasecmp(owner.c_str(), username) == 0))
	{
		sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=(mstatus|%d) WHERE mid='%d'", MSG_ATTR_DELETED, mid); //sprintf(sqlcmd, "delete FROM mailtbl WHERE mid='%d'", mid);
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
	}
	else
	{
		show_error(&m_hMySQL);
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
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				int count = mysql_num_rows(qResult);
				mysql_free_result(qResult);
				if(count > 0)
					return 0;
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}
		
		sprintf(sqlcmd, "INSERT INTO grouptbl(groupname, membername, gtime) VALUES('%s', '%s', %d)",
		strSafetyGroupname.c_str(), strSafetyUsername.c_str(), time(NULL));
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			return 0;
		}
		else
			return -1;
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
		
	sprintf(sqlcmd, "delete FROM grouptbl WHERE groupname='%s'and membername='%s'", strSafetyGroupname.c_str(), strSafetyUsername.c_str());
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
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
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
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
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
}

int MailStorage::DeleteDir(const char* username, int dirid)
{
	char sqlcmd[1024];
	//delete mail of ownself
	sprintf(sqlcmd, "delete FROM mailtbl WHERE mdirid=%d", dirid);
	//printf("%s\n",sqlcmd);
	if(mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
		return -1;

	//delete self
	sprintf(sqlcmd, "delete FROM dirtbl WHERE did=%d", dirid);
	//printf("%s\n",sqlcmd);
	if(mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
		return -1;

	//delete subdir of ownself
	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
	//printf("%s\n",sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while(row = mysql_fetch_row(qResult))
			{
				DeleteDir(username, atoi(row[0]));
			}
			mysql_free_result(qResult);
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
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT dparent FROM dirtbl WHERE did='%d'", dirid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				parentid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
		//if(strSafetyDirname.find("%") == string::npos)
			sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
				strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		//else
		//	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname like '%s' AND dparent=%d", 
		//		strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				//can not find the current dir via its name
				row = mysql_fetch_row(qResult);
				if(row == NULL)
				{
					mysql_free_result(qResult);
					return -1;
				}
				else
				{
					parentid = atoi(row[0]);
					mysql_free_result(qResult);
				}
			}
		}
		else
		{
		    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			//can not find the current dir via its name
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				return -1;
			}
			else
			{
				mysql_free_result(qResult);
				return 0;
			}
		}
	}
	else
	{
	    
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

	//if(strSafetyDirname.find("%") == string::npos)
		sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
			strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
	//else
	//	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname like '%s' AND dparent=%d", 
	//		strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			//can not find the current dir via its name
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				return -1;
			}
			else
			{
				mysql_free_result(qResult);
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
		    
		    if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		    {
			    MYSQL_RES *qResult;
			    MYSQL_ROW row;
			    qResult = mysql_store_result(&m_hMySQL);
			    
			    if(qResult)
			    {
				    //can not find the current dir via its name
				    while((row = mysql_fetch_row(qResult)))
				    {
					    vchildid.push_back(atoi(row[0]));
				    }
				    mysql_free_result(qResult);
			    }
			    else
			    {
				    show_error(&m_hMySQL);
				    return -1;
			    }
		    }
		    else
		    {
		        
			    show_error(&m_hMySQL);
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

		//if(strSafetyDirname.find("%") == string::npos)
			sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname='%s' AND dparent=%d", 
				strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		//else
		//	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s' AND dname like '%s' AND dparent=%d", 
		//		strSafetyUsername.c_str(), strSafetyDirname.c_str(), parentid);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				//can not find the current dir via its name
				row = mysql_fetch_row(qResult);
				if(row == NULL)
				{
					mysql_free_result(qResult);
					show_error(&m_hMySQL);
					return -1;
				}
				else
				{
					parentid = atoi(row[0]);
					mysql_free_result(qResult);
				}
			}
			else
			{
				show_error(&m_hMySQL);
				return -1;
			}
		}
		else
		{
		    
			show_error(&m_hMySQL);
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
	//printf("%s\n",sqlcmd );
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Inbox','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duInbox, time(NULL));
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duInbox, strSafetyUsername.c_str(), "Inbox");
					if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);

				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Junk','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duJunk, time(NULL));
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duJunk, strSafetyUsername.c_str(), "Junk");
					if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Drafts','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duDrafts, time(NULL));
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duDrafts, strSafetyUsername.c_str(), "Drafts");
					if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Sent','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duSent, time(NULL));
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duSent, strSafetyUsername.c_str(), "Sent");
					if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
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
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				sprintf(sqlcmd, "INSERT INTO dirtbl(dname, downer, dparent , dstatus, dusage, dtime) VALUES('Trash','%s',-1, %d, %d, %d)",
						strSafetyUsername.c_str(), DIR_ATTR_MARKED|DIR_ATTR_SUBSCRIBED, duTrash, time(NULL));
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
				{
					sprintf(sqlcmd, "UPDATE dirtbl SET dusage=%d WHERE downer='%s' AND dname='%s' AND dparent='-1'",
						duTrash, strSafetyUsername.c_str(), "Trash");
					if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) != 0)
					{
						return -1;
					}
				}
					
				return mysql_insert_id(&m_hMySQL);
			}
			else
			{
				dirid = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}


int MailStorage::GetDirMailCount(const char* username, int dirid, unsigned int& count)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM mailtbl WHERE mdirid =%d AND mstatus&%d<>%d", dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			count = mysql_num_rows(qResult);
			mysql_free_result(qResult);
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

int MailStorage::GetUnauditedMailCount(const char* username, unsigned int& count)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT mdirid FROM mailtbl WHERE mstatus&%d<>%d AND mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_UNAUDITED, MSG_ATTR_UNAUDITED);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			count = mysql_num_rows(qResult);
			mysql_free_result(qResult);
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

int MailStorage::GetDirStatus(const char* username, const char* dirref, unsigned int& status)
{
	int dirID;
	if(GetDirID(username, dirref, dirID) == -1)
		return -1;
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT dstatus FROM dirtbl WHERE did=%d", dirID);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				return -1;
			}
			else
			{
				status = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	//printf("%s\n", sqlcmd);
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
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
	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				return -1;
			}
			else
			{
				status = atoi(row[0]);
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
}

int MailStorage::GetMailUID(int mid, string uid)
{
	char sqlcmd[1024];
	sprintf(sqlcmd, "SELECT muniqid FROM mailtbl WHERE mid=%d", mid);
	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row == NULL)
			{
				mysql_free_result(qResult);
				return -1;
			}
			else
			{
				uid = row[0];
				mysql_free_result(qResult);
				return 0;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
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

	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		return 0;
	}
	else
		return -1;
	
}

int MailStorage::TraversalListDir(const char* username, const char* dirref, vector<Dir_Tree_Ex>& listtbl)
{
	char sqlcmd[1024];
	string strDirRef = dirref;
	vector<int> dirParentID;
	//printf("%s\r\n", dirref);
	//Replace(strDirRef, "%", "_%");
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
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				while(row = mysql_fetch_row(qResult))
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
				mysql_free_result(qResult);
			}
			else
				return -1;
		}
		else
		{
		    
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
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while(row = mysql_fetch_row(qResult))
			{
				Dir_Info di;
				di.did = atoi(row[2]);
				strcpy(di.name, row[0]);
				di.status = atoi(row[1]);
				di.parentid = pid;
				strcpy(di.owner, username);
				di.childrennum = 0;
				
				sprintf(sqlcmd, "SELECT count(*) FROM dirtbl WHERE dparent=%d", di.did);
				
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
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
			mysql_free_result(qResult);
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
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT COUNT(*) FROM mailtbl WHERE mdirid='%d' AND mstatus&%d<>%d AND mstatus&%d<>%d", dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED, MSG_ATTR_SEEN, MSG_ATTR_SEEN);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				
				num += atoi(row[0]);
				mysql_free_result(qResult);

				sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
				
				if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
				{					
					qResult = mysql_store_result(&m_hMySQL);
					
					if(qResult)
					{
						while((row = mysql_fetch_row(qResult)))
						{
							int pid = atoi(row[0]);
							GetUnseenMail(pid, pid);
						}
						mysql_free_result(qResult);
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
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "UPDATE mailtbl SET mstatus=(mstatus|%d) WHERE mdirid='%d' AND mstatus&%d<>%d", MSG_ATTR_DELETED, dirid, MSG_ATTR_DELETED, MSG_ATTR_DELETED);
    
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			mysql_free_result(qResult);

			sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE dparent=%d", dirid);
			
			if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
			{					
				qResult = mysql_store_result(&m_hMySQL);
				
				if(qResult)
				{
					while((row = mysql_fetch_row(qResult)))
					{
						int pid = atoi(row[0]);
						EmptyDir(username, pid);
					}
					mysql_free_result(qResult);
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
	MYSQL_RES *qResult;
	MYSQL_ROW row;
	
	sprintf(sqlcmd, "SELECT dname FROM dirtbl WHERE did='%d'", dirid);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{					
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				dirname = row[0];
				mysql_free_result(qResult);
				return 0;
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
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

	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
	
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				commonMailNumber = atoi(row[0]);
				mysql_free_result(qResult);
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}
	
	sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
	
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				deletedMailNumber = atoi(row[0]);
				mysql_free_result(qResult);
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}

	sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d<>%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
	
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				commonMailSize = row[0]== NULL ? 0 :  atoi(row[0]);
				mysql_free_result(qResult);
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}

	sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED);
	//printf("%s\n", sqlcmd);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
	
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			row = mysql_fetch_row(qResult);
			if(row)
			{
				deletedMailSize =  row[0]== NULL ? 0 :  atoi(row[0]);
				mysql_free_result(qResult);
			}
			else
			{
				mysql_free_result(qResult);
				return -1;
			}
		}
		else
			return -1;
	}
	else
	{
	    
		return -1;
	}

	return 0;
}

int MailStorage::GetAllDirOfID(const char* username, vector<int>& didtbl)
{
	char sqlcmd[1024];

	sprintf(sqlcmd, "SELECT did FROM dirtbl WHERE downer='%s'", username);
	
	if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
	{
		MYSQL_RES *qResult;
		MYSQL_ROW row;
		
		qResult = mysql_store_result(&m_hMySQL);
		
		if(qResult)
		{
			while((row = mysql_fetch_row(qResult)))
			{
				didtbl.push_back(row[0] == NULL ? 0 : atoi(row[0]));
			}
			mysql_free_result(qResult);
		}
		else
		{
			show_error(&m_hMySQL);
			return -1;
		}
		return 0;
	}
	else
	{
	    
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
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
		
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				row = mysql_fetch_row(qResult);
				if(row)
				{
					commonMailNumber += atoi(row[0]);
					mysql_free_result(qResult);
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}
		
		sprintf(sqlcmd, "SELECT count(*) FROM mailtbl WHERE mstatus&%d=%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
		
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				row = mysql_fetch_row(qResult);
				if(row)
				{
					deletedMailNumber += atoi(row[0]);
					mysql_free_result(qResult);
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}

		sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d<>%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
		
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				row = mysql_fetch_row(qResult);
				if(row)
				{
					commonMailSize += row[0] == NULL ? 0 : atoi(row[0]);
					mysql_free_result(qResult);
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}

		sprintf(sqlcmd, "SELECT SUM(msize) FROM mailtbl WHERE mstatus&%d=%d AND mdirid=%d", MSG_ATTR_DELETED, MSG_ATTR_DELETED, didtbl[x]);
		
		if( mysql_real_query(&m_hMySQL, sqlcmd, strlen(sqlcmd)) == 0)
		{
			MYSQL_RES *qResult;
			MYSQL_ROW row;
		
			qResult = mysql_store_result(&m_hMySQL);
			
			if(qResult)
			{
				row = mysql_fetch_row(qResult);
				if(row)
				{
					deletedMailSize += row[0] == NULL ? 0 : atoi(row[0]);
					mysql_free_result(qResult);
				}
				else
				{
					mysql_free_result(qResult);
					return -1;
				}
			}
			else
				return -1;
		}
		else
		{
		    
			return -1;
		}
	}

	return 0;
}

