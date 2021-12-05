function init()
{
	window.parent.change_tab("status");
	list_status();
}

function display_tbl(tblobj, btnobj)
{
	if(tblobj.style.display == "none")
	{
		btnobj.src="closediv.gif";
		tblobj.style.display = "block";
	}
	else
	{
		btnobj.src="opendiv.gif";
		tblobj.style.display = "none";
	}
}

function list_status()
{
	var qUrl = "/api/systemstatus.xml";
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState==4)
		{
			if(xmlHttp.status== 200)
			{
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0" || errno == 0)
					{
						var strTmp;
						var statusList = responseNode.childNodes;
						
						for(var i = 0; i < statusList.length; i++)
						{
							if(statusList.item(i).tagName == "commonMailNumber")
							{
								_$_('commonMailNumber').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "deletedMailNumber")
							{
								_$_('deletedMailNumber').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "commonMailSize")
							{
								_$_('commonMailSize').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  Math.round(statusList.item(i).childNodes[0].nodeValue/1024*100)/100 + "KB";
							}
							else if(statusList.item(i).tagName == "deletedMailSize")
							{
								_$_('deletedMailSize').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  Math.round(statusList.item(i).childNodes[0].nodeValue/1024*100)/100 + "KB";
							}
							else if(statusList.item(i).tagName == "Encoding")
							{
								_$_('Encoding').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "EmailDomainName")
							{
								_$_('EmailDomainName').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "LocalHostName")
							{
								_$_('LocalHostName').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "HostIP")
							{
								_$_('HostIP').innerHTML = statusList.item(i).childNodes[0] == null ? "服务未绑定IP" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "DNSServer")
							{
								_$_('DNSServer').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "MaxConnPerProtocal")
							{
								_$_('MaxConnPerProtocal').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "RelayTaskNum")
							{
								_$_('RelayTaskNum').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}


							else if(statusList.item(i).tagName == "EnableRelay")
							{
								_$_('EnableRelay').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "SMTPHostNameCheck")
							{
								_$_('SMTPHostNameCheck').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "EnableSMTPTLS")
							{
								_$_('EnableSMTPTLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "SMTPSEnable")
							{
								_$_('SMTPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "SMTPSPort")
							{
								_$_('SMTPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							
							else if(statusList.item(i).tagName == "POP3Enable")
							{
								_$_('POP3Enable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "POP3Port")
							{
								_$_('POP3Port').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "EnablePOP3TLS")
							{
								_$_('EnablePOP3TLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "POP3SEnable")
							{
								_$_('POP3SEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "POP3SPort")
							{
								_$_('POP3SPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							
							else if(statusList.item(i).tagName == "IMAPEnable")
							{
								_$_('IMAPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "IMAPPort")
							{
								_$_('IMAPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "EnableIMAPTLS")
							{
								_$_('EnableIMAPTLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "IMAPSEnable")
							{
								_$_('IMAPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "IMAPSPort")
							{
								_$_('IMAPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}


							else if(statusList.item(i).tagName == "HTTPEnable")
							{
								_$_('HTTPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "HTTPPort")
							{
								_$_('HTTPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "HTTPSEnable")
							{
								_$_('HTTPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "HTTPSPort")
							{
								_$_('HTTPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							
							else if(statusList.item(i).tagName == "XMPPEnable")
							{
								_$_('XMPPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "XMPPPort")
							{
								_$_('XMPPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "EncryptXMPP")
							{
								var arr = ["Non-encrypted or TLS optional", "TLS required", "Old-SSL-based"];
								_$_('EncryptXMPP').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  arr[statusList.item(i).childNodes[0].nodeValue];
							}
							else if(statusList.item(i).tagName == "XMPPWorkerThreadNum")
							{
								_$_('XMPPWorkerThreadNum').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							
							else if(statusList.item(i).tagName == "LDAPEnable")
							{
								_$_('LDAPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
							else if(statusList.item(i).tagName == "LDAPPort")
							{
								_$_('LDAPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "LDAPSPort")
							{
								_$_('LDAPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" :  statusList.item(i).childNodes[0].nodeValue;
							}
							else if(statusList.item(i).tagName == "EncryptLDAP")
							{
								_$_('EncryptLDAP').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
							}
						}
						_$_("STATUS").innerHTML = "";
						_$_("STATUS").style.display = "none";
					}
				}
			}
		}
		else
		{
			_$_("STATUS").innerHTML = "<center><img src=\"waiting.gif\"></center>";
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}
