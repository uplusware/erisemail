#ifndef _DNS_H_
#define _DNS_H_

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <algorithm>

using namespace std;

#include "util/general.h"

#pragma pack(push, 1)

typedef struct
{
	char address[65];
	unsigned int level;
} Mx_List;

bool __inline__ sort_algorithm(const Mx_List & l1, const Mx_List& l2)
{
	return (l1.level > l2.level);
}

typedef struct
{
	unsigned short Tag;
	unsigned short Flag;
	unsigned short nQuestion;
	unsigned short nResource;
	unsigned short nAuthenResource;
	unsigned short nExternResource;
}Dns_Header;

typedef struct
{
	unsigned short QueryType;
	unsigned short QueryBase;
}Question_Tailer;

typedef struct
{
	unsigned short QueryType;
	unsigned short QueryBase;
}Ack_Tailer;

#pragma pack(push, pop)

class udpdns
{
public:
	udpdns(const char* server)
	{
		m_server = server;
	}
	virtual ~udpdns()
	{
		
	}

	unsigned int gethostname(const unsigned char* dnsbuf, unsigned int seek, string& hostname)
	{
		unsigned int ret = 0;
		unsigned int i = 0;
		while(1)
		{
			if(dnsbuf[seek + i] == 0)
			{
				i++;
				break; 
			}
			else if((dnsbuf[seek + i] & 0xc0) == 0xc0)
			{
				unsigned short cptr = dnsbuf[seek + i] & (~0xc0);
				cptr <<= 8;
				i++;
				cptr |= dnsbuf[seek + i];
				//printf("cptr: %d\n", cptr);
				gethostname(dnsbuf, cptr, hostname);
				i++;
				break;
			}
			else
			{
				char sztmp[64];
				memset(sztmp, 0, 64);
				memcpy(sztmp, &dnsbuf[seek + i + 1], dnsbuf[seek + i]);
				i += dnsbuf[seek + i] + 1;
				if(hostname == "")
				{
					hostname += sztmp;
				}
				else
				{
					hostname += ".";
					hostname += sztmp;
				}
				
			}
		}
		return i;
	}
	
	int mxquery(const char* hostname, vector<Mx_List> & mxlist)
	{
		//make the dns package
		Dns_Header DnsHdr;
		memset(&DnsHdr, 0, sizeof(DnsHdr));
		DnsHdr.Tag = htons(time(NULL)%0xFFFF);
		DnsHdr.nQuestion = htons(1);
		DnsHdr.Flag = htons(0x0100);
		Question_Tailer qTailer;
		qTailer.QueryBase = htons(0x0001);
		qTailer.QueryType = htons(0x000F);

		unsigned char dnsbufptr[1024];
		unsigned int dnsbuflength = 0;
		memset(dnsbufptr, 0 ,1024);
		memcpy(dnsbufptr, &DnsHdr, sizeof(Dns_Header));
		
		vector<string> vecDest;
		string strhostname = hostname;
		vSplitString(strhostname, vecDest, ".", TRUE, 0x7FFFFFFF);
		int vLen = vecDest.size();
		int qseek = 0;
		for(int i = 0;  i < vLen; i++)
		{
			dnsbufptr[sizeof(Dns_Header) + qseek] = vecDest[i].length();
			qseek += 1;
			memcpy(&dnsbufptr[sizeof(Dns_Header) + qseek], vecDest[i].c_str(), vecDest[i].length());
			qseek += vecDest[i].length();
		}
		
		dnsbufptr[sizeof(Dns_Header) + qseek] = 0;
		qseek += 1;
		memcpy(&dnsbufptr[sizeof(Dns_Header) + qseek], &qTailer, sizeof(Question_Tailer));
		dnsbuflength = sizeof(Dns_Header) + qseek + sizeof(Question_Tailer);
		
		//connect dns server;
		int dns_sockfd;
		struct sockaddr_in server_sockaddr;
		struct sockaddr client_sockaddr;
		if((dns_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			return -1;
		}
		server_sockaddr.sin_family = AF_INET;
		server_sockaddr.sin_port = htons(53);
		server_sockaddr.sin_addr.s_addr = inet_addr(m_server.c_str());

		int flags = fcntl(dns_sockfd, F_GETFL, 0); 
		fcntl(dns_sockfd, F_SETFL, flags | O_NONBLOCK); 

		int res;
		fd_set mask; 
		struct timeval dns_timeout; 
		dns_timeout.tv_sec = 5; 
		dns_timeout.tv_usec = 0;

		FD_ZERO(&mask);
		FD_SET(dns_sockfd, &mask);

		res = select(dns_sockfd + 1,NULL, &mask, NULL, &dns_timeout);

		if( res != 1) 
		{
			close(dns_sockfd);
			return -1;
		}
		
		int n = sendto(dns_sockfd, (char*)dnsbufptr, dnsbuflength, 0, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr));
		//printf("n: %d\n", n);
		if(n != dnsbuflength)
		{
			return -1;
		}

		//Recv Ack
		memset(dnsbufptr, 0, 1024);
		int nLen=sizeof(struct sockaddr);
		FD_ZERO(&mask);
		FD_SET(dns_sockfd, &mask);

		res = select(dns_sockfd + 1,&mask, NULL, NULL, &dns_timeout);
		if( res != 1) 
		{
			close(dns_sockfd);
			return -1;
		}
		n = recvfrom(dns_sockfd, (char*)dnsbufptr, 1024, 0, (struct sockaddr*)&client_sockaddr, (socklen_t*)&nLen);
		if(n < 12)
		{
			return -1;
		}

		
		Dns_Header* pAckHeader = (Dns_Header*)dnsbufptr;
		//printf("%d %d %d\n", n, pAckHeader->Tag, DnsHdr.Tag);
		if(pAckHeader->Tag != DnsHdr.Tag)
			return -1;

		//Jump over the Quesion field
		int c = ntohs(pAckHeader->nQuestion);
		//printf("c: %d\n", c);
		int aseek = 0;
		while(c)
		{
			string hostname;
			unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + aseek, hostname);
			c--;
			aseek += hostnamelen;
			//printf("%s\n", hostname.c_str());
			
		}

