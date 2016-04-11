#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <mysql.h>
#include <pthread.h>
#include <vector>
#include <map>
#include <string>
#include "util/general.h"
using namespace std; 

#define MAIL_ROOT_DIR	"/var/erisemail/"
#define MAIL_INBOX_DIR	"INBOX"

#define RECENT_MSG_TIME	(3600*24*7) //7 days

typedef enum
{	
	upReadOnly = 0,
	upReadWrite = 1
} User_Privilege;

#define MSG_ATTR_SEEN			(1<<0)
#define MSG_ATTR_ANSWERED		(1<<1)
#define MSG_ATTR_FLAGGED		(1<<2)
#define MSG_ATTR_DELETED		(1<<3)
#define MSG_ATTR_DRAFT			(1<<4)
#define MSG_ATTR_RECENT			(1<<5)
#define MSG_ATTR_UNAUDITED		(1<<6)

#define DIR_ATTR_MARKED			(1<<0)
#define DIR_ATTR_NOSELECT		(1<<1)
#define DIR_ATTR_NOINFERIORS	(1<<2)
#define DIR_ATTR_SUBSCRIBED		(1<<3)

typedef enum
{
	duCommon = 0,
	duInbox,
	duDrafts,
	duSent,
	duTrash,
	duJunk
}Dir_Usage;

typedef enum
{
	utMember = 1,
	utGroup = 2
}UserType;

typedef enum
{
	ldFalse = 0,
	ldTrue = 1
}LevelDeault;

typedef enum
{
	eaFalse = 0,
	eaTrue = 1
}EnableAudit;

typedef enum
{
	urDisabled = 0,
	urGeneralUser = 1,
	urAdministrator = 2
}UserRole;

typedef enum
{
	usActive = 0,
	usDisabled = 1
}UserStatus;


typedef enum
{
	mtLocal = 1,
	mtExtern = 2,
	mtFowarding = 3
}MailType;

typedef struct
{
	unsigned int mid;
	char uniqid[256];
	string mailfrom;
	string rcptto;	
	unsigned int mtime;
	unsigned int mstatus;
	MailType mtype;
	int mdid;
	unsigned int length;
	unsigned int reserve;
}Mail_Info;

typedef struct
{
	unsigned int drafttime;
	string subject;
	unsigned int lastupdatetime;
	unsigned int reserve;
}Draft_Info;

typedef struct
{
	char username[64];
	char alias[128];
	UserType type;
	UserRole role;
	unsigned int size;
	UserStatus status;
	int level;
}User_Info;

typedef struct
{	
	int did;
	char owner[64];
	char name[256];
	unsigned int status;
	int parentid;
	int childrennum;
}Dir_Info;

typedef struct
{	
	int did;
	char owner[64];
	char name[256];
	string path;
	unsigned int status;
	int parentid;
}Dir_Tree_Ex;

typedef struct
{
	unsigned int lid;
	string lname;
	string ldescription;
	unsigned long long mailmaxsize;
	unsigned long long boxmaxsize;
	EnableAudit enableaudit;
	unsigned int mailsizethreshold;
	unsigned int attachsizethreshold;
	LevelDeault ldefault;
	unsigned int ltime;
} Level_Info;

class MailStorage
{
public:
	MailStorage(const char* encoding, const char* private_path, unsigned global_uid);
	virtual ~MailStorage();

	//system
	int Connect(const char * host, const char* username, const char* password, const char* database);
	void Close();
	int Ping();
	
	void KeepLive();
	
	void EntryThread();

	void LeaveThread();
	
	int Install(const char* database);
	int Uninstall(const char* database);
	
	//User and Group
	int CheckAdmin(const char* username, const char* password);
	
	int CheckLogin(const char* username, const char* password);
	int VerifyUser(const char* username);
	int VerifyGroup(const char* groupname);
	int AddID(const char* username, const char* password, const char* alias, UserType type, UserRole role, unsigned int size, int level);
	int UpdateID(const char* username, const char* alias, UserStatus status, int level);
	int DelID(const char* username);	
	int DelAllMailOfDir(int mdirid);

	int AddLevel(const char* lname, const char* ldescription, unsigned long long mailmaxsize, unsigned long long boxlmaxsize, EnableAudit lenableaudit, unsigned int mailsizethreshold, unsigned int attachsizethreshold, unsigned int& lid);
	int UpdateLevel(unsigned int lid, const char* lname, const char* ldescription, unsigned long long mailmaxsize, unsigned long long boxmaxsize, EnableAudit lenableaudit, unsigned int mailsizethreshold, unsigned int attachsizethreshold);

	int DelLevel(unsigned int lid);
	int SetDefaultLevel(unsigned int lid);
	int ListLevel(vector<Level_Info>& litbl);
	int GetLevel(int lid, Level_Info& linfo);
	int GetDefaultLevel(int& lid);	
	int GetDefaultLevel(Level_Info& linfo);
	int GetUserLevel(const char* username, Level_Info& linfo);
	int SetUserLevel(const char* username, int lid);

	int AppendUserToGroup(const char* username, const char* groupname);
	int RemoveUserFromGroup(const char* username, const char* groupname);
	
	int LoadMailFromFile(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* emlfilepath, int& mailid);
	int UpdateMailFromFile(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* emlfilepath, int mailid);

