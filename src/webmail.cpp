#include "webmail.h"

void Webmail::Response()
{
    if(m_session->GetMethod()!= hmPost && m_session->GetMethod() != hmGet)
    {
        m_session->HttpSend(RSP_501_SYS_ERR, strlen(RSP_501_SYS_ERR));
        return;
    }
    string strQueryPage = m_session->GetQueryPage();
    string strResp;
    strResp.clear();
    vector <string> vPath;
    
    if(strQueryPage == "")
    {
        strQueryPage = "index.html";
        
    }
    else if((strcasecmp(strQueryPage.c_str(),"admin") == 0) || (strcasecmp(strQueryPage.c_str(),"admin/") == 0))
    {
        strQueryPage = "man_index.html";
    }
    else
    {
        vSplitString(strQueryPage, vPath, "/", TRUE, 0x7FFFFFFF);
        
        int nCDUP = 0;
        int nCDIN = 0;
        for(int x = 0; x < vPath.size(); x++)
        {
            if(vPath[x] == "..")
            {
                nCDUP++;
            }
            else if(vPath[x] == ".")
            {
                //do nothing
            }
            else
            {
                nCDIN++;
            }
        }
        if(nCDUP >= nCDIN)
        {
            strQueryPage = "index.html";
        }
    }
    
    m_session->SetPagename(strQueryPage.c_str());
    
    if(strQueryPage == "api/listusers.xml")
    {	
        ApiListUsers* pDoc = new ApiListUsers(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listgroups.xml")
    {	
        ApiListGroups* pDoc = new ApiListGroups(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listmembers.xml")
    {
        ApiListMembers* pDoc = new ApiListMembers(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/addusertogroup.xml")
    {
        ApiAddUserToGroup* pDoc = new ApiAddUserToGroup(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/deluserfromgroup.xml")
    {
        ApiDelUserFromGroup* pDoc = new ApiDelUserFromGroup(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listmembersofgroup.xml")
    {
        ApiListMembersofGroup* pDoc = new ApiListMembersofGroup(m_session);
        pDoc->Response();
        delete pDoc;	
    }
    else if(strQueryPage == "api/listlevels.xml")
    {
        ApiListLevels* pDoc = new ApiListLevels(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/createlevel.xml")
    {
        ApiCreateLevel* pDoc = new ApiCreateLevel(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/defaultlevel.xml")
    {
        ApiDefaultLevel* pDoc = new ApiDefaultLevel(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/updatelevel.xml")
    {
        ApiUpdateLevel* pDoc = new ApiUpdateLevel(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/deletelevel.xml")
    {
        ApiDeleteLevel* pDoc = new ApiDeleteLevel(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/adduser.xml")
    {
        ApiCreateUser* pDoc = new ApiCreateUser(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/edituser.xml")
    {
        ApiUpdateUser* pDoc = new ApiUpdateUser(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/deluser.xml")
    {
        ApiDeleteUser* pDoc = new ApiDeleteUser(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listdirs.xml")
    {
        ApiListDirs* pDoc = new ApiListDirs(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/traversaldirs.xml")
    {
        ApiTraversalDirs* pDoc = new ApiTraversalDirs(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listmails.xml")
    {
        ApiListMails* pDoc = new ApiListMails(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/listunauditedmails.xml")
    {
        ApiListUnauditedMails* pDoc = new ApiListUnauditedMails(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/readmail.xml")
    {
        ApiReadMail* pDoc = new ApiReadMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/login.xml")
    {
        ApiLogin* pDoc = new ApiLogin(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/manlogin.xml")
    {
        ApiManLogin* pDoc = new ApiManLogin(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/passwd.xml")
    {
        ApiPasswd* pDoc = new ApiPasswd(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/alias.xml")
    {
        
        ApiAlias* pDoc = new ApiAlias(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/userinfo.xml")
    {
        
        ApiUserInfo* pDoc = new ApiUserInfo(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/logout.xml")
    {
        ApiLogout* pDoc = new ApiLogout(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/delmail.xml")
    {
        ApiDeleteMail* pDoc = new ApiDeleteMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/passmail.xml")
    {
        ApiPassMail* pDoc = new ApiPassMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/rejectmail.xml")
    {
        ApiRejectMail* pDoc = new ApiRejectMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/sendmail.xml")
    {
        ApiSendMail* pDoc = new ApiSendMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/upfile.cgi")
    {
        ApiUploadFile* pDoc = new ApiUploadFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/flagmail.xml")
    {
        ApiFlagMail* pDoc = new ApiFlagMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/seenmail.xml")
    {
        ApiSeenMail* pDoc = new ApiSeenMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/getmailnum.xml")
    {
        ApiGetDirMailNum* pDoc = new ApiGetDirMailNum(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/getunauditedmailnum.xml")
    {
        ApiGetUnauditedMailNum* pDoc = new ApiGetUnauditedMailNum(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/savedraft.xml")
    {
        ApiSaveDraft* pDoc = new ApiSaveDraft(m_session);
        pDoc->Response();
        delete pDoc;	
    }
    else if(strQueryPage == "api/savesent.xml")
    {
        ApiSaveSent* pDoc = new ApiSaveSent(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/movemail.xml")
    {
        ApiMoveMail* pDoc = new ApiMoveMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/trashmail.xml")
    {
        ApiTrashMail* pDoc = new ApiTrashMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/copymail.xml")
    {
        
        ApiCopyMail* pDoc = new ApiCopyMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/deluploaded.xml")
    {
        
        ApiDeleteUploadedFile* pDoc = new ApiDeleteUploadedFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/attachment.cgi")
    {
        ApiAttachment* pDoc = new ApiAttachment(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/gettmpfile.cgi")
    {
        ApiTempFile* pDoc = new ApiTempFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/getunseen.xml")
    {
        ApiUnseenMail* pDoc = new ApiUnseenMail(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/emptytrash.xml")
    {	
        ApiEmptyTrash* pDoc = new ApiEmptyTrash(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/extractattach.xml")
    {
        
        ApiExtractAttach* pDoc = new ApiExtractAttach(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/createlabel.xml")
    {
        
        ApiCreateLabel* pDoc = new ApiCreateLabel(m_session);
        pDoc->Response();
        delete pDoc;	
    }
    else if(strQueryPage == "api/deletelabel.xml")
    {
    
        ApiDeleteLabel* pDoc = new ApiDeleteLabel(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/loadconfigfile.xml")
    {
    
        ApiLoadConfigFile* pDoc = new ApiLoadConfigFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/saveconfigfile.xml")
    {
        ApiSaveConfigFile* pDoc = new ApiSaveConfigFile(m_session);
        pDoc->Response();
        delete pDoc;	
    }
    else if(strQueryPage == "api/listlogs.xml")
    {
    
        ApiListLogFile* pDoc = new ApiListLogFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/getlog.cgi")
    {
    
        ApiGetLogFile* pDoc = new ApiGetLogFile(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/systemstatus.xml")
    {
    
        ApiSystemStatus* pDoc = new ApiSystemStatus(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else if(strQueryPage == "api/currentusername.xml")
    {
        ApiCurrentUsername* pDoc = new ApiCurrentUsername(m_session);
        pDoc->Response();
        delete pDoc;
    }
    else
    {
        doc *pDoc = new doc(m_session);
        pDoc->Response();
        delete pDoc;
    }
}	