#include "util/general.h"
#include "base.h"
#include <libgen.h>
#include "util/security.h"

void usage()
{
	printf("Usage:eriseutil --install\n");
	printf("Usage:eriseutil --uninstall\n");
	printf("Usage:eriseutil -a user_group_name password  alias [-u|-g] [-A|-U]\n");
	printf("Usage:eriseutil -a user_group_name alias [-u|-g] [-A|-U]\n");
	printf("Usage:eriseutil -d user_group_name\n");
	printf("Usage:eriseutil -p username\n");
	printf("Usage:eriseutil -A username groupname\n");
	printf("Usage:eriseutil -D username groupname\n");
	printf("Usage:eriseutil -l --level\n");
	printf("Usage:eriseutil -s --level username level_id\n");
	printf("Usage:eriseutil -s --defaultlevel level_id\n");
	printf("Usage:eriseutil -a --level level_name level_description level_mailmaxsize level_boxmaxsize level_audit(yes/no) level_mailsizethreshold level_attachsizethreshold\n");
	printf("Usage:eriseutil -l --user\n");
	printf("Usage:eriseutil -l --group groupname\n");
	printf("Usage:eriseutil --disable user_group_name\n");
	printf("Usage:eriseutil --enable user_group_name\n");
	printf("Usage:eriseutil --encode\n");
}

