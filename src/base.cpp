/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "base.h"
#include "util/security.h"
#include "posixname.h"

#define ERISEMAIL_VERSION "1.6.10"

static char CHAR_TBL[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=~!@#$%^&*()_+[]\\{}|;':\",./<>?";

//////////////////////////////////////////////////////////////////////////
//CMailBase
//
//Software Version
string CMailBase::m_sw_version = ERISEMAIL_VERSION;

//Global
BOOL CMailBase::m_prod_type = PROD_EMAIL;

BOOL CMailBase::m_close_stderr = FALSE;
BOOL CMailBase::m_smtp_client_trace = FALSE;

string CMailBase::m_encoding = "UTF-8";
string CMailBase::m_master_hostname = "";

#ifdef _WITH_HDFS_
    string CMailBase::m_hdfs_host = "hdfs://localhost";
    unsigned short CMailBase::m_hdfs_port = 9000;
    string CMailBase::m_hdfs_path = "/erisemail/eml";
    string   CMailBase::m_hdfs_user = "_huser";
    string   CMailBase::m_java_home = "/usr/lib/jvm/default-java";
    string   CMailBase::m_hadoop_home = "/usr/local/hadoop";
#endif /* _WITH_HDFS_ */

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
unsigned short	CMailBase::m_xmppsport = 5223;
BOOL     CMailBase::m_enablexmppfederation = TRUE;
unsigned short	CMailBase::m_xmpps2sport = 5269;
unsigned int	CMailBase::m_encryptxmpp = XMPP_TLS_OPTIONAL; /* 0: Non-encrypted or TLS optional; 1: TLS required; 2: Old-SSL-based */
unsigned int	CMailBase::m_xmpp_worker_thread_num = 8;
string CMailBase::m_xmpp_federation_secret = "s3cr3tf0rd14lb4ck";

BOOL		CMailBase::m_enable_local_ldap = TRUE;
unsigned short	CMailBase::m_local_ldapport = 8389;
unsigned short	CMailBase::m_local_ldapsport = 8636;
BOOL	CMailBase::m_encrypt_local_ldap = FALSE;

BOOL   CMailBase::m_ca_verify_client = FALSE;
string CMailBase::m_ca_crt_root = "/var/erisemail/cert/ca.crt";
string CMailBase::m_ca_crt_server = "/var/erisemail/cert/server.crt";
string CMailBase::m_ca_key_server = "/var/erisemail/cert/server.key";
string CMailBase::m_ca_crt_client = "/var/erisemail/cert/client.crt";
string CMailBase::m_ca_key_client = "/var/erisemail/cert/client.key";
string CMailBase::m_ca_client_base_dir = "/var/erisemail/cert/client";

string CMailBase::m_ca_password = "";

#ifdef _WITH_GSSAPI_ 
    string   CMailBase::m_krb5_ktname = "/etc/erisemail/krb5.keytab";
    string   CMailBase::m_krb5_realm = "ERISESOFT.COM";
    string   CMailBase::m_krb5_hostname = "localhost";
    string   CMailBase::m_krb5_smtp_service_name = "smtp";
    string   CMailBase::m_krb5_pop3_service_name = "pop";
    string   CMailBase::m_krb5_imap_service_name = "imap";
    string   CMailBase::m_krb5_http_service_name = "http";
    string   CMailBase::m_krb5_xmpp_service_name = "xmpp";
#endif /* _WITH_GSSAPI_ */
string CMailBase::m_db_host = "localhost";
unsigned short  CMailBase::m_db_port = 3306;
string CMailBase::m_db_name = "erisemail_db";
string CMailBase::m_db_username = "root";
string CMailBase::m_db_password = "";
string CMailBase::m_db_sock_file = "/var/run/mysqld/mysqld.sock";

BOOL   CMailBase::m_is_master = FALSE;
string CMailBase::m_master_db_host = "localhost";
unsigned short  CMailBase::m_master_db_port = 3306;
string CMailBase::m_master_db_name = "erisemail_db";
string CMailBase::m_master_db_username = "root";
string CMailBase::m_master_db_password = "";
string CMailBase::m_master_db_sock_file = "/var/run/mysqld/mysqld.sock";

#ifdef _WITH_LDAP_
	string	CMailBase::m_ldap_sever_uri = "ldap://localhost";
	int		CMailBase::m_ldap_sever_version = 3;;
	string	CMailBase::m_ldap_manager = "cn=admin,dc=erisesoft,dc=com";
	string	CMailBase::m_ldap_manager_passwd = "dev";
	string	CMailBase::m_ldap_search_base = "dc=erisesoft,dc=com";
	string	CMailBase::m_ldap_search_filter_user = "(&(objectClass=posixAccount)(uid=%s))";
    string	CMailBase::m_ldap_search_attribute_user_id = "uid";
	string	CMailBase::m_ldap_search_attribute_user_password = "userPassword";
    string	CMailBase::m_ldap_search_attribute_user_alias = "cn";
	string	CMailBase::m_ldap_search_filter_group = "(&(objectClass=posixGroup)(cn=%s))";
	string	CMailBase::m_ldap_search_attribute_group_member = "memberUid";
    string	CMailBase::m_ldap_user_dn = "cn=%s,dc=erisesoft,dc=com";
#endif /* _WITH_LDAP_ */

unsigned int CMailBase::m_db_max_conn = MAX_STORAGE_CONN;

BOOL CMailBase::m_enablemta = TRUE;
unsigned int CMailBase::m_mta_relaythreadnum = 3;
unsigned int CMailBase::m_mta_relaycheckinterval = 5;

BOOL   CMailBase::m_enablesmtphostnamecheck = FALSE;
unsigned int CMailBase::m_connect_num = 0;
unsigned int CMailBase::m_mda_max_conn = 4096;
unsigned int CMailBase::m_runtime = 0;
string	CMailBase::m_config_file = CONFIG_FILE_PATH;
string	CMailBase::m_permit_list_file = PERMIT_FILE_PATH;
string	CMailBase::m_reject_list_file = REJECT_FILE_PATH;
string	CMailBase::m_domain_list_file = DOMAIN_FILE_PATH;
string	CMailBase::m_webadmin_list_file = WEBADMIN_FILE_PATH;
string	CMailBase::m_clusters_list_file = CLUSTERS_FILE_PATH;

vector<stReject> CMailBase::m_reject_list;
vector<string> CMailBase::m_permit_list;
vector<string> CMailBase::m_domain_list;
vector<string> CMailBase::m_webadmin_list;


unsigned int	CMailBase::m_connection_idle_timeout = 20;
unsigned int    CMailBase::m_connection_sync_timeout = 3;
unsigned int    CMailBase::m_max_service_request_num = 65536;

unsigned int    CMailBase::m_service_idle_timeout = 7200;

unsigned int	CMailBase::m_thread_increase_step = 8;

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
	    
		if(strncasecmp(strline.c_str(), "#", 1) != 0)
		{
            string strKey;
            strcut(strline.c_str(), NULL, "=", strKey);
            strtrim(strKey);
			if(strcasecmp(strKey.c_str(), "Encoding") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_encoding);
				strtrim(m_encoding);
			}
#ifdef _WITH_DIST_			
			else if(strcasecmp(strKey.c_str(), "ClusterMasterHostname") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_master_hostname);
				strtrim(m_master_hostname);
			}
