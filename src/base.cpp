#include "base.h"
#include "util/security.h"

static char CHAR_TBL[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=~!@#$%^&*()_+[]\\{}|;':\",./<>?";

//////////////////////////////////////////////////////////////////////////
//CMailBase
//
//Software Version
string CMailBase::m_sw_version = "1.6.08";

//Global
string CMailBase::m_encoding = "UTF-8";

string CMailBase::m_private_path = "/var/erisemail/private";
string CMailBase::m_html_path = "/var/erisemail/html";


string CMailBase::m_localhostname = "mail.erisesoft.com";
string CMailBase::m_email_domain = "erisesoft.com";
string CMailBase::m_dns_server;
string CMailBase::m_hostip;

map<string, int> CMailBase::m_memcached_list;

unsigned short CMailBase::m_smtpport = 110;
BOOL CMailBase::m_enablesmtptls = FALSE;
BOOL CMailBase::m_enablerelay = FALSE;

BOOL   CMailBase::m_enablepop3 = TRUE;
unsigned short CMailBase::m_pop3port = 25;
BOOL CMailBase::m_enablepop3tls = TRUE;

BOOL   CMailBase::m_enableimap = TRUE;
unsigned short CMailBase::m_imapport = 143;
BOOL CMailBase::m_enableimaptls = TRUE;

BOOL   CMailBase::m_enablesmtps = TRUE;
unsigned short CMailBase::m_smtpsport = 465;

BOOL   CMailBase::m_enablepop3s = TRUE;
unsigned short CMailBase::m_pop3sport = 995;

BOOL   CMailBase::m_enableimaps = TRUE;
unsigned short CMailBase::m_imapsport = 993;

BOOL		CMailBase::m_enablehttp = TRUE;
unsigned short	CMailBase::m_httpport = 8081;

BOOL		CMailBase::m_enablehttps = TRUE;
unsigned short	CMailBase::m_httpsport = 8082;
	
BOOL   CMailBase::m_enableclientcacheck = FALSE;
string CMailBase::m_ca_crt_root = "/var/cert/ca.cer";
string CMailBase::m_ca_crt_server = "/var/cert/server.cer";
string CMailBase::m_ca_key_server = "/var/cert/server-key.pem";
string CMailBase::m_ca_crt_client = "/var/cert/client.cer";
string CMailBase::m_ca_key_client = "/var/cert/client-key.pem";

string CMailBase::m_ca_password = "";

string   CMailBase::m_krb5_ktname = "/etc/erisemail/krb5.keytab";

string CMailBase::m_db_host = "localhost";
unsigned short  CMailBase::m_db_port = 0;
string CMailBase::m_db_name = "erisemail_db";
string CMailBase::m_db_username = "root";
string CMailBase::m_db_password = "";
string CMailBase::m_db_sock_file = "/var/run/mysqld/mysqld.sock";

unsigned int CMailBase::m_db_max_conn = MAX_STORAGE_CONN;

unsigned int CMailBase::m_relaytasknum = 3;

BOOL   CMailBase::m_enablesmtphostnamecheck = FALSE;
unsigned int CMailBase::m_connect_num = 0;
unsigned int CMailBase::m_max_conn = MAX_STORAGE_CONN;
unsigned int CMailBase::m_runtime = 0;
string	CMailBase::m_config_file = CONFIG_FILE_PATH;
string	CMailBase::m_permit_list_file = PERMIT_FILE_PATH;
string	CMailBase::m_reject_list_file = REJECT_FILE_PATH;
string	CMailBase::m_domain_list_file = DOMAIN_FILE_PATH;
string	CMailBase::m_webadmin_list_file = WEBADMIN_FILE_PATH;

vector<stReject> CMailBase::m_reject_list;
vector<string> CMailBase::m_permit_list;
vector<string> CMailBase::m_domain_list;
vector<string> CMailBase::m_webadmin_list;

char CMailBase::m_des_key[9];

BOOL CMailBase::m_userpwd_cache_updated = TRUE;

CMailBase::CMailBase()
{

}

CMailBase::~CMailBase()
{
	UnLoadConfig();
}

void CMailBase::SetConfigFile(const char* config_file, const char* permit_list_file, const char* reject_list_file, const char* domain_list_file, const char* webadmin_list_file)
{
	m_config_file = config_file;
	m_permit_list_file = permit_list_file;
	m_reject_list_file = reject_list_file;

	m_domain_list_file = domain_list_file;

	m_webadmin_list_file = webadmin_list_file;
}

BOOL CMailBase::LoadConfig()
{	
    srandom(time(NULL)/3600);
    for(int x = 0; x < 8; x++)
    {
        int ind = random()%(sizeof(CHAR_TBL) - 1);
        
        m_des_key[x] = CHAR_TBL[ind];
    }
    m_des_key[8] = '\0';
	m_domain_list.clear();	
	m_permit_list.clear();
	m_reject_list.clear();
	m_webadmin_list.clear();
	
	ifstream configfilein(m_config_file.c_str(), ios_base::binary);
	string strline = "";
	if(!configfilein.is_open())
	{
		printf("%s is not exist.", m_config_file.c_str());
		return FALSE;
	}
	while(getline(configfilein, strline))
	{
		strtrim(strline);
		
		if(strline == "")
			continue;
	    
		if(strncasecmp(strline.c_str(), "#", strlen("#")) != 0)
		{
			if(strncasecmp(strline.c_str(), "Encoding", strlen("Encoding")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_encoding);
				strtrim(m_encoding);
				//printf("%s\n", m_encoding.c_str());
			}
			else if(strncasecmp(strline.c_str(), "PrivatePath", strlen("PrivatePath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_private_path);
				strtrim(m_private_path);
				//printf("%s\n", m_private_path.c_str());
			}
			else if(strncasecmp(strline.c_str(), "HTMLPath", strlen("HTMLPath")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_html_path);
				strtrim(m_html_path);
				//printf("%s\n", m_html_path.c_str());
			}
			else if(strncasecmp(strline.c_str(), "LocalHostName", strlen("LocalHostName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_localhostname );
				strtrim(m_localhostname);
				//printf("%s\n", m_localhostname.c_str());
			}
			else if(strncasecmp(strline.c_str(), "EmailDomainName", strlen("EmailDomainName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_email_domain );
				strtrim(m_email_domain);
				// printf("%s\n", m_email_domain.c_str());

				//add to admit list
				m_domain_list.push_back(m_email_domain);
				
			}
			else if(strncasecmp(strline.c_str(), "HostIP", strlen("HostIP")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hostip );
				strtrim(m_hostip);
				/* printf("[%s]\n", m_hostip.c_str()); */
			}
			else if(strncasecmp(strline.c_str(), "DNSServer", strlen("DNSServer")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_dns_server );
				strtrim(m_dns_server);
				/* printf("%s\n", m_dns_server.c_str()); */
			}
			else if(strncasecmp(strline.c_str(), "MaxConnPerProtocal", strlen("MaxConnPerProtocal")) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_conn= atoi(maxconn.c_str());
				//printf("%d\n", m_max_conn);
			}
			else if(strncasecmp(strline.c_str(), "SMTPPort", strlen("SMTPPort")) == 0)
			{
				string smtpport;
				strcut(strline.c_str(), "=", NULL, smtpport );
				strtrim(smtpport);
				m_smtpport = atoi(smtpport.c_str());
				//printf("%d\n", m_smtpport);
			}
			else if(strncasecmp(strline.c_str(), "EnableSMTPTLS", strlen("EnableSMTPTLS")) == 0)
			{
				string EnableSMTPTLS;
				strcut(strline.c_str(), "=", NULL, EnableSMTPTLS );
				strtrim(EnableSMTPTLS);
				m_enablesmtptls = (strcasecmp(EnableSMTPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "EnableRelay", strlen("EnableRelay")) == 0)
			{
				string EnableRelay;
				strcut(strline.c_str(), "=", NULL, EnableRelay );
				strtrim(EnableRelay);
				m_enablerelay = (strcasecmp(EnableRelay.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3Enable", strlen("POP3Enable")) == 0)
			{
				string POP3Enable;
				strcut(strline.c_str(), "=", NULL, POP3Enable );
				strtrim(POP3Enable);
				m_enablepop3= (strcasecmp(POP3Enable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3Port", strlen("POP3Port")) == 0)
			{
				string pop3port;
				strcut(strline.c_str(), "=", NULL, pop3port );
				strtrim(pop3port);
				m_pop3port = atoi(pop3port.c_str());
				//printf("%d\n", m_pop3port);
			}
			else if(strncasecmp(strline.c_str(), "EnablePOP3TLS", strlen("EnablePOP3TLS")) == 0)
			{
				string EnablePOP3TLS;
				strcut(strline.c_str(), "=", NULL, EnablePOP3TLS );
				strtrim(EnablePOP3TLS);
				m_enablepop3tls= (strcasecmp(EnablePOP3TLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPEnable", strlen("IMAPEnable")) == 0)
			{
				string IMAPEnable;
				strcut(strline.c_str(), "=", NULL, IMAPEnable );
				strtrim(IMAPEnable);
				m_enableimap= (strcasecmp(IMAPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPPort", strlen("IMAPPort")) == 0)
			{
				string imapport;
				strcut(strline.c_str(), "=", NULL, imapport );
				strtrim(imapport);
				m_imapport= atoi(imapport.c_str());
				//printf("%d\n", m_imapport);
			}
			else if(strncasecmp(strline.c_str(), "EnableIMAPTLS", strlen("EnableIMAPTLS")) == 0)
			{
				string EnableIMAPTLS;
				strcut(strline.c_str(), "=", NULL, EnableIMAPTLS );
				strtrim(EnableIMAPTLS);
				m_enableimaptls= (strcasecmp(EnableIMAPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "SMTPSEnable", strlen("SMTPSEnable")) == 0)
			{
				string SMTPSEnable;
				strcut(strline.c_str(), "=", NULL, SMTPSEnable );
				strtrim(SMTPSEnable);
				m_enablesmtps= (strcasecmp(SMTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "SMTPSPort", strlen("SMTPSPort")) == 0)
			{
				string smtpsport;
				strcut(strline.c_str(), "=", NULL, smtpsport );
				strtrim(smtpsport);
				m_smtpsport = atoi(smtpsport.c_str());
				//printf("%d\n", m_smtpsport);
			}
			else if(strncasecmp(strline.c_str(), "POP3SEnable", strlen("POP3SEnable")) == 0)
			{
				string POP3SEnable;
				strcut(strline.c_str(), "=", NULL, POP3SEnable );
				strtrim(POP3SEnable);
				m_enablepop3s= (strcasecmp(POP3SEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3SPort", strlen("POP3SPort")) == 0)
			{
				string pop3sport;
				strcut(strline.c_str(), "=", NULL, pop3sport );
				strtrim(pop3sport);
				m_pop3sport = atoi(pop3sport.c_str());
				//printf("%d\n", m_pop3sport);
			}
			else if(strncasecmp(strline.c_str(), "IMAPSEnable", strlen("IMAPSEnable")) == 0)
			{
				string IMAPSEnable;
				strcut(strline.c_str(), "=", NULL, IMAPSEnable );
				strtrim(IMAPSEnable);
				m_enableimaps= (strcasecmp(IMAPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPSPort", strlen("IMAPSPort")) == 0)
			{
				string imapsport;
				strcut(strline.c_str(), "=", NULL, imapsport );
				strtrim(imapsport);
				m_imapsport= atoi(imapsport.c_str());
				//printf("%d\n", m_imapsport);
			}
			else if(strncasecmp(strline.c_str(), "HTTPEnable", strlen("HTTPEnable")) == 0)
			{
				string HTTPEnable;
				strcut(strline.c_str(), "=", NULL, HTTPEnable );
				strtrim(HTTPEnable);
				m_enablehttp= (strcasecmp(HTTPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPPort", strlen("HTTPPort")) == 0)
			{
				string httpport;
				strcut(strline.c_str(), "=", NULL, httpport );
				strtrim(httpport);
				m_httpport = atoi(httpport.c_str());
				//printf("%d\n", m_httpport);
			}
			else if(strncasecmp(strline.c_str(), "HTTPSEnable", strlen("HTTPSEnable")) == 0)
			{
				string HTTPSEnable;
				strcut(strline.c_str(), "=", NULL, HTTPSEnable );
				strtrim(HTTPSEnable);
				m_enablehttps= (strcasecmp(HTTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPSPort", strlen("HTTPSPort")) == 0)
			{
				string httpsport;
				strcut(strline.c_str(), "=", NULL, httpsport );
				strtrim(httpsport);
				m_httpsport = atoi(httpsport.c_str());
				//printf("%d\n", m_httpsport);
			}
			else if(strncasecmp(strline.c_str(), "CheckClientCA", strlen("CheckClientCA")) == 0)
			{
				string CheckClientCA;
				strcut(strline.c_str(), "=", NULL, CheckClientCA );
				strtrim(CheckClientCA);
				m_enableclientcacheck = (strcasecmp(CheckClientCA.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "CARootCrt", strlen("CARootCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_root);
				strtrim(m_ca_crt_root);
				//printf("%s\n", m_ca_crt_root.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAServerCrt", strlen("CAServerCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_server);
				strtrim(m_ca_crt_server);
				//printf("%s\n", m_ca_crt_server.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAServerKey", strlen("CAServerKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_server);
				strtrim(m_ca_key_server);
				//printf("%s\n", m_ca_crt_server.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAClientCrt", strlen("CAClientCrt")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_client);
				strtrim(m_ca_crt_client);
				//printf("%s\n", m_ca_crt_client.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAClientKey", strlen("CAClientKey")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_client);
				strtrim(m_ca_key_client);
				//printf("%s\n", m_ca_crt_client.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CAPassword", strlen("CAPassword")) == 0)
			{
				m_ca_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_ca_password);
			}
#ifdef _WITH_GSSAPI_              
            else if(strncasecmp(strline.c_str(), "KRB5_KTNAME", strlen("KRB5_KTNAME")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_ktname);
				strtrim(m_krb5_ktname);
              
				setenv("KRB5_KTNAME", m_krb5_ktname.c_str(), 1);
			}
#endif /* _WITH_GSSAPI_ */            
			else if(strncasecmp(strline.c_str(), "DBHost", strlen("DBHost")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_host );
				strtrim(m_db_host);
				//printf("%s\n", m_db_host.c_str());
			}
            else if(strncasecmp(strline.c_str(), "DBPort", strlen("DBPort")) == 0)
			{
				string dbport;
				strcut(strline.c_str(), "=", NULL, dbport );
				strtrim(dbport);
				m_db_port = atoi(dbport.c_str());
				//printf("%d\n", m_db_port);
			}
			else if(strncasecmp(strline.c_str(), "DBName", strlen("DBName")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_name );
				strtrim(m_db_name);
				//printf("%s\n", m_db_name.c_str());
			}
			else if(strncasecmp(strline.c_str(), "DBUser", strlen("DBUser")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_username );
				strtrim(m_db_username);
				//printf("%s\n", m_db_username.c_str());
			}
			else if(strncasecmp(strline.c_str(), "DBPassword", strlen("DBPassword")) == 0)
			{
				m_db_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_db_password);
				
				//printf("%s\n", m_db_password.c_str());
				
			}
            else if(strncasecmp(strline.c_str(), "DBSockFile", strlen("DBSockFile")) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_sock_file );
				strtrim(m_db_sock_file);
				//printf("%s\n", m_db_sock_file.c_str());
			}
			else if(strncasecmp(strline.c_str(), "DBMaxConn", strlen("DBMaxConn")) == 0)
			{
				string DBMaxConn;
				strcut(strline.c_str(), "=", NULL, DBMaxConn );
				strtrim(DBMaxConn);
				m_db_max_conn = atoi(DBMaxConn.c_str());
				//printf("%d\n", m_db_max_conn);
			}
			else if(strncasecmp(strline.c_str(), "RelayTaskNum", strlen("RelayTaskNum")) == 0)
			{
				string relaytasknum;
				strcut(strline.c_str(), "=", NULL, relaytasknum );
				strtrim(relaytasknum);
				m_relaytasknum = atoi(relaytasknum.c_str());
			}
			else if(strncasecmp(strline.c_str(), "SMTPHostNameCheck", strlen("SMTPHostNameCheck")) == 0)
			{
				string SMTPHostNameCheck;
				strcut(strline.c_str(), "=", NULL, SMTPHostNameCheck );
				strtrim(SMTPHostNameCheck);
				m_enablesmtphostnamecheck = (strcasecmp(SMTPHostNameCheck.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "MEMCACHEDList", strlen("MEMCACHEDList")) == 0)
			{
				string memc_list;
				strcut(strline.c_str(), "=", NULL, memc_list );
				strtrim(memc_list);
				
				string memc_addr, memc_port_str;
				int memc_port;
				
				strcut(memc_list.c_str(), NULL, ":", memc_addr );
				strcut(memc_list.c_str(), ":", NULL, memc_port_str );
				
				strtrim(memc_addr);
				strtrim(memc_port_str);

				memc_port = atoi(memc_port_str.c_str());
				
				m_memcached_list.insert(make_pair<string, int>(memc_addr.c_str(), memc_port));
			}
			/* else
			{
				printf("%s\n", strline.c_str());
			} */
			strline = "";
		}
		
	}
	configfilein.close();
	
	ifstream permitfilein(m_permit_list_file.c_str(), ios_base::binary);
	if(!permitfilein.is_open())
	{
		printf("%s is not exist. please creat it", m_permit_list_file.c_str());
		return FALSE;
	}
	while(getline(permitfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
			m_permit_list.push_back(strline);
	}
	permitfilein.close();

	
	ifstream rejectfilein(m_reject_list_file.c_str(), ios_base::binary);
	if(!rejectfilein.is_open())
	{
		printf("%s is not exist. please creat it", m_reject_list_file.c_str());
		return FALSE;
	}
	while(getline(rejectfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
		{
			stReject sr;
			sr.ip = strline;
			sr.expire = 0xFFFFFFFF;
			m_reject_list.push_back(sr);
		}
	}
	rejectfilein.close();

	ifstream domainfilein(m_domain_list_file.c_str(), ios_base::binary);
	if(!domainfilein.is_open())
		return FALSE;
	while(getline(domainfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
			m_domain_list.push_back(strline);
	}
	domainfilein.close();
	
	ifstream webadminfilein(m_webadmin_list_file.c_str(), ios_base::binary);
	if(!webadminfilein.is_open())
		return FALSE;
	while(getline(webadminfilein, strline))
	{
		strtrim(strline);
		if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
			m_webadmin_list.push_back(strline);
	}
	webadminfilein.close();
	
	m_runtime = time(NULL);

	return TRUE;
}

BOOL CMailBase::UnLoadConfig()
{

}

BOOL CMailBase::LoadList()
{
	string strline;
	sem_t* plock = NULL;
	///////////////////////////////////////////////////////////////////////////////
	// GLOBAL_REJECT_LIST
	plock = sem_open("/.GLOBAL_REJECT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);

		m_permit_list.clear();
		ifstream permitfilein(m_permit_list_file.c_str(), ios_base::binary);
		if(!permitfilein.is_open())
		{
			printf("%s is not exist. please creat it", m_permit_list_file.c_str());
			return FALSE;
		}
		while(getline(permitfilein, strline))
		{
			strtrim(strline);
			if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
				m_permit_list.push_back(strline);
		}
		permitfilein.close();
		sem_post(plock);
		sem_close(plock);
	}
	/////////////////////////////////////////////////////////////////////////////////
	// GLOBAL_PERMIT_LIST
	plock = sem_open("/.GLOBAL_PERMIT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);

		m_reject_list.clear();
		ifstream rejectfilein(m_reject_list_file.c_str(), ios_base::binary);
		if(!rejectfilein.is_open())
		{
			printf("%s is not exist. please creat it", m_reject_list_file.c_str());
			return FALSE;
		}
		while(getline(rejectfilein, strline))
		{
			strtrim(strline);
			if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
			{
				stReject sr;
				sr.ip = strline;
				sr.expire = 0xFFFFFFFF;
				m_reject_list.push_back(sr);
			}
		}
		rejectfilein.close();
		sem_post(plock);
		sem_close(plock);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// DOMAIN_PERMIT_LIST
	plock = sem_open("/.DOMAIN_PERMIT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);
		m_domain_list.clear();
		m_domain_list.push_back(m_email_domain);
		ifstream domainfilein(m_domain_list_file.c_str(), ios_base::binary);
		if(!domainfilein.is_open())
			return FALSE;
		while(getline(domainfilein, strline))
		{
			strtrim(strline);
			if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
				m_domain_list.push_back(strline);
		}
		domainfilein.close();
		sem_post(plock);
		sem_close(plock);
	}
	////////////////////////////////////////////////////////////////////////////////////////
	// WEBADMIN_PERMIT_LIST
	plock = sem_open("/.WEBADMIN_PERMIT_LIST.sem", O_CREAT | O_RDWR, 0644, 1);
	if(plock != SEM_FAILED)
	{
		sem_wait(plock);
		m_webadmin_list.clear();
		ifstream webadminfilein(m_webadmin_list_file.c_str(), ios_base::binary);
		if(!webadminfilein.is_open())
			return FALSE;
		while(getline(webadminfilein, strline))
		{
			strtrim(strline);
			if((strline != "")&&(strncmp(strline.c_str(), "#", 1) != 0))
				m_webadmin_list.push_back(strline);
		}
		webadminfilein.close();

		sem_post(plock);
		sem_close(plock);
	}
}

BOOL CMailBase::UnLoadList()
{

}

BOOL CMailBase::Is_Local_Domain(const char* domain)
{
	for(int i = 0; i < m_domain_list.size(); i++)
	{
		if(strcasecmp(m_domain_list[i].c_str(), domain) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}



