/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "base.h"
#include "util/security.h"
#include "posixname.h"

#define ERISEMAIL_VERSION "1.6.09"

static char CHAR_TBL[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=~!@#$%^&*()_+[]\\{}|;':\",./<>?";

//////////////////////////////////////////////////////////////////////////
//CMailBase
//
//Software Version
string CMailBase::m_sw_version = ERISEMAIL_VERSION;

//Global
BOOL CMailBase::m_close_stderr = TRUE;
string CMailBase::m_encoding = "UTF-8";

string CMailBase::m_private_path = "/var/erisemail/private";
string CMailBase::m_html_path = "/var/erisemail/html";


string CMailBase::m_localhostname = "mail";
string CMailBase::m_email_domain = "erisesoft.com";
string CMailBase::m_dns_server;
string CMailBase::m_hostip;

map<string, int> CMailBase::m_memcached_list;

BOOL CMailBase::m_enablesmtp = TRUE;
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

BOOL		CMailBase::m_enablexmpp = TRUE;
unsigned short	CMailBase::m_xmppport = 5222;
unsigned int	CMailBase::m_encryptxmpp = 2; /* 0: Non-encrypt or TLS optional; 1: TLS required; 2: SSL-based */

BOOL   CMailBase::m_enableclientcacheck = FALSE;
string CMailBase::m_ca_crt_root = "/var/erisemail/cert/ca.crt";
string CMailBase::m_ca_crt_server = "/var/erisemail/cert/server.crt";
string CMailBase::m_ca_key_server = "/var/erisemail/cert/server.key";
string CMailBase::m_ca_crt_client = "/var/erisemail/cert/client.crt";
string CMailBase::m_ca_key_client = "/var/erisemail/cert/client.key";

string CMailBase::m_ca_password = "";

string   CMailBase::m_krb5_ktname = "/etc/erisemail/krb5.keytab";

string CMailBase::m_db_host = "localhost";
unsigned short  CMailBase::m_db_port = 3306;
string CMailBase::m_db_name = "erisemail_db";
string CMailBase::m_db_username = "root";
string CMailBase::m_db_password = "";
string CMailBase::m_db_sock_file = "/var/run/mysqld/mysqld.sock";

unsigned int CMailBase::m_db_max_conn = MAX_STORAGE_CONN;

BOOL CMailBase::m_enablemta = TRUE;
unsigned int CMailBase::m_mta_relaytasknum = 3;
unsigned int CMailBase::m_mta_relaycheckinterval = 5;

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
	    
		if(strncasecmp(strline.c_str(), "#", sizeof("#") - 1) != 0)
		{
			if(strncasecmp(strline.c_str(), "Encoding", sizeof("Encoding") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_encoding);
				strtrim(m_encoding);
			}
            else if(strncasecmp(strline.c_str(), "CloseStderr", sizeof("CloseStderr") - 1) == 0)
			{
				string close_stderr;
				strcut(strline.c_str(), "=", NULL, close_stderr );
				strtrim(close_stderr);
				m_close_stderr = (strcasecmp(close_stderr.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "PrivatePath", sizeof("PrivatePath") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_private_path);
				strtrim(m_private_path);
			}
			else if(strncasecmp(strline.c_str(), "HTMLPath", sizeof("HTMLPath") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_html_path);
				strtrim(m_html_path);
			}
			else if(strncasecmp(strline.c_str(), "LocalHostName", sizeof("LocalHostName") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_localhostname );
				strtrim(m_localhostname);
			}
			else if(strncasecmp(strline.c_str(), "EmailDomainName", sizeof("EmailDomainName") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_email_domain );
				strtrim(m_email_domain);

				//add to admit list
				m_domain_list.push_back(m_email_domain);
				
			}
			else if(strncasecmp(strline.c_str(), "HostIP", sizeof("HostIP") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hostip );
				strtrim(m_hostip);
			}
			else if(strncasecmp(strline.c_str(), "DNSServer", sizeof("DNSServer") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_dns_server );
				strtrim(m_dns_server);
			}
			else if(strncasecmp(strline.c_str(), "MaxCocurrentConnNum", sizeof("MaxCocurrentConnNum") - 1) == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_max_conn= atoi(maxconn.c_str());
			}
            else if(strncasecmp(strline.c_str(), "SMTPEnable", sizeof("SMTPEnable") - 1) == 0)
			{
				string enable_smtp;
				strcut(strline.c_str(), "=", NULL, enable_smtp );
				strtrim(enable_smtp);
				m_enablesmtp = (strcasecmp(enable_smtp.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "SMTPPort", sizeof("SMTPPort") - 1) == 0)
			{
				string smtpport;
				strcut(strline.c_str(), "=", NULL, smtpport );
				strtrim(smtpport);
				m_smtpport = atoi(smtpport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "EnableSMTPTLS", sizeof("EnableSMTPTLS") - 1) == 0)
			{
				string EnableSMTPTLS;
				strcut(strline.c_str(), "=", NULL, EnableSMTPTLS );
				strtrim(EnableSMTPTLS);
				m_enablesmtptls = (strcasecmp(EnableSMTPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "EnableRelay", sizeof("EnableRelay") - 1) == 0)
			{
				string EnableRelay;
				strcut(strline.c_str(), "=", NULL, EnableRelay );
				strtrim(EnableRelay);
				m_enablerelay = (strcasecmp(EnableRelay.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3Enable", sizeof("POP3Enable") - 1) == 0)
			{
				string POP3Enable;
				strcut(strline.c_str(), "=", NULL, POP3Enable );
				strtrim(POP3Enable);
				m_enablepop3= (strcasecmp(POP3Enable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3Port", sizeof("POP3Port") - 1) == 0)
			{
				string pop3port;
				strcut(strline.c_str(), "=", NULL, pop3port );
				strtrim(pop3port);
				m_pop3port = atoi(pop3port.c_str());
			}
			else if(strncasecmp(strline.c_str(), "EnablePOP3TLS", sizeof("EnablePOP3TLS") - 1) == 0)
			{
				string EnablePOP3TLS;
				strcut(strline.c_str(), "=", NULL, EnablePOP3TLS );
				strtrim(EnablePOP3TLS);
				m_enablepop3tls= (strcasecmp(EnablePOP3TLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPEnable", sizeof("IMAPEnable") - 1) == 0)
			{
				string IMAPEnable;
				strcut(strline.c_str(), "=", NULL, IMAPEnable );
				strtrim(IMAPEnable);
				m_enableimap= (strcasecmp(IMAPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPPort", sizeof("IMAPPort") - 1) == 0)
			{
				string imapport;
				strcut(strline.c_str(), "=", NULL, imapport );
				strtrim(imapport);
				m_imapport= atoi(imapport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "EnableIMAPTLS", sizeof("EnableIMAPTLS") - 1) == 0)
			{
				string EnableIMAPTLS;
				strcut(strline.c_str(), "=", NULL, EnableIMAPTLS );
				strtrim(EnableIMAPTLS);
				m_enableimaptls= (strcasecmp(EnableIMAPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "SMTPSEnable", sizeof("SMTPSEnable") - 1) == 0)
			{
				string SMTPSEnable;
				strcut(strline.c_str(), "=", NULL, SMTPSEnable );
				strtrim(SMTPSEnable);
				m_enablesmtps= (strcasecmp(SMTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "SMTPSPort", sizeof("SMTPSPort") - 1) == 0)
			{
				string smtpsport;
				strcut(strline.c_str(), "=", NULL, smtpsport );
				strtrim(smtpsport);
				m_smtpsport = atoi(smtpsport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "POP3SEnable", sizeof("POP3SEnable") - 1) == 0)
			{
				string POP3SEnable;
				strcut(strline.c_str(), "=", NULL, POP3SEnable );
				strtrim(POP3SEnable);
				m_enablepop3s= (strcasecmp(POP3SEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "POP3SPort", sizeof("POP3SPort") - 1) == 0)
			{
				string pop3sport;
				strcut(strline.c_str(), "=", NULL, pop3sport );
				strtrim(pop3sport);
				m_pop3sport = atoi(pop3sport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "IMAPSEnable", sizeof("IMAPSEnable") - 1) == 0)
			{
				string IMAPSEnable;
				strcut(strline.c_str(), "=", NULL, IMAPSEnable );
				strtrim(IMAPSEnable);
				m_enableimaps= (strcasecmp(IMAPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "IMAPSPort", sizeof("IMAPSPort") - 1) == 0)
			{
				string imapsport;
				strcut(strline.c_str(), "=", NULL, imapsport );
				strtrim(imapsport);
				m_imapsport= atoi(imapsport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "HTTPEnable", sizeof("HTTPEnable") - 1) == 0)
			{
				string HTTPEnable;
				strcut(strline.c_str(), "=", NULL, HTTPEnable );
				strtrim(HTTPEnable);
				m_enablehttp= (strcasecmp(HTTPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPPort", sizeof("HTTPPort") - 1) == 0)
			{
				string httpport;
				strcut(strline.c_str(), "=", NULL, httpport );
				strtrim(httpport);
				m_httpport = atoi(httpport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "HTTPSEnable", sizeof("HTTPSEnable") - 1) == 0)
			{
				string HTTPSEnable;
				strcut(strline.c_str(), "=", NULL, HTTPSEnable );
				strtrim(HTTPSEnable);
				m_enablehttps= (strcasecmp(HTTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "HTTPSPort", sizeof("HTTPSPort") - 1) == 0)
			{
				string httpsport;
				strcut(strline.c_str(), "=", NULL, httpsport );
				strtrim(httpsport);
				m_httpsport = atoi(httpsport.c_str());
			}
            else if(strncasecmp(strline.c_str(), "XMPPEnable", sizeof("XMPPEnable") - 1) == 0)
			{
				string XMPPEnable;
				strcut(strline.c_str(), "=", NULL, XMPPEnable );
				strtrim(XMPPEnable);
				m_enablexmpp= (strcasecmp(XMPPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "XMPPPort", sizeof("XMPPPort") - 1) == 0)
			{
				string xmppport;
				strcut(strline.c_str(), "=", NULL, xmppport );
				strtrim(xmppport);
				m_xmppport = atoi(xmppport.c_str());
			}
            else if(strncasecmp(strline.c_str(), "EncryptXMPP", sizeof("EncryptXMPP") - 1) == 0)
			{
				string encryptxmpp;
				strcut(strline.c_str(), "=", NULL, encryptxmpp );
				strtrim(encryptxmpp);
				m_encryptxmpp = atoi(encryptxmpp.c_str());
			}
			else if(strncasecmp(strline.c_str(), "CheckClientCA", sizeof("CheckClientCA") - 1) == 0)
			{
				string CheckClientCA;
				strcut(strline.c_str(), "=", NULL, CheckClientCA );
				strtrim(CheckClientCA);
				m_enableclientcacheck = (strcasecmp(CheckClientCA.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "CARootCrt", sizeof("CARootCrt") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_root);
				strtrim(m_ca_crt_root);
			}
			else if(strncasecmp(strline.c_str(), "CAServerCrt", sizeof("CAServerCrt") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_server);
				strtrim(m_ca_crt_server);
			}
			else if(strncasecmp(strline.c_str(), "CAServerKey", sizeof("CAServerKey") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_server);
				strtrim(m_ca_key_server);
			}
			else if(strncasecmp(strline.c_str(), "CAClientCrt", sizeof("CAClientCrt") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_client);
				strtrim(m_ca_crt_client);
			}
			else if(strncasecmp(strline.c_str(), "CAClientKey", sizeof("CAClientKey") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_client);
				strtrim(m_ca_key_client);
			}
			else if(strncasecmp(strline.c_str(), "CAPassword", sizeof("CAPassword") - 1) == 0)
			{
				m_ca_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_ca_password);
			}
#ifdef _WITH_GSSAPI_              
            else if(strncasecmp(strline.c_str(), "KRB5_KTNAME", sizeof("KRB5_KTNAME") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_ktname);
				strtrim(m_krb5_ktname);
              
				setenv("KRB5_KTNAME", m_krb5_ktname.c_str(), 1);
			}
#endif /* _WITH_GSSAPI_ */            
			else if(strncasecmp(strline.c_str(), "DBHost", sizeof("DBHost") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_host );
				strtrim(m_db_host);
			}
            else if(strncasecmp(strline.c_str(), "DBPort", sizeof("DBPort") - 1) == 0)
			{
				string dbport;
				strcut(strline.c_str(), "=", NULL, dbport );
				strtrim(dbport);
				m_db_port = atoi(dbport.c_str());
			}
			else if(strncasecmp(strline.c_str(), "DBName", sizeof("DBName") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_name );
				strtrim(m_db_name);
			}
			else if(strncasecmp(strline.c_str(), "DBUser", sizeof("DBUser") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_username );
				strtrim(m_db_username);
			}
			else if(strncasecmp(strline.c_str(), "DBPassword", sizeof("DBPassword") - 1) == 0)
			{
				m_db_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_db_password);
				
			}
            else if(strncasecmp(strline.c_str(), "DBSockFile", sizeof("DBSockFile") - 1) == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_sock_file );
				strtrim(m_db_sock_file);
			}
			else if(strncasecmp(strline.c_str(), "DBMaxConn", sizeof("DBMaxConn") - 1) == 0)
			{
				string DBMaxConn;
				strcut(strline.c_str(), "=", NULL, DBMaxConn );
				strtrim(DBMaxConn);
				m_db_max_conn = atoi(DBMaxConn.c_str());
			}
            else if(strncasecmp(strline.c_str(), "MTAEnable", sizeof("MTAEnable") - 1) == 0)
			{
				string enable_mta;
				strcut(strline.c_str(), "=", NULL, enable_mta );
				strtrim(enable_mta);
				m_enablemta = (strcasecmp(enable_mta.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "MTARelayTaskNum", sizeof("MTARelayTaskNum") - 1) == 0)
			{
				string mta_relaytasknum;
				strcut(strline.c_str(), "=", NULL, mta_relaytasknum );
				strtrim(mta_relaytasknum);
				m_mta_relaytasknum = atoi(mta_relaytasknum.c_str());
			}
			else if(strncasecmp(strline.c_str(), "MTARelayCheckInterval", sizeof("MTARelayCheckInterval") - 1) == 0)
			{
				string mta_relaycheckinterval;
				strcut(strline.c_str(), "=", NULL, mta_relaycheckinterval );
				strtrim(mta_relaycheckinterval);
				m_mta_relaycheckinterval = atoi(mta_relaycheckinterval.c_str());
                if(m_mta_relaycheckinterval <= 0)
                    m_mta_relaycheckinterval = 5;
			}
			else if(strncasecmp(strline.c_str(), "SMTPHostNameCheck", sizeof("SMTPHostNameCheck") - 1) == 0)
			{
				string SMTPHostNameCheck;
				strcut(strline.c_str(), "=", NULL, SMTPHostNameCheck );
				strtrim(SMTPHostNameCheck);
				m_enablesmtphostnamecheck = (strcasecmp(SMTPHostNameCheck.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strncasecmp(strline.c_str(), "MEMCACHEDList", sizeof("MEMCACHEDList") - 1) == 0)
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
			sr.expire = 0xFFFFFFFFU;
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
	plock = sem_open(ERISEMAIL_GLOBAL_REJECT_LIST, O_CREAT | O_RDWR, 0644, 1);
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
	plock = sem_open(ERISEMAIL_GLOBAL_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
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
				sr.expire = 0xFFFFFFFFU;
				m_reject_list.push_back(sr);
			}
		}
		rejectfilein.close();
		sem_post(plock);
		sem_close(plock);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// DOMAIN_PERMIT_LIST
	plock = sem_open(ERISEMAIL_DOMAIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
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
	plock = sem_open(ERISEMAIL_WEBADMIN_PERMIT_LIST, O_CREAT | O_RDWR, 0644, 1);
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