#endif /* _WITH_DIST_ */
            else if(strcasecmp(strKey.c_str(), "CloseStderr") == 0)
			{
				string close_stderr;
				strcut(strline.c_str(), "=", NULL, close_stderr );
				strtrim(close_stderr);
				m_close_stderr = (strcasecmp(close_stderr.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strcasecmp(strKey.c_str(), "SmtpClientTrace") == 0)
			{
				string smtp_client_trace;
				strcut(strline.c_str(), "=", NULL, smtp_client_trace );
				strtrim(smtp_client_trace);
				m_smtp_client_trace = (strcasecmp(smtp_client_trace.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
#ifdef _WITH_HDFS_
            else if(strcasecmp(strKey.c_str(), "HDFSHost") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hdfs_host);
				strtrim(m_hdfs_host);
			}
            else if(strcasecmp(strKey.c_str(), "HDFSPort") == 0)
			{
				string hdfs_port;
				strcut(strline.c_str(), "=", NULL, hdfs_port);
				strtrim(hdfs_port);
				m_hdfs_port = atoi(hdfs_port.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "HDFSUser") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hdfs_user);
				strtrim(m_hdfs_user);
			}
            else if(strcasecmp(strKey.c_str(), "HDFSPath") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hdfs_path);
				strtrim(m_hdfs_path);
			}
#endif /* _WITH_HDFS_ */
			else if(strcasecmp(strKey.c_str(), "PrivatePath") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_private_path);
				strtrim(m_private_path);
			}
			else if(strcasecmp(strKey.c_str(), "HTMLPath") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_html_path);
				strtrim(m_html_path);
			}
			else if(strcasecmp(strKey.c_str(), "LocalHostName") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_localhostname );
				strtrim(m_localhostname);
			}
			else if(strcasecmp(strKey.c_str(), "EmailDomainName") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_email_domain );
				strtrim(m_email_domain);

				//add to admit list
				m_domain_list.push_back(m_email_domain);
				
			}
			else if(strcasecmp(strKey.c_str(), "HostIP") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_hostip );
				strtrim(m_hostip);
			}
			else if(strcasecmp(strKey.c_str(), "DNSServer") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_dns_server );
				strtrim(m_dns_server);
			}
			else if(strcasecmp(strKey.c_str(), "MDAMaxCocurrentConnNum") == 0)
			{
				string maxconn;
				strcut(strline.c_str(), "=", NULL, maxconn );
				strtrim(maxconn);
				m_mda_max_conn= atoi(maxconn.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "SMTPEnable") == 0)
			{
				string enable_smtp;
				strcut(strline.c_str(), "=", NULL, enable_smtp );
				strtrim(enable_smtp);
				m_enablesmtp = (strcasecmp(enable_smtp.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "SMTPPort") == 0)
			{
				string smtpport;
				strcut(strline.c_str(), "=", NULL, smtpport );
				strtrim(smtpport);
				m_smtpport = atoi(smtpport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "EnableSMTPTLS") == 0)
			{
				string EnableSMTPTLS;
				strcut(strline.c_str(), "=", NULL, EnableSMTPTLS );
				strtrim(EnableSMTPTLS);
				m_enablesmtptls = (strcasecmp(EnableSMTPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "EnableRelay") == 0)
			{
				string EnableRelay;
				strcut(strline.c_str(), "=", NULL, EnableRelay );
				strtrim(EnableRelay);
				m_enablerelay = (strcasecmp(EnableRelay.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "POP3Enable") == 0)
			{
				string POP3Enable;
				strcut(strline.c_str(), "=", NULL, POP3Enable );
				strtrim(POP3Enable);
				m_enablepop3= (strcasecmp(POP3Enable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "POP3Port") == 0)
			{
				string pop3port;
				strcut(strline.c_str(), "=", NULL, pop3port );
				strtrim(pop3port);
				m_pop3port = atoi(pop3port.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "EnablePOP3TLS") == 0)
			{
				string EnablePOP3TLS;
				strcut(strline.c_str(), "=", NULL, EnablePOP3TLS );
				strtrim(EnablePOP3TLS);
				m_enablepop3tls= (strcasecmp(EnablePOP3TLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "IMAPEnable") == 0)
			{
				string IMAPEnable;
				strcut(strline.c_str(), "=", NULL, IMAPEnable );
				strtrim(IMAPEnable);
				m_enableimap= (strcasecmp(IMAPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "IMAPPort") == 0)
			{
				string imapport;
				strcut(strline.c_str(), "=", NULL, imapport );
				strtrim(imapport);
				m_imapport= atoi(imapport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "EnableIMAPTLS") == 0)
			{
				string EnableIMAPTLS;
				strcut(strline.c_str(), "=", NULL, EnableIMAPTLS );
				strtrim(EnableIMAPTLS);
				m_enableimaptls= (strcasecmp(EnableIMAPTLS.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "SMTPSEnable") == 0)
			{
				string SMTPSEnable;
				strcut(strline.c_str(), "=", NULL, SMTPSEnable );
				strtrim(SMTPSEnable);
				m_enablesmtps= (strcasecmp(SMTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "SMTPSPort") == 0)
			{
				string smtpsport;
				strcut(strline.c_str(), "=", NULL, smtpsport );
				strtrim(smtpsport);
				m_smtpsport = atoi(smtpsport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "POP3SEnable") == 0)
			{
				string POP3SEnable;
				strcut(strline.c_str(), "=", NULL, POP3SEnable );
				strtrim(POP3SEnable);
				m_enablepop3s= (strcasecmp(POP3SEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "POP3SPort") == 0)
			{
				string pop3sport;
				strcut(strline.c_str(), "=", NULL, pop3sport );
				strtrim(pop3sport);
				m_pop3sport = atoi(pop3sport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "IMAPSEnable") == 0)
			{
				string IMAPSEnable;
				strcut(strline.c_str(), "=", NULL, IMAPSEnable );
				strtrim(IMAPSEnable);
				m_enableimaps= (strcasecmp(IMAPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "IMAPSPort") == 0)
			{
				string imapsport;
				strcut(strline.c_str(), "=", NULL, imapsport );
				strtrim(imapsport);
				m_imapsport= atoi(imapsport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "HTTPEnable") == 0)
			{
				string HTTPEnable;
				strcut(strline.c_str(), "=", NULL, HTTPEnable );
				strtrim(HTTPEnable);
				m_enablehttp= (strcasecmp(HTTPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "HTTPPort") == 0)
			{
				string httpport;
				strcut(strline.c_str(), "=", NULL, httpport );
				strtrim(httpport);
				m_httpport = atoi(httpport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "HTTPSEnable") == 0)
			{
				string HTTPSEnable;
				strcut(strline.c_str(), "=", NULL, HTTPSEnable );
				strtrim(HTTPSEnable);
				m_enablehttps= (strcasecmp(HTTPSEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "HTTPSPort") == 0)
			{
				string httpsport;
				strcut(strline.c_str(), "=", NULL, httpsport );
				strtrim(httpsport);
				m_httpsport = atoi(httpsport.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "XMPPEnable") == 0)
			{
				string XMPPEnable;
				strcut(strline.c_str(), "=", NULL, XMPPEnable );
				strtrim(XMPPEnable);
				m_enablexmpp= (strcasecmp(XMPPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "XMPPPort") == 0)
			{
				string xmppport;
				strcut(strline.c_str(), "=", NULL, xmppport );
				strtrim(xmppport);
				m_xmppport = atoi(xmppport.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "XMPPSPort") == 0)
			{
				string xmppsport;
				strcut(strline.c_str(), "=", NULL, xmppsport );
				strtrim(xmppsport);
				m_xmppsport = atoi(xmppsport.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "XMPPFederationEnable") == 0)
			{
				string XMPPFederationEnable;
				strcut(strline.c_str(), "=", NULL, XMPPFederationEnable );
				strtrim(XMPPFederationEnable);
				m_enablexmppfederation= (strcasecmp(XMPPFederationEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strcasecmp(strKey.c_str(), "XMPPSrvToSrvPort") == 0)
			{
				string xmpps2sport;
				strcut(strline.c_str(), "=", NULL, xmpps2sport );
				strtrim(xmpps2sport);
				m_xmpps2sport = atoi(xmpps2sport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "XMPPFederationSecret") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_xmpp_federation_secret);
				strtrim(m_xmpp_federation_secret);
			}
            else if(strcasecmp(strKey.c_str(), "EncryptXMPP") == 0)
			{
				string encryptxmpp;
				strcut(strline.c_str(), "=", NULL, encryptxmpp );
				strtrim(encryptxmpp);
				m_encryptxmpp = atoi(encryptxmpp.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "LocalLDAPEnable") == 0)
			{
				string LDAPEnable;
				strcut(strline.c_str(), "=", NULL, LDAPEnable );
				strtrim(LDAPEnable);
				m_enable_local_ldap= (strcasecmp(LDAPEnable.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "LocalLDAPPort") == 0)
			{
				string ldapport;
				strcut(strline.c_str(), "=", NULL, ldapport );
				strtrim(ldapport);
				m_local_ldapport = atoi(ldapport.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "LocalLDAPSPort") == 0)
			{
				string ldapsport;
				strcut(strline.c_str(), "=", NULL, ldapsport );
				strtrim(ldapsport);
				m_local_ldapsport = atoi(ldapsport.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "EncryptLocalLDAP") == 0)
			{
				string encryptldap;
				strcut(strline.c_str(), "=", NULL, encryptldap );
				strtrim(encryptldap);
                
				m_encrypt_local_ldap = (strcasecmp(encryptldap.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strcasecmp(strKey.c_str(), "XMPPWorkerThreadNum") == 0)
			{
				string XMPPWorkerThreadNum;
				strcut(strline.c_str(), "=", NULL, XMPPWorkerThreadNum );
				strtrim(XMPPWorkerThreadNum);
				m_xmpp_worker_thread_num = atoi(XMPPWorkerThreadNum.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "CAVerifyClient") == 0)
			{
				string ca_verify_client;
				strcut(strline.c_str(), "=", NULL, ca_verify_client);
				strtrim(ca_verify_client);
				m_ca_verify_client = (strcasecmp(ca_verify_client.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "CARootCrt") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_root);
				strtrim(m_ca_crt_root);
			}
			else if(strcasecmp(strKey.c_str(), "CAServerCrt") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_server);
				strtrim(m_ca_crt_server);
			}
			else if(strcasecmp(strKey.c_str(), "CAServerKey") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_server);
				strtrim(m_ca_key_server);
			}
			else if(strcasecmp(strKey.c_str(), "CAClientCrt") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_crt_client);
				strtrim(m_ca_crt_client);
			}
			else if(strcasecmp(strKey.c_str(), "CAClientKey") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_key_client);
				strtrim(m_ca_key_client);
			}
			else if(strcasecmp(strKey.c_str(), "CAPassword") == 0)
			{
				m_ca_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_ca_password);
			}
            else if(strcasecmp(strKey.c_str(), "CAClientBaseDir") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ca_client_base_dir);
				strtrim(m_ca_client_base_dir);
			}
#ifdef _WITH_GSSAPI_              
            else if(strcasecmp(strKey.c_str(), "KRB5_KTNAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_ktname);
				strtrim(m_krb5_ktname);
              
				setenv("KRB5_KTNAME", m_krb5_ktname.c_str(), 1);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_REALM") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_realm);
				strtrim(m_krb5_realm);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_HOSTNAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_hostname);
				strtrim(m_krb5_hostname);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_SMTP_SERVICE_NAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_smtp_service_name);
				strtrim(m_krb5_smtp_service_name);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_POP3_SERVICE_NAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_pop3_service_name);
				strtrim(m_krb5_pop3_service_name);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_IMAP_SERVICE_NAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_imap_service_name);
				strtrim(m_krb5_imap_service_name);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_HTTP_SERVICE_NAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_http_service_name);
				strtrim(m_krb5_http_service_name);
			}
            else if(strcasecmp(strKey.c_str(), "KRB5_XMPP_SERVICE_NAME") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_krb5_xmpp_service_name);
				strtrim(m_krb5_xmpp_service_name);
			}
#endif /* _WITH_GSSAPI_ */            
			else if(strcasecmp(strKey.c_str(), "DBHost") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_host );
				strtrim(m_db_host);
			}
            else if(strcasecmp(strKey.c_str(), "DBPort") == 0)
			{
				string dbport;
				strcut(strline.c_str(), "=", NULL, dbport );
				strtrim(dbport);
				m_db_port = atoi(dbport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "DBName") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_name );
				strtrim(m_db_name);
			}
			else if(strcasecmp(strKey.c_str(), "DBUser") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_username );
				strtrim(m_db_username);
			}
			else if(strcasecmp(strKey.c_str(), "DBPassword") == 0)
			{
				m_db_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_db_password);
				
			}
            else if(strcasecmp(strKey.c_str(), "DBSockFile") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_db_sock_file );
				strtrim(m_db_sock_file);
			}
			else if(strcasecmp(strKey.c_str(), "DBMaxConn") == 0)
			{
				string DBMaxConn;
				strcut(strline.c_str(), "=", NULL, DBMaxConn );
				strtrim(DBMaxConn);
				m_db_max_conn = atoi(DBMaxConn.c_str());
			}
            
            else if(strcasecmp(strKey.c_str(), "ClusterMaster") == 0)
			{
				string is_master;
				strcut(strline.c_str(), "=", NULL, is_master);
				strtrim(is_master);
				m_is_master = (strcasecmp(is_master.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strcasecmp(strKey.c_str(), "MasterDBHost") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_master_db_host );
				strtrim(m_master_db_host);
			}
            else if(strcasecmp(strKey.c_str(), "MasterDBPort") == 0)
			{
				string dbport;
				strcut(strline.c_str(), "=", NULL, dbport );
				strtrim(dbport);
				m_master_db_port = atoi(dbport.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "MasterDBName") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_master_db_name );
				strtrim(m_master_db_name);
			}
			else if(strcasecmp(strKey.c_str(), "MasterDBUser") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_master_db_username );
				strtrim(m_master_db_username);
			}
			else if(strcasecmp(strKey.c_str(), "MasterDBPassword") == 0)
			{
				m_master_db_password = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_master_db_password);
				
			}
            else if(strcasecmp(strKey.c_str(), "MasterDBSockFile") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_master_db_sock_file );
				strtrim(m_master_db_sock_file);
			}