int run(int argc, char* argv[])
{
	int retVal = 0;
	CMailBase::LoadConfig();
	MailStorage* mailStg;
	mailStg = new MailStorage(CMailBase::m_encoding.c_str(), CMailBase::m_private_path.c_str());
	do
	{
		if(argc == 10)
		{
			if(strcmp(argv[1], "-a") == 0)
			{
				if(strcmp(argv[2], "--level") == 0)
				{
					unsigned int lid;
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
						mailStg->AddLevel(argv[3], argv[4], atoi(argv[5]), atoi(argv[6]), strcasecmp(argv[7], "yes") == 0 ? eaTrue : eaFalse, atoi(argv[8]), atoi(argv[9]), lid);
						printf("Done.\n");
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					usage();
				}
			}
			else
			{
				usage();
			}
		}
		else if(argc == 7)
		{
			if(strcmp(argv[1], "-a") == 0)
			{
				UserType type;
				UserRole role;
				if(strcmp(argv[5], "-u") == 0)
					type = utMember;
				else if(strcmp(argv[5], "-g") == 0)
					type = utGroup;

				if(strcmp(argv[6], "-A") == 0)
				 	role = urAdministrator;
				else if(strcmp(argv[6], "-U") == 0)
				 	role = urGeneralUser;
				
				int lid;
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
					mailStg->GetDefaultLevel(lid);
					if(mailStg->AddID(argv[2], argv[3],argv[4], type, role, 5000*1024, lid) == -1)
					{
						printf("Add the user Failed\n");
						retVal = -1;
						break;
					}
					else
					 	printf("Done\n");

				}
				else
				{
					printf("Ops..., system error!\n");
				}
				
			}
			else
			{
				usage();
			}
		}
		else if(argc == 6)
		{
			if(strcmp(argv[1], "-a") == 0)
			{
				string strpwd1 = getpass("New Password:");
				string strpwd2 = getpass("Verify:");
				if(strpwd1 == strpwd2)
				{
					UserType type;
					UserRole role;
					if(strcmp(argv[4], "-u") == 0)
					 	type = utMember;
					else if(strcmp(argv[4], "-g") == 0)
					 	type = utGroup;

					if(strcmp(argv[5], "-A") == 0)
					 	role = urAdministrator;
					else if(strcmp(argv[5], "-U") == 0)
					 	role = urGeneralUser;
					int lid;
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
										
						mailStg->GetDefaultLevel(lid);
						if(mailStg->AddID(argv[2], strpwd1.c_str(),argv[3], type, role, 5000*1024, lid) == -1)
						{
					 		printf("Add the user Failed\n");
							retVal = -1;
							break;
						}
						else
					 		printf("Done\n");
						
					}
					else
					{
						printf("Ops..., system error!\n");
					}
					
				}
				else
				{
					printf("Sorry, passwords do not match.\n");
				}
			}
			else
			{
				usage();
			}
		}
		else if(argc == 5)
		{
			if(strcmp(argv[1], "-s") == 0)
			{
				if(strcmp(argv[2], "--level") == 0)
				{

					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
											
						mailStg->SetUserLevel(argv[3], atoi(argv[4]));
						
						printf("Done.\n");
						
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					usage();
				}
			}
			else
			{
				usage();
			}
		}
		else if(argc == 4)
		{
			if(strcmp(argv[1], "-A") == 0)
			{
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
										
					if(mailStg->AppendUserToGroup(argv[2], argv[3])== -1)
					{
						printf("Add user to group failed or username has exist\n");
						retVal = -1;
						break;
					}
					else
						printf("Done\n");

					
				}
				else
				{
					printf("Ops..., system error!\n");
				}
				
			}
			else if(strcmp(argv[1], "-D") == 0)
			{
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
										
					if(mailStg->RemoveUserFromGroup(argv[2], argv[3])== -1)
					{
						printf("Remove user from group failed or username not exist\n");
						retVal = -1;
						break;
					}
					else
						printf("Done\n");

				}
				else
				{
					printf("Ops..., system error!\n");
				}
				
			}
			else if(strcmp(argv[1], "-l") == 0)
			{
				if(strcmp(argv[2], "--group") == 0)
				{
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
												
						vector < User_Info > listtbl;
						if(mailStg->ListMemberOfGroup(argv[3], listtbl) == -1)
						{
							retVal = -1;
							break;
						}
						int uLen = listtbl.size();
						if(uLen > 0)
						{
							printf("----------------------\n");
							for(int i = 0; i < uLen; i++)
							{
								printf("|  %-16s  |\n", listtbl[i].username);
								printf("----------------------\n");
							}
						}
						
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					usage();
				}
			}
			else if(strcmp(argv[1], "-s") == 0)
			{
				if(strcmp(argv[2], "--defaultlevel") == 0)
				{
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
											
						if(mailStg->SetDefaultLevel(atoi(argv[3])) == -1)
						{
							retVal = -1;
							break;
						}
						else
							printf("Done.\n");

					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					usage();
				}
			}
			else
			{
				usage();
			}
		}
		else if(argc == 3)
		{
			if(strcmp(argv[1], "-d") == 0)
			{
				if(strcasecmp(argv[2], "admin") == 0)
				{
					printf("You can not delete the administrator's account id.\n");
					retVal = -1;
					break;
				}

				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
										
					if(mailStg->DelID(argv[2]) == -1)
					{
						printf("Delete the user failed\n");
						retVal = -1;
						break;
					}
					else
						printf("Done\n");

				}
				else
				{
					printf("Ops..., system error!\n");
				}
				
			}
			else if(strcmp(argv[1], "-p") == 0)
			{
				string strpwd1 = getpass("New Password:");
				string strpwd2 = getpass("Verify:");

				if(strpwd1 == strpwd2)
				{
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
											
						if(mailStg->Passwd(argv[2], strpwd1.c_str()) == -1)
						{
					 		printf("Update password failed\n");
							retVal = -1;
							break;
						}
						else
						 	printf("Done\n");
						
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					printf("Sorry, Twice input is not equal.\n");
				}
			}
			else if(strcmp(argv[1], "-l") == 0)
			{
				if(strcmp(argv[2], "--user") == 0)
				{
					const char* szUserType[] = {NULL, "Member", "Group", NULL};
					const char* szUserRole[] = {NULL, "User", "Administrator", NULL};
					const char* szUserStatus[] = {"Active", "Disabled", NULL};
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
											
						vector < User_Info > listtbl;
						mailStg->ListID(listtbl);
						int uLen = listtbl.size();
						printf("-----------------------------------------------------------------------------------------\n");
						printf("|  ID               |  Alias              |  Type     |  Role               |  Status   |\n");
						printf("-----------------------------------------------------------------------------------------\n"); 
						for(int i = 0; i < uLen; i++)
						{
							
							printf("|  %-16s |  %-18s |  %-8s |  %-18s |  %-8s |\n", 
								listtbl[i].username, listtbl[i].alias, szUserType[listtbl[i].type], 
								listtbl[i].type == utGroup ? "*" : szUserRole[listtbl[i].role],
								szUserStatus[listtbl[i].status]);
							printf("-----------------------------------------------------------------------------------------\n");
						}
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else if(strcmp(argv[2], "--level") == 0)
				{
					
					if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
					{
											
						vector <Level_Info> listtbl;
						mailStg->ListLevel(listtbl);
						int uLen = listtbl.size();
						printf("-------------------------------------------\n");
						printf("|  ID               |  Name               |\n");
						printf("-------------------------------------------\n"); 
						for(int i = 0; i < uLen; i++)
						{
						printf("|   %09d       |  %-18s |\n", listtbl[i].lid, listtbl[i].lname.c_str());
						printf("-------------------------------------------\n");
						}
					}
					else
					{
						printf("Ops..., system error!\n");
					}
					
				}
				else
				{
					usage();
				}
			}
			else if(strcmp(argv[1], "--enable") == 0)
			{
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
									
					mailStg->SetUserStatus(argv[2], usActive);
					
					printf("Done\n");
				}
				else
				{
					printf("Ops..., system error!\n");
				}
			}
			else if(strcmp(argv[1], "--disable") == 0)
			{
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0)
				{
									
					mailStg->SetUserStatus(argv[2], usDisabled);
					
					printf("Done\n");
				}
				else
				{
					printf("Ops..., system error!\n");
				}
			}
			else
			{
				usage();
			}
		}
		else if(argc == 2)
		{
			if(strcasecmp(argv[1], "--install") == 0)
			{
				
				if(mailStg && mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), NULL, CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) ==  0)
				{
					if(mailStg->Install(CMailBase::m_db_name.c_str()) == -1)
					{
						printf("Install Database failed\n");
						retVal = -1;
						break;
					}
					else
						printf("Database is ready.\n");
				}
				else
				{
					printf("Ops..., system error!\n");
				}
				
			}
			else if(strcasecmp(argv[1], "--uninstall") == 0)
			{
				string linestr;
				printf("Are you sure to uninstall the service?[Yes/No]:");
				getline(cin, linestr);
				if(strcasecmp(linestr.c_str(), "yes") == 0)
				{
					string strpwd = getpass("Please input administrator's password:");
					
					if(mailStg)
					{
						
					
						if(mailStg->Connect(CMailBase::m_db_host.c_str(), CMailBase::m_db_username.c_str(), CMailBase::m_db_password.c_str(), CMailBase::m_db_name.c_str(), CMailBase::m_db_port, CMailBase::m_db_sock_file.c_str()) == 0
                            && mailStg->CheckLogin("admin",strpwd.c_str()) == 0)
						{
							if(mailStg->Uninstall(CMailBase::m_db_name.c_str()) == -1)
							{
								retVal = -1;
								break;
							}
							else
								printf("Database is deleted.\n");
						}
						else
						{
							printf("Sorry, wrong password.\n");
							retVal = -1;
							break;
						}
					}
					else
					{
						printf("Ops..., system error!\n");
					}
				}
				else
				{
					printf("Abort the command\n");
				}
			}
			else if(strcmp(argv[1], "--encode") == 0)
			{
				string strpwd = getpass("Input: ");

				string strOut;
				Security::Encrypt(strpwd.c_str(), strpwd.length(), strOut);
				printf("%s\n", strOut.c_str());
			}
			else
			{
				usage();
			}
		}
		else
		{
			usage();
		}
	}while(0);
	
	if(mailStg)
	{
		mailStg->Close();
		delete mailStg;
	}

	return retVal;
}

int main(int argc, char* argv[])
{
	if(getgid() != 0)
	{
		printf("You need root privileges to run this program\n");
		return -1;
	}
	else
	{
		CMailBase::SetConfigFile(CONFIG_FILE_PATH, PERMIT_FILE_PATH, CONFIG_FILE_PATH, DOMAIN_FILE_PATH, WEBADMIN_FILE_PATH);
		CMailBase::LoadConfig();
		int ret = run(argc, argv);
		CMailBase::UnLoadConfig();
		return ret;
	}
}