	int ListMailByDir(const char* username, vector<Mail_Info>& listtbl, const char* diref);
	int ListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid);
	int ListAllMail(vector<Mail_Info>& listtbl);
	int ListExternMail(vector<Mail_Info>& listtbl);

	int GetMailUID(int mid, string uid);
	int Prefoward(int mid);
	int CancelFoward(int mid);

	int ListID(vector<User_Info>& listtbl, string orderby = "utime", BOOL desc = FALSE);
	int ListGroup(vector<User_Info>& listtbl);	
	int ListMember(vector<User_Info>& listtbl);
	
	int GetID(const char* uname, User_Info& uinfo);

	
	int DelAllMailOfID(const char* username);

	int Passwd(const char* uname, const char* password);
	int Alias(const char* uname, const char* alias);

	int IsAdmin(const char* username);
	
	int SetUserStatus(const char* uname, UserStatus status);
	int SetUserSize(const char* uname, unsigned int size);

	int GetUserSize(const char* uname, unsigned long long& size);
		

	int DumpMailToFile(int mid, string& dumpfile);
	int DelMail(const char* username, int mid);
	int ShitDelMail(int mid);
	int GetMailDir(int mid, int & dirid);
	int GetMailOwner(int mid, string & owner);
	int GetDirOwner(int dirid, string & owner);
	
	int ListMemberOfGroup(const char* group, vector<User_Info>& listtbl);
	int GetPassword(const char* username, string& password);

	int GetMailLen(int mid, int& mlen);
	int GetMailBody(int mid, char* body);

	int GetMailIndex(int mid, string& path);
	
	//dir method
	
	int CreateDir(const char* username, const char* dirref);
	int CreateDir(const char* username, const char* dirname, int parentid);
	
	int SetDirStatus(const char* username, const char* dirref, unsigned int status);
	int GetDirStatus(const char* username, const char* dirref, unsigned int& status);

	int SetMailStatus(const char* username, int mid, unsigned int status);
	int GetMailStatus(int mid, unsigned int& status);

	int GetMailFromAndTo(int mid, string & from, string &to);
	
	int GetDirID(const char* username, const char* dirref, int & dirid);
	int GetDirID(const char* username, const char* dirref, vector<int>& vdirid);

	int IsDirExist(const char* username, const char* dirref);
	int IsDirExist(const char* username, int dirid);
	int IsSubDirExist(const char* username, int parentid, const char* dirname);
	
	int GetDirParentID(const char* username, const char* dirref, int& parentid);
	int GetDirParentID(const char* username, int dirid, int& parentid);
	int GetDirParentID(const char* username, const char* dirref, vector<int>& vparentid);

	int GetInboxID(const char* username, int &dirid);
	int GetSentID(const char* username, int &dirid);
	int GetDraftsID(const char* username, int &dirid);
	int GetTrashID(const char* username, int &dirid);
	int GetJunkID(const char* username, int &dirid);
	
	int RenameDir(const char* username, const char* oldname, const char* newname);

	int DeleteDir(const char* username, int dirid);
	int DeleteDir(const char* username, const char* dirref);
	int TraversalListDir(const char* username, const char* dirref, vector<Dir_Tree_Ex>& listtbl);
	int ListSubDir(const char* username, int pid, vector<Dir_Info>& listtbl);
	int SplitDir(const char* dirref, string& parentdir, string& dirname);
	int GetDirMailCount(const char* username, int dirid, unsigned int& count);
	int GetUnauditedMailCount(const char* username, unsigned int& count);


	int LimitListMailByDir(const char* username, vector<Mail_Info>& listtbl, unsigned int dirid, int beg, int rows);
	int LimitListUnauditedMailByDir(const char* username, vector<Mail_Info>& listtbl, int beg, int rows);

	int InsertMail(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx, const char* muniqid,int mdirid, unsigned int mstatus, const char* mbody, unsigned int msize, int& mailid);
	int UpdateMail(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx, const char* muniqid,int mdirid, unsigned int mstatus, const char* mbody, unsigned int msize,  int mailid);
	int InsertMailIndex(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* mpath, unsigned int msize,  int& mailid);
	int UpdateMailIndex(const char* mfrom, const char* mto, unsigned int mtime,unsigned int mtx,const char* muniqid,int mdirid, unsigned int mstatus, const char* mpath, unsigned int msize,  int mailid);

	int ChangeMailDir(int mdirid, int mailid);
	
	int GetUnseenMail(int dirid, int& num);
	int GetDirName(const char * username, int dirid, string& dirname);
	int GetDirPath(const char * username, int dirid, string& globalpath);

	int EmptyDir(const char* username, int dirid);

	
	int GetAllDirOfID(const char* username, vector<int>& didtbl);

	int GetGlobalStorage(unsigned int& commonMailNumber, unsigned int& deletedMailNumber, unsigned int& commonMailSize, unsigned int& deletedMailSize );	
	int GetUserStorage(const char* username, unsigned int& commonMailNumber, unsigned int& deletedMailNumber, unsigned int& commonMailSize, unsigned int& deletedMailSize );
	
	int SetMailSize(unsigned int mid, unsigned int msize);
	
protected:
	MYSQL* m_hMySQL;

	BOOL m_bOpened;
	void SqlSafetyString(string& strInOut);

	string m_host;
	string m_username;
	string m_password;
	string m_database;
    
    string m_encoding;
    string m_private_path;
    unsigned m_global_uid;
    
    pthread_mutex_t m_thread_pool_mutex;
    
    map<string, string> m_userpwd_cache;
    int m_userpwd_cache_update_time;
private:
    int mysql_thread_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length)
    {
        pthread_mutex_lock(&m_thread_pool_mutex);
        int ret = mysql_real_query(mysql, stmt_str, length);
        pthread_mutex_unlock(&m_thread_pool_mutex);
        return ret;
    }
};
	
#endif /* _STORAGE_H_ */