#ifdef _WITH_LDAP_
			else if(strcasecmp(strKey.c_str(), "LDAPServerURI") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_sever_uri );
				strtrim(m_ldap_sever_uri);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPServerVersion") == 0)
			{
				string LDAPServerVersion;
				strcut(strline.c_str(), "=", NULL, LDAPServerVersion );
				strtrim(LDAPServerVersion);
				m_ldap_sever_version = atoi(LDAPServerVersion.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "LDAPManager") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_manager );
				strtrim(m_ldap_manager);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPManagerPassword") == 0)
			{
				m_ldap_manager_passwd = "";
				
				string strEncoded;
				strcut(strline.c_str(), "=", NULL, strEncoded);
				strtrim(strEncoded);

				Security::Decrypt(strEncoded.c_str(), strEncoded.length(), m_ldap_manager_passwd);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchBase") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_base );
				strtrim(m_ldap_search_base);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchAttribute_User_ID") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_attribute_user_id );
				strtrim(m_ldap_search_attribute_user_id);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchAttribute_User_Password") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_attribute_user_password );
				strtrim(m_ldap_search_attribute_user_password);
			}
            else if(strcasecmp(strKey.c_str(), "LDAPSearchAttribute_User_Alias") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_attribute_user_alias );
				strtrim(m_ldap_search_attribute_user_alias);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchFilter_User") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_filter_user );
				strtrim(m_ldap_search_filter_user);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchFilter_Group") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_filter_group );
				strtrim(m_ldap_search_filter_group);
			}
			else if(strcasecmp(strKey.c_str(), "LDAPSearchAttribute_Group_Memeber") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_search_attribute_group_member );
				strtrim(m_ldap_search_attribute_group_member);
			}
            else if(strcasecmp(strKey.c_str(), "LDAPUserDN") == 0)
			{
				strcut(strline.c_str(), "=", NULL, m_ldap_user_dn );
				strtrim(m_ldap_user_dn);
			}