		int j;
		
		
		//printf("nResource: %d\n", ntohs(pAckHeader->nResource));
		
		for(j = 0; j < ntohs(pAckHeader->nResource); j++)
		{
			//printf("\n\n");
			c = 1;
			while(c)
			{
				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);
				c--;
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
				//printf("%s\n", hostname.c_str());
			}

			unsigned short reply_type = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_type <<= 8;
			reply_type |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			//printf("replytype: %d\n", reply_type);
			
			unsigned short reply_class = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_class <<= 8;
			reply_class |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			unsigned int replay_ttl = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			
			//printf("replay_ttl: %d\n", replay_ttl);


			unsigned short reply_resource_len = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_resource_len <<= 8;
			reply_resource_len |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			//printf("reply_resource_len: %d\n", reply_resource_len);

			if(reply_type == 0x000F)
			{
				unsigned short reply_resource_preference = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				reply_resource_preference <<= 8;
				reply_resource_preference |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				//printf("reply_resource_preference: %d\n", reply_resource_preference);

				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);

				Mx_List list;
				strcpy(list.address, hostname.c_str());
				list.level = reply_resource_preference;
				mxlist.push_back(list);
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
				//printf("%s\n\n\n", hostname.c_str());
			}
			else
			{
				aseek += reply_resource_len;
			}
		}
		
		//printf("nAuthenResource: %d\n", ntohs(pAckHeader->nAuthenResource));
		for(j = 0; j < ntohs(pAckHeader->nAuthenResource); j++)
		{
			//printf("\n\n");
			c = 1;
			while(c)
			{
				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);
				c--;
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
				//printf("%s\n", hostname.c_str());
			}

			unsigned short reply_type = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_type <<= 8;
			reply_type |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			//printf("replytype: %d\n", reply_type);
			
			unsigned short reply_class = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_class <<= 8;
			reply_class |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			unsigned int replay_ttl = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			
			//printf("replay_ttl: %d\n", replay_ttl);

			unsigned short reply_resource_len = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_resource_len <<= 8;
			reply_resource_len |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			//printf("reply_resource_len: %d\n", reply_resource_len);

			if(reply_type == 0x000F)
			{
				unsigned short reply_resource_preference = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				reply_resource_preference <<= 8;
				reply_resource_preference |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				//printf("reply_resource_preference: %d\n", reply_resource_preference);

				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);

				Mx_List list;
				strcpy(list.address, hostname.c_str());
				list.level = reply_resource_preference;
				mxlist.push_back(list);
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
			}
			else
			{
				aseek += reply_resource_len;
			}
			
		}
		
		//printf("nExternResource: %d\n", ntohs(pAckHeader->nExternResource));
		for(j = 0; j < ntohs(pAckHeader->nExternResource); j++)
		{
			//printf("j: %d aseek: %d\n", j, aseek);
			c = 1;
			while(c)
			{
				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);
				c--;
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
				//printf("%s\n", hostname.c_str());
			}

			unsigned short reply_type = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_type <<= 8;
			reply_type |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			//printf("replytype: %d\n", reply_type);
			
			unsigned short reply_class = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_class <<= 8;
			reply_class |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;

			unsigned int replay_ttl = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			replay_ttl <<= 8;

			replay_ttl |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			
			//printf("replay_ttl: %u\n", replay_ttl);


			unsigned short reply_resource_len = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			reply_resource_len <<= 8;
			reply_resource_len |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
			aseek++;
			//printf("reply_resource_len: %d\n", reply_resource_len);

			if(reply_type == 0x000F)
			{
				unsigned short reply_resource_preference = dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				reply_resource_preference <<= 8;
				reply_resource_preference |= dnsbufptr[sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek];
				aseek++;
				//printf("reply_resource_preference: %d\n", reply_resource_preference);

				string hostname;
				unsigned int hostnamelen = gethostname(dnsbufptr, sizeof(Dns_Header) + sizeof(Question_Tailer) + aseek, hostname);

				Mx_List list;
				strcpy(list.address, hostname.c_str());
				list.level = reply_resource_preference;
				mxlist.push_back(list);
				aseek += hostnamelen;
				//printf("hostnamelen: %d\n", hostnamelen);
				//printf("%s\n\n\n", hostname.c_str());
			}
			else
			{
				aseek += reply_resource_len;
			}
		}
		if(mxlist.size() > 0)
			sort(mxlist.begin(), mxlist.end(), sort_algorithm);
		return 0;
	}
protected:
	string m_server;
};

#endif /* _DNS_H_ */

