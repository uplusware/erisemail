#include "cache.h"
#include "tinyxml/tinyxml.h"

#define FILE_MAX_SIZE 	(1024*512)
#define MAX_CACHE_SIZE	(1024*1024*2)

memory_cache::memory_cache()
{
	m_htdoc.clear();
}

memory_cache::~memory_cache()
{
	unload();
}

void memory_cache::load(const char* szdir)
{
	unload();

	int cache_size = 0;
	
	//create the multitype list
	m_type_table.insert(map<string, string>::value_type("323", "text/h323"));
	m_type_table.insert(map<string, string>::value_type("acx", "application/internet-property-stream"));
	m_type_table.insert(map<string, string>::value_type("ai", "application/postscript"));
	m_type_table.insert(map<string, string>::value_type("aif", "audio/x-aiff"));
	m_type_table.insert(map<string, string>::value_type("aifc", "audio/x-aiff"));
	m_type_table.insert(map<string, string>::value_type("aiff", "audio/x-aiff"));
	m_type_table.insert(map<string, string>::value_type("asf", "video/x-ms-asf"));
	m_type_table.insert(map<string, string>::value_type("asr", "video/x-ms-asf"));
	m_type_table.insert(map<string, string>::value_type("asx", "video/x-ms-asf"));
	m_type_table.insert(map<string, string>::value_type("au", "audio/basic"));
	m_type_table.insert(map<string, string>::value_type("avi", "video/x-msvideo"));
	m_type_table.insert(map<string, string>::value_type("axs", "application/olescript"));
	m_type_table.insert(map<string, string>::value_type("bas", "text/plain"));
	m_type_table.insert(map<string, string>::value_type("bcpio", "application/x-bcpio"));
	m_type_table.insert(map<string, string>::value_type("bin", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("bmp", "image/bmp"));
	m_type_table.insert(map<string, string>::value_type("c", "text/plain"));
	m_type_table.insert(map<string, string>::value_type("cat", "application/vnd.ms-pkiseccat"));
	m_type_table.insert(map<string, string>::value_type("cdf", "application/x-cdf"));
	m_type_table.insert(map<string, string>::value_type("cer", "application/x-x509-ca-cert"));
	m_type_table.insert(map<string, string>::value_type("class", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("clp", "application/x-msclip"));
	m_type_table.insert(map<string, string>::value_type("cmx", "image/x-cmx"));
	m_type_table.insert(map<string, string>::value_type("cod", "image/cis-cod"));
	m_type_table.insert(map<string, string>::value_type("cpio", "application/x-cpio"));
	m_type_table.insert(map<string, string>::value_type("crd", "application/x-mscardfile"));
	m_type_table.insert(map<string, string>::value_type("crl", "application/pkix-crl"));
	m_type_table.insert(map<string, string>::value_type("crt", "application/x-x509-ca-cert"));
	m_type_table.insert(map<string, string>::value_type("csh", "application/x-csh"));
	m_type_table.insert(map<string, string>::value_type("css", "text/css"));
	m_type_table.insert(map<string, string>::value_type("dcr", "application/x-director"));
	m_type_table.insert(map<string, string>::value_type("der", "application/x-x509-ca-cert"));
	m_type_table.insert(map<string, string>::value_type("dir", "application/x-director"));
	m_type_table.insert(map<string, string>::value_type("dll", "application/x-msdownload"));
	m_type_table.insert(map<string, string>::value_type("dms", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("doc", "application/msword"));
	m_type_table.insert(map<string, string>::value_type("dot", "application/msword"));
	m_type_table.insert(map<string, string>::value_type("dvi", "application/x-dvi"));
	m_type_table.insert(map<string, string>::value_type("dxr", "application/x-director"));
	m_type_table.insert(map<string, string>::value_type("eps", "application/postscript"));
	m_type_table.insert(map<string, string>::value_type("etx", "text/x-setext"));
	m_type_table.insert(map<string, string>::value_type("evy", "application/envoy"));
	m_type_table.insert(map<string, string>::value_type("exe", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("fif", "application/fractals"));
	m_type_table.insert(map<string, string>::value_type("flr", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("gif", "image/gif"));
	m_type_table.insert(map<string, string>::value_type("gtar", "application/x-gtar"));
	m_type_table.insert(map<string, string>::value_type("gz", "application/x-gzip"));
	m_type_table.insert(map<string, string>::value_type("h", "text/plain"));
	m_type_table.insert(map<string, string>::value_type("hdf", "application/x-hdf"));
	m_type_table.insert(map<string, string>::value_type("hlp", "application/winhlp"));
	m_type_table.insert(map<string, string>::value_type("hqx", "application/mac-binhex40"));
	m_type_table.insert(map<string, string>::value_type("hta", "application/hta"));
	m_type_table.insert(map<string, string>::value_type("htc", "text/x-component"));
	m_type_table.insert(map<string, string>::value_type("htm", "text/html"));
	m_type_table.insert(map<string, string>::value_type("html", "text/html"));
	m_type_table.insert(map<string, string>::value_type("htt", "text/webviewhtml"));
	m_type_table.insert(map<string, string>::value_type("ico", "image/x-icon"));
	m_type_table.insert(map<string, string>::value_type("ief", "image/ief"));
	m_type_table.insert(map<string, string>::value_type("iii", "application/x-iphone"));
	m_type_table.insert(map<string, string>::value_type("ins", "application/x-internet-signup"));
	m_type_table.insert(map<string, string>::value_type("isp", "application/x-internet-signup"));
	m_type_table.insert(map<string, string>::value_type("jfif", "image/pipeg"));
	m_type_table.insert(map<string, string>::value_type("jpe", "image/jpeg"));
	m_type_table.insert(map<string, string>::value_type("jpeg", "image/jpeg"));
	m_type_table.insert(map<string, string>::value_type("jpg", "image/jpeg"));
	m_type_table.insert(map<string, string>::value_type("js", "application/x-javascript"));
	m_type_table.insert(map<string, string>::value_type("latex", "application/x-latex"));
	m_type_table.insert(map<string, string>::value_type("lha", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("lsf", "video/x-la-asf"));
	m_type_table.insert(map<string, string>::value_type("lsx", "video/x-la-asf"));
	m_type_table.insert(map<string, string>::value_type("lzh", "application/octet-stream"));
	m_type_table.insert(map<string, string>::value_type("m13", "application/x-msmediaview"));
	m_type_table.insert(map<string, string>::value_type("m14", "application/x-msmediaview"));
	m_type_table.insert(map<string, string>::value_type("m3u", "audio/x-mpegurl"));
	m_type_table.insert(map<string, string>::value_type("man", "application/x-troff-man"));
	m_type_table.insert(map<string, string>::value_type("mdb", "application/x-msaccess"));
	m_type_table.insert(map<string, string>::value_type("me", "application/x-troff-me"));
	m_type_table.insert(map<string, string>::value_type("mht", "message/rfc822"));
	m_type_table.insert(map<string, string>::value_type("mhtml", "message/rfc822"));
	m_type_table.insert(map<string, string>::value_type("mid", "audio/mid"));
	m_type_table.insert(map<string, string>::value_type("mny", "application/x-msmoney"));
	m_type_table.insert(map<string, string>::value_type("mov", "video/quicktime"));
	m_type_table.insert(map<string, string>::value_type("movie", "video/x-sgi-movie"));
	m_type_table.insert(map<string, string>::value_type("mp2", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mp3", "audio/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mpa", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mpe", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mpeg", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mpg", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("mpp", "application/vnd.ms-project"));
	m_type_table.insert(map<string, string>::value_type("mpv2", "video/mpeg"));
	m_type_table.insert(map<string, string>::value_type("ms", "application/x-troff-ms"));
	m_type_table.insert(map<string, string>::value_type("mvb", "application/x-msmediaview"));
	m_type_table.insert(map<string, string>::value_type("nws", "message/rfc822"));
	m_type_table.insert(map<string, string>::value_type("oda", "application/oda"));
	m_type_table.insert(map<string, string>::value_type("p10", "application/pkcs10"));
	m_type_table.insert(map<string, string>::value_type("p12", "application/x-pkcs12"));
	m_type_table.insert(map<string, string>::value_type("p7b", "application/x-pkcs7-certificates"));
	m_type_table.insert(map<string, string>::value_type("p7c", "application/x-pkcs7-mime"));
	m_type_table.insert(map<string, string>::value_type("p7m", "application/x-pkcs7-mime"));
	m_type_table.insert(map<string, string>::value_type("p7r", "application/x-pkcs7-certreqresp"));
	m_type_table.insert(map<string, string>::value_type("p7s", "application/x-pkcs7-signature"));
	m_type_table.insert(map<string, string>::value_type("pbm", "image/x-portable-bitmap"));
	m_type_table.insert(map<string, string>::value_type("pdf", "application/pdf"));
	m_type_table.insert(map<string, string>::value_type("pfx", "application/x-pkcs12"));
	m_type_table.insert(map<string, string>::value_type("pgm", "image/x-portable-graymap"));
	m_type_table.insert(map<string, string>::value_type("pko", "application/ynd.ms-pkipko"));
	m_type_table.insert(map<string, string>::value_type("pma", "application/x-perfmon"));
	m_type_table.insert(map<string, string>::value_type("pmc", "application/x-perfmon"));
	m_type_table.insert(map<string, string>::value_type("pml", "application/x-perfmon"));
	m_type_table.insert(map<string, string>::value_type("pmr", "application/x-perfmon"));
	m_type_table.insert(map<string, string>::value_type("pmw", "application/x-perfmon"));
	m_type_table.insert(map<string, string>::value_type("pnm", "image/x-portable-anymap"));
	m_type_table.insert(map<string, string>::value_type("pot,", "application/vnd.ms-powerpoint"));
	m_type_table.insert(map<string, string>::value_type("ppm", "image/x-portable-pixmap"));
	m_type_table.insert(map<string, string>::value_type("pps", "application/vnd.ms-powerpoint"));
	m_type_table.insert(map<string, string>::value_type("ppt", "application/vnd.ms-powerpoint"));
	m_type_table.insert(map<string, string>::value_type("prf", "application/pics-rules"));
	m_type_table.insert(map<string, string>::value_type("ps", "application/postscript"));
	m_type_table.insert(map<string, string>::value_type("pub", "application/x-mspublisher"));
	m_type_table.insert(map<string, string>::value_type("qt", "video/quicktime"));
	m_type_table.insert(map<string, string>::value_type("ra", "audio/x-pn-realaudio"));
	m_type_table.insert(map<string, string>::value_type("ram", "audio/x-pn-realaudio"));
	m_type_table.insert(map<string, string>::value_type("ras", "image/x-cmu-raster"));
	m_type_table.insert(map<string, string>::value_type("rgb", "image/x-rgb"));
	m_type_table.insert(map<string, string>::value_type("rmi", "audio/mid"));
	m_type_table.insert(map<string, string>::value_type("roff", "application/x-troff"));
	m_type_table.insert(map<string, string>::value_type("rtf", "application/rtf"));
	m_type_table.insert(map<string, string>::value_type("rtx", "text/richtext"));
	m_type_table.insert(map<string, string>::value_type("scd", "application/x-msschedule"));
	m_type_table.insert(map<string, string>::value_type("sct", "text/scriptlet"));
	m_type_table.insert(map<string, string>::value_type("setpay", "application/set-payment-initiation"));
	m_type_table.insert(map<string, string>::value_type("setreg", "application/set-registration-initiation"));
	m_type_table.insert(map<string, string>::value_type("sh", "application/x-sh"));
	m_type_table.insert(map<string, string>::value_type("shar", "application/x-shar"));
	m_type_table.insert(map<string, string>::value_type("sit", "application/x-stuffit"));
	m_type_table.insert(map<string, string>::value_type("snd", "audio/basic"));
	m_type_table.insert(map<string, string>::value_type("spc", "application/x-pkcs7-certificates"));
	m_type_table.insert(map<string, string>::value_type("spl", "application/futuresplash"));
	m_type_table.insert(map<string, string>::value_type("src", "application/x-wais-source"));
	m_type_table.insert(map<string, string>::value_type("sst", "application/vnd.ms-pkicertstore"));
	m_type_table.insert(map<string, string>::value_type("stl", "application/vnd.ms-pkistl"));
	m_type_table.insert(map<string, string>::value_type("stm", "text/html"));
	m_type_table.insert(map<string, string>::value_type("svg", "image/svg+xml"));
	m_type_table.insert(map<string, string>::value_type("sv4cpio", "application/x-sv4cpio"));
	m_type_table.insert(map<string, string>::value_type("sv4crc", "application/x-sv4crc"));
	m_type_table.insert(map<string, string>::value_type("swf", "application/x-shockwave-flash"));
	m_type_table.insert(map<string, string>::value_type("t", "application/x-troff"));
	m_type_table.insert(map<string, string>::value_type("tar", "application/x-tar"));
	m_type_table.insert(map<string, string>::value_type("tcl", "application/x-tcl"));
	m_type_table.insert(map<string, string>::value_type("tex", "application/x-tex"));
	m_type_table.insert(map<string, string>::value_type("texi", "application/x-texinfo"));
	m_type_table.insert(map<string, string>::value_type("texinfo", "application/x-texinfo"));
	m_type_table.insert(map<string, string>::value_type("tgz", "application/x-compressed"));
	m_type_table.insert(map<string, string>::value_type("tif", "image/tiff"));
	m_type_table.insert(map<string, string>::value_type("tiff", "image/tiff"));
	m_type_table.insert(map<string, string>::value_type("tr", "application/x-troff"));
	m_type_table.insert(map<string, string>::value_type("trm", "application/x-msterminal"));
	m_type_table.insert(map<string, string>::value_type("tsv", "text/tab-separated-values"));
	m_type_table.insert(map<string, string>::value_type("txt", "text/plain"));
	m_type_table.insert(map<string, string>::value_type("uls", "text/iuls"));
	m_type_table.insert(map<string, string>::value_type("ustar", "application/x-ustar"));
	m_type_table.insert(map<string, string>::value_type("vcf", "text/x-vcard"));
	m_type_table.insert(map<string, string>::value_type("vrml", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("wav", "audio/x-wav"));
	m_type_table.insert(map<string, string>::value_type("wcm", "application/vnd.ms-works"));
	m_type_table.insert(map<string, string>::value_type("wdb", "application/vnd.ms-works"));
	m_type_table.insert(map<string, string>::value_type("wks", "application/vnd.ms-works"));
	m_type_table.insert(map<string, string>::value_type("wmf", "application/x-msmetafile"));
	m_type_table.insert(map<string, string>::value_type("wps", "application/vnd.ms-works"));
	m_type_table.insert(map<string, string>::value_type("wri", "application/x-mswrite"));
	m_type_table.insert(map<string, string>::value_type("wrl", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("wrz", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("xaf", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("xbm", "image/x-xbitmap"));
	m_type_table.insert(map<string, string>::value_type("xla", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xlc", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xlm", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xls", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xlt", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xlw", "application/vnd.ms-excel"));
	m_type_table.insert(map<string, string>::value_type("xof", "x-world/x-vrml"));
	m_type_table.insert(map<string, string>::value_type("xpm", "image/x-xpixmap"));
	m_type_table.insert(map<string, string>::value_type("xwd", "image/x-xwindowdump"));
	m_type_table.insert(map<string, string>::value_type("z", "application/x-compress"));
	m_type_table.insert(map<string, string>::value_type("zip", "application/zip"));

	m_dirpath = szdir;
	
	m_htdoc.clear();
	struct dirent * dirp;
	DIR *dp = opendir(szdir);
	if(dp)
	{
		while( (dirp = readdir(dp)) != NULL)
		{
			if(cache_size > MAX_CACHE_SIZE)
				break;
			if((strcmp(dirp->d_name, "..") == 0) || (strcmp(dirp->d_name, ".") == 0))
			{
				continue;
			}
			else
			{
				string strfilename = dirp->d_name;
				string strfilepath = szdir;
				strfilepath += "/";
				strfilepath += strfilename;
				
				int fd = open(strfilepath.c_str(), O_RDONLY);
				if(fd > 0)
				{
					filedata fdata;
					
			  		struct stat file_stat;
			  		fstat(fd, &file_stat);
					
					fdata.flen = file_stat.st_size;
					
					if(fdata.flen < FILE_MAX_SIZE)
					{
						cache_size += fdata.flen;
						
						//printf("Load html %s\n", strfilename.c_str());
						fdata.pbuf = (char*)malloc(fdata.flen);
						int nRead = 0;
						while(1)
						{
							int l = read(fd, fdata.pbuf + nRead, fdata.flen - nRead);
							if(l <= 0)
								break;
							else
								nRead += l;
						}
						m_htdoc.insert(map<string, filedata>::value_type(strfilename, fdata));
					}
					close(fd);
				}
			}
		}
		closedir(dp);
	}

	string strLanguagePath = m_dirpath;
	strLanguagePath += "/language.xml";
	TiXmlDocument* pXmlDoc = new TiXmlDocument();
	pXmlDoc->LoadFile(strLanguagePath.c_str());

	if(pXmlDoc && pXmlDoc->RootElement())
	{
		TiXmlNode * pText = pXmlDoc->RootElement()->FirstChild("text");

		while(pText)
		{
			if(pText && pText->ToElement())
			{
				string strName = pText->ToElement()->Attribute("name");
				string strValue = pText->ToElement()->Attribute("value");				
				m_language.insert(map<string, string>::value_type(strName, strValue));
			}
			pText = pText->NextSibling("text");
		}
	}

	delete pXmlDoc;
	
}

void memory_cache::unload()
{
	map<string, filedata>::iterator iter;
	for(iter = m_htdoc.begin(); iter != m_htdoc.end(); iter++)
	{
		//printf("Unload html %s\n", iter->first.c_str());
		free(iter->second.pbuf);
	}
	m_htdoc.clear();

	m_language.clear();
}