#endif /* _WITH_LDAP_ */
            else if(strcasecmp(strKey.c_str(), "MTAEnable") == 0)
			{
				string enable_mta;
				strcut(strline.c_str(), "=", NULL, enable_mta );
				strtrim(enable_mta);
				m_enablemta = (strcasecmp(enable_mta.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
			else if(strcasecmp(strKey.c_str(), "MTARelayThreadNum") == 0)
			{
				string mta_relaythreadnum;
				strcut(strline.c_str(), "=", NULL, mta_relaythreadnum );
				strtrim(mta_relaythreadnum);
				m_mta_relaythreadnum = atoi(mta_relaythreadnum.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "MTARelayCheckInterval") == 0)
			{
				string mta_relaycheckinterval;
				strcut(strline.c_str(), "=", NULL, mta_relaycheckinterval );
				strtrim(mta_relaycheckinterval);
				m_mta_relaycheckinterval = atoi(mta_relaycheckinterval.c_str());
                if(m_mta_relaycheckinterval <= 0)
                    m_mta_relaycheckinterval = 5;
			}
			else if(strcasecmp(strKey.c_str(), "SMTPHostNameCheck") == 0)
			{
				string SMTPHostNameCheck;
				strcut(strline.c_str(), "=", NULL, SMTPHostNameCheck );
				strtrim(SMTPHostNameCheck);
				m_enablesmtphostnamecheck = (strcasecmp(SMTPHostNameCheck.c_str(), "yes")) == 0 ? TRUE : FALSE;
			}
            else if(strcasecmp(strKey.c_str(), "ConnectionIdleTimeout") == 0)
			{
				string connection_idle_timeout;
				strcut(strline.c_str(), "=", NULL, connection_idle_timeout );
				strtrim(connection_idle_timeout);
				m_connection_idle_timeout = atoi(connection_idle_timeout.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "ServiceIdleTimeout") == 0)
			{
				string service_idle_timeout;
				strcut(strline.c_str(), "=", NULL, service_idle_timeout );
				strtrim(service_idle_timeout);
				m_service_idle_timeout = atoi(service_idle_timeout.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "ConnectionSyncTimeout") == 0)
			{
				string connection_sync_timeout;
				strcut(strline.c_str(), "=", NULL, connection_sync_timeout );
				strtrim(connection_sync_timeout);
				m_connection_sync_timeout = atoi(connection_sync_timeout.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "MaxServiceRequestNum") == 0)
			{
				string max_service_request_num;
				strcut(strline.c_str(), "=", NULL, max_service_request_num );
				strtrim(max_service_request_num);
				m_max_service_request_num = atoi(max_service_request_num.c_str());
			}
            else if(strcasecmp(strKey.c_str(), "ThreadIncreaseStep") == 0)
			{
				string thread_increase_step;
				strcut(strline.c_str(), "=", NULL, thread_increase_step );
				strtrim(thread_increase_step);
				m_thread_increase_step = atoi(thread_increase_step.c_str());
			}
			else if(strcasecmp(strKey.c_str(), "MEMCACHEDList") == 0)
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
				
				m_memcached_list.insert(std::pair<string, int>(memc_addr.c_str(), memc_port));
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
    return TRUE;
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
    
    return TRUE;
}

BOOL CMailBase::UnLoadList()
{
    return TRUE;
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



