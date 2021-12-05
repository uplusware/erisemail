var sending_status = false;
var sent_status = false;
var saving_status = false;
var uploading_status = false;

var uploaded_file_list = new Array();

function uploading( status )
{
	uploading_status = status;
	toggle_onbeforeunload(uploading_status);
}

function toggle_onbeforeunload(val)
{
	if(val)
	{
		window.onbeforeunload = function(e)
		{
			if(sending_status || sent_status || saving_status || uploading_status)
			{
				return ("The mail is processing");
			}
			else if(isChange)
			{
				return ("Mail is not sent or saved yet");
			}
			else
			{
				return ("Sure to exit?");
			}
		}
	}
	else
	{
		if(!isChange)
			window.onbeforeunload = null;
	}
}

function RTC(strid)
{
	if(navigator.appName != "Netscape")
	{
		return this.document.frames[strid].document;
	}
	else
	{
		return this.document.getElementById(strid).contentWindow.document;
	}
}
	
var arrColor = [
	['000000','993300','333300','003300','003366','000080','333399','333333'],
	['800000','FF6600','808000','008000','008080','0000FF','666699','808080'],
	['FF0000','FF9900','99CC00','339966','33CCCC','3366FF','800080','999999'],
	['FF00FF','FFCC00','FFFF00','00FF00','00FFFF','00CCFF','993366','C0C0C0'],
	['FF99CC','FFCC99','FFFF99','CCFFCC','CCFFFF','99CCFF','CC99FF','FFFFFF']
	];

function colorCmd(cmd, divid, color)
{
	RTC("richeditor").execCommand(cmd, false ,color);

	_$_(divid).style.display = "none";
}

function sizeCmd(size)
{
	if(size == '0' || size == 0) 
		return;
	RTC("richeditor").execCommand('FontSize', false, size);

	_$_('fontSizeTbl').style.display = "none";
}

function paintColorDiv(cmd, divid)
{
  var Str="<table width='100%' border='0' cellspacing='2' cellpadding='2'><tbody>";
   for(var i = 0; i < arrColor.length; i++)
	{
	  Str+="<tr>";
	  for(var j = 0; j < arrColor[i].length;j++)
	  {
		 Str+="<td class='td' style='background-color:#" + arrColor[i][j] + ";border:1px solid #fafafa;'"
		 + " onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\";' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\";'"
		 + " onclick='colorCmd(\"" + cmd + "\", \"" + divid + "\", \"#" + arrColor[i][j] + "\")'>"
		 + "<img style='width:12px;height:12px' "+" style='background-color:#"+arrColor[i][j]+";border:solid 1px #cccccc'/></td>";
	  }
	  Str+="</tr>";
   }
   Str+="</tbody></table>";
   return Str;
}

function paintSizeDiv(cmd, divid)
{
	var Str="<table width='100%' border='0' cellspacing='2' cellpadding='2'><tbody>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"1\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='1'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"2\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='2'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"3\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='3'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"4\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='4'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"5\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='5'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"6\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='6'>T</font></td></tr>";
	Str+="<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"7\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='7'>T</font></td></tr>";
	Str+="</tbody></table>";
	return Str;
}

function popforeDiv()
{
	if(_$_('foreColorTbl').style.display == 'block')
		_$_('foreColorTbl').style.display='none'; 
	else 
		_$_('foreColorTbl').style.display='block';
	
	_$_('backColorTbl').style.display='none'; 
	_$_('fontSizeTbl').style.display='none'; 
	
	_$_('foreColorTbl').style.left = 89;
	_$_('foreColorTbl').style.top = 258;
}

function popbackDiv()
{
	if(_$_('backColorTbl').style.display == 'block')
		_$_('backColorTbl').style.display='none'; 
	else 
		_$_('backColorTbl').style.display='block';

	_$_('foreColorTbl').style.display='none';
	_$_('fontSizeTbl').style.display='none'; 
	
	_$_('backColorTbl').style.left = 115;
	_$_('backColorTbl').style.top = 258;
}

function popsizeDiv()
{
	if(_$_('fontSizeTbl').style.display == 'block')
		_$_('fontSizeTbl').style.display='none'; 
	else 
		_$_('fontSizeTbl').style.display='block';

	_$_('foreColorTbl').style.display='none';
	_$_('backColorTbl').style.display='none'; 
	
	_$_('fontSizeTbl').style.left = 150;
	_$_('fontSizeTbl').style.top = 258;
}

function ontimer()
{
	if(_$_("CONTENT").value != _$_('richeditor').contentWindow.document.body.innerHTML)
	{
		isChange = true;

		toggle_onbeforeunload(isChange)
			
		_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
	}
	
	setTimeout('ontimer()',1000);
}
	
function init()
{
	sending_status = false;
	sent_status = false;
	saving_status = false;
	uploading_status = false;
	isChange = false;
	
	_$_('GLOBALPATH').innerHTML = (Request.QueryString('GPATH') == null? ("/Drafts") : decodeURIComponent(Request.QueryString('GPATH')));

	strID = Request.QueryString('ID');
	strType = Request.QueryString('TYPE');
	
	if((strID != null)&&(strType != null))
	{			
		read_mail(strID, strType);
	}
	else
	{
		_$_('TO_ADDRS').value = Request.QueryString('TOADDRS') == null ? "" : Request.QueryString('TOADDRS');
	}

	_$_('foreColorTbl').innerHTML = paintColorDiv("ForeColor", "foreColorTbl");
	_$_('backColorTbl').innerHTML = paintColorDiv("BackColor", "backColorTbl");
	_$_('fontSizeTbl').innerHTML = paintSizeDiv("FontSize", "fontSizeTbl");

	_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
	
	ontimer();
}
	
function upfile()
{
	isChange = true;
	toggle_onbeforeunload(isChange);
}

function delmail(force)
{
	if(_$_('DRAFTID').value == null || _$_('DRAFTID').value == "" || _$_('DRAFTID').value == -1)
	{
		alert("The draft is unsaved yet")
		return false;
	}
	do_trashmail();

	if(window.opener.refresh)
		window.opener.refresh();
	
	isChange = false;
	window.onbeforeunload = null;
	
	window.close();
}

function do_delsentmail()
{
	if(_$_('DRAFTID').value == null || _$_('DRAFTID').value == "" || _$_('DRAFTID').value == -1)
	{
		return;
	}
	
	isChange = false;
	toggle_onbeforeunload(isChange);
	
	var qUrl = "/api/delmail.xml?MAILID=" + _$_('DRAFTID').value;
	var xmlHttp = initxmlhttp();
	xmlHttp.open("GET", qUrl , false);
	xmlHttp.send("");
	
	if (xmlHttp.status == 200)
	{
		var xmldom = xmlHttp.responseXML;
		xmldom.documentElement.normalize();
		var responseNode = xmldom.documentElement.childNodes.item(0);
		if(responseNode.tagName == "response")
		{
			var errno = responseNode.getAttribute("errno")
			if(errno == "0" || errno == 0)
			{
				return true;
			}
		}
	}
	return false;
}

function do_trashmail()
{
	if(_$_('DRAFTID').value == null || _$_('DRAFTID').value == "" || _$_('DRAFTID').value == -1)
	{
		return;
	}
	
	isChange = false;
	toggle_onbeforeunload(isChange);
	var qUrl = "/api/trashmail.xml?MAILID=" + _$_('DRAFTID').value;
	var xmlHttp = initxmlhttp();
	xmlHttp.open("GET", qUrl , false);
	xmlHttp.send("");
	
	if (xmlHttp.status == 200)
	{
		var xmldom = xmlHttp.responseXML;
		xmldom.documentElement.normalize();
		var responseNode = xmldom.documentElement.childNodes.item(0);
		if(responseNode.tagName == "response")
		{
			var errno = responseNode.getAttribute("errno")
			if(errno == "0" || errno == 0)
			{
				return true;
			}
		}
	}
	return false;
}


function flag_mail(flag)
{
	if(Request.QueryString('ID') == null || Request.QueryString('ID') == "")
	{
		alert("The draft is unsaved yet")
		return;
	}
	
	if(flag ==  true)
		strFlag="yes";
	else
		strFlag = "no";
		
	var qUrl = "/api/flagmail.xml?ID=" + Request.QueryString('ID') + "&FLAG=" + strFlag;
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState==4 && xmlHttp.status== 200)
		{
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0")
				{
					
				}
			}
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function seen_mail(flag)
{
	if(Request.QueryString('ID') == null || Request.QueryString('ID') == "")
	{
		alert("The draft is unsaved yet")
		return;
	}
	
	if(flag ==  true)
		strFlag="yes";
	else
		strFlag = "no";
		
	var qUrl = "/api/seenmail.xml?ID=" + Request.QueryString('ID') + "&SEEN=" + strFlag;
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState==4 && xmlHttp.status== 200)
		{
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					
				}
			}
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function do_savesent()
{
	var uploadedfiles = "";
	for(var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++)
	{
		uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
		uploadedfiles += "*"
	}
	_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
	var strPostData = "TO_ADDRS=" + encodeURIComponent(_$_('TO_ADDRS').value) + "&";
	strPostData += "CC_ADDRS=" + encodeURIComponent(_$_('CC_ADDRS').value) + "&";
	strPostData += "BCC_ADDRS=" + encodeURIComponent(_$_('BCC_ADDRS').value) + "&";
	strPostData += "SUBJECT=" + encodeURIComponent(_$_('SUBJECT').value) + "&";
	strPostData += "CONTENT=" + encodeURIComponent(_$_('CONTENT').value) + "&";
	strPostData += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
	
	var qUrl = "api/savesent.xml";
	
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState==4)
		{
			sent_status = false;
			toggle_onbeforeunload(sent_status);
			if(xmlHttp.status == 200)
			{
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0" || errno == 0)
					{
						do_delsentmail();

						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Save mail to sent folder successfully.";
						$( "#PROCESSING_DIV" ).dialog();

						if(window.opener && window.opener.refresh)
							window.opener.refresh();
						else if(window.opener && window.opener.childframe1 && window.opener.childframe1.refresh)
							window.opener.childframe1.refresh();
							
						
						isChange = false;
						window.onbeforeunload = null;
						
						window.close();
					}
					else
					{
						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Save mail to sent folder failed.</font>";
						$( "#PROCESSING_DIV" ).dialog();
					}
				}
			}
			else
			{
				_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Save mail to sent folder failed.</font>";
				$( "#PROCESSING_DIV" ).dialog();
			}
		}
		else
		{
			sent_status = true;
			toggle_onbeforeunload(sent_status);

			_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Saving mail to sent folder - <img src=\"waiting.gif\" />";
			$( "#PROCESSING_DIV" ).dialog();
		}
	}

	xmlHttp.open("POST", qUrl , true);
	xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlHttp.send(strPostData);
	
}

function send_mail()
{
	do_send_mail();	
}

function do_send_mail()
{
	if(_$_('TO_ADDRS').value == "")
	{
		alert("To address is empty.");
		return;
	}
	
	if(_$_('SUBJECT').value == "")
	{
		if(confirm("Subject is empty yet，do you continue?") == false)
			return;
	}
	
	var uploadedfiles = "";
	for(var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++)
	{
		uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
		uploadedfiles += "*"
	}
	_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
	var strPostData = "TO_ADDRS=" + encodeURIComponent(_$_('TO_ADDRS').value) + "&";
	strPostData += "CC_ADDRS=" + encodeURIComponent(_$_('CC_ADDRS').value) + "&";
	strPostData += "BCC_ADDRS=" + encodeURIComponent(_$_('BCC_ADDRS').value) + "&";
	strPostData += "SUBJECT=" + encodeURIComponent(_$_('SUBJECT').value) + "&";
	strPostData += "CONTENT=" + encodeURIComponent(_$_('CONTENT').value) + "&";
	strPostData += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
	var qUrl = "api/sendmail.xml";
	
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState==4)
		{
			sending_status = false;
			toggle_onbeforeunload(sending_status);
			if(xmlHttp.status== 200)
			{
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0")
					{
						isChange = false;
						toggle_onbeforeunload(isChange);
						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Send mail successfully.";
						$( "#PROCESSING_DIV" ).dialog();
						
						do_savesent();
					}
					else
					{
						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Send mail failed(Mail size is too large.)</font>";
						$( "#PROCESSING_DIV" ).dialog();
					}
				}
			}
			else
			{
				_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Send mail failed</font>";
			}
		}
		else
		{
			sending_status = true;
			toggle_onbeforeunload(sending_status);
			_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Sending mail - <img src=\"waiting.gif\" />";
			$( "#PROCESSING_DIV" ).dialog();
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlHttp.send(strPostData);
}

function save_draft()
{
	if(isChange == true)
	{
		do_savedraft();
	}
}

function do_savedraft()
{
	var uploadedfiles = "";
	for(var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++)
	{
		uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
		uploadedfiles += "*"
	}
	_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
	var strPostData = "DRAFTID=" + _$_('DRAFTID').value + "&";
	strPostData += "TO_ADDRS=" + encodeURIComponent(_$_('TO_ADDRS').value) + "&";
	strPostData += "CC_ADDRS=" + encodeURIComponent(_$_('CC_ADDRS').value) + "&";
	strPostData += "BCC_ADDRS=" + encodeURIComponent(_$_('BCC_ADDRS').value) + "&";
	strPostData += "SUBJECT=" + encodeURIComponent(_$_('SUBJECT').value) + "&";
	strPostData += "CONTENT=" + encodeURIComponent(_$_('CONTENT').value) + "&";
	strPostData += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
	var qUrl = "api/savedraft.xml";

	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		if(xmlHttp.readyState==4)
		{
			saving_status = false;
			toggle_onbeforeunload(saving_status);
			if(xmlHttp.status== 200)
			{
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0")
					{
						isChange = false;
						toggle_onbeforeunload(isChange);

						var bodyList = responseNode.childNodes;
						for(var i = 0; i < bodyList.length; i++)
						{
							if(bodyList.item(i).tagName == "draftid")
							{
								_$_('DRAFTID').value = bodyList.item(i).childNodes[0].nodeValue;
							}
						}
						
						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Save draft successfully";
						$( "#PROCESSING_DIV" ).dialog();
						
					}
					else
					{
						
						_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Save draft failed(Draft is too large)</font>";
						$( "#PROCESSING_DIV" ).dialog();

					}
				}
			}
			else
			{

				_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>Save draft failed</font>";
				$( "#PROCESSING_DIV" ).dialog();


			}
			//document.body.style.cursor = "default";
		}
		else
		{
			saving_status = true;
			toggle_onbeforeunload(saving_status);
			_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Saving draft - <img src=\"waiting.gif\" />";
			$( "#PROCESSING_DIV" ).dialog();
		}
	}
	
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlHttp.send(strPostData);
	
}

function read_mail(strID, strType)
{
	var qUrl = "/api/readmail.xml?EDIT=yes&ID=" + strID;
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
					var errno = responseNode.getAttribute("errno");
					
					if(errno == "0")
					{
						var strFrom, strTo, strCc, strSubject, strTextContent, strHtmlContent, strAttach;
						strFrom = "";
						strTo = "";
						strCc = "";
						strSubject = "";
						strTextContent = "";
						strHtmlContent = "";
						
						var bodyList = responseNode.childNodes;
						for(var i = 0; i < bodyList.length; i++)
						{
							if(bodyList.item(i).tagName == "from")
							{
								strFrom = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "to")
							{
								strTo = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "cc")
							{
								strCc = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "date")
							{
								strDate = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "subject")
							{
								strSubject = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "text_content")
							{
								strTextContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
								strTextContent = TextToHTML(strTextContent);
							}
							else if(bodyList.item(i).tagName == "html_content")
							{
								strHtmlContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
							else if(bodyList.item(i).tagName == "attach")
							{
								strAttach = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
							}
						}
						
						if(strType == "4")
						{
							_$_('DRAFTID').value = strID;
						
							_$_("TO_ADDRS").value = strTo;
							_$_("CC_ADDRS").value = strCc;
							_$_("SUBJECT").value = strSubject;
							_$_("CONTENT").value = (strHtmlContent!= "" ? strHtmlContent : strTextContent);
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
							
							if(strAttach != "")
							{
								extract_att(strID);	
							}
							
						}
						else if(strType == "3")
						{
							_$_("SUBJECT").value = "Fwd: " + strSubject;
							
							_$_("CONTENT").value = (strHtmlContent!= "" ? strHtmlContent : strTextContent);
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							
							strReply = "<br><br><br><br><hr>";
							strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>From:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>To:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
							if(strCc != "")
								strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Cc:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Date:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Subject:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
							strReply += "</table>";

							strReply += _$_('richeditor').contentWindow.document.body.innerHTML;
							
							_$_("CONTENT").value = strReply;
							
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
							
							if(strAttach != "")
							{
								extract_att(strID);	
							}
							
						}
						else if(strType == "2")
						{
							var to_addrs = strFrom + "," + strTo;								
							var to_addrs_arr = to_addrs.split(",");
							var to_addrs_result = new Array();
							to_addrs = "";
							for(var x = 0; x < to_addrs_arr.length; x++)
							{
								var res = retrieve(to_addrs_arr[x]);
								if(res != null)
								{
									var same = false;
									for(var a = 0; a < to_addrs_result.length; a++)
									{
										if(res == to_addrs_result[a])
										{
											same = true;
											break;
										}
									}
									if(!same)
									{
										to_addrs_result.push(res);
										if(to_addrs != "")
											to_addrs += ",";
										to_addrs += res;
									}
								}
							}
							
							var cc_addrs = strCc;
							var cc_addrs_arr = cc_addrs.split(",");
							var cc_addrs_result = new Array();
							cc_addrs = "";
							for(var x = 0; x < cc_addrs_arr.length; x++)
							{
								var res = retrieve(cc_addrs_arr[x]);
								if(res != null)
								{
									var same = false;
									for(var a = 0; a < cc_addrs_result.length; a++)
									{
										if(res == cc_addrs_result[a])
										{
											same = true;
											break;
										}
									}

									for(var a = 0; a < to_addrs_result.length; a++)
									{
										if(res == to_addrs_result[a])
										{
											same = true;
											break;
										}
									}
									
									if(!same)
									{
										cc_addrs_result.push(res);
										if(cc_addrs != "")
											cc_addrs += ","
										cc_addrs += res;
									}
								}
							}
							
							_$_("TO_ADDRS").value = to_addrs;
							_$_("CC_ADDRS").value = cc_addrs;
							
							_$_("SUBJECT").value = "Re: " + strSubject;
							
							_$_("CONTENT").value = (strHtmlContent!= "" ? strHtmlContent : strTextContent);
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							
							strReply = "<br><br><br><br><hr>";
							strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>From:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>To:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
							if(strCc != "")
								strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Cc:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Date:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Subject:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
							strReply += "</table>";

							strReply += _$_('richeditor').contentWindow.document.body.innerHTML;
							
							_$_("CONTENT").value = strReply;
							
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
							
						}
						else if(strType == "1")
						{
							_$_("TO_ADDRS").value = strFrom;
							_$_("SUBJECT").value = "Re: " + strSubject;

							_$_("CONTENT").value = (strHtmlContent!= "" ? strHtmlContent : strTextContent);
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							
							strReply = "<br><br><br><br><hr>";
							strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>From:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>To:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
							if(strCc != "")
								strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Cc:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Date:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
							strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>Subject:</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
							strReply += "</table>";

							strReply += _$_('richeditor').contentWindow.document.body.innerHTML;
							
							_$_("CONTENT").value = strReply;
							
							_$_('richeditor').contentWindow.document.body.innerHTML = _$_("CONTENT").value;
							_$_("CONTENT").value = _$_('richeditor').contentWindow.document.body.innerHTML;
							
						}
					}
				}
			}
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function extract_att(strID)
{
	var qUrl = "/api/extractattach.xml?ID=" + strID;
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
					var errno = responseNode.getAttribute("errno");
					
					if(errno == "0")
					{
						var strTmpname, strFilename;
						var attList = responseNode.childNodes;
						for(var i = 0; i < attList.length; i++)
						{
							if(attList.item(i).tagName == "attach")
							{
								strTmpname = attList.item(i).getAttribute("tmpname") == null ? "" : attList.item(i).getAttribute("tmpname");
								strFilename = attList.item(i).getAttribute("filename") == null ? "" : attList.item(i).getAttribute("filename");
								nAttsize = attList.item(i).getAttribute("attsize") == null ? 0 : attList.item(i).getAttribute("attsize");
								if(strTmpname != "" && strFilename != "")
								{
									show_attachs(strTmpname, strFilename, nAttsize);
								}
							}
						}
						_$_('attach_flag').innerHTML = "";
					}
					else
					{
						_$_('attach_flag').innerHTML = "<img src=\"alert.gif\">";	
					}
				}
			}
			else
			{
				_$_('attach_flag').innerHTML = "<img src=\"alert.gif\">";	
			}
		}
		else
		{
			_$_('attach_flag').innerHTML = "<img src=\"loading.gif\">";
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function del_attach(selfid, tmpname)
{
	if(del_uploaded(tmpname))
	{
		var tObj = _$_('SHOW_UPLOADEDFILES');
		tObj.deleteRow(_$_(selfid).rowIndex);	
	}
}

function del_uploaded(tmpname)
{
	var qUrl = "/api/deluploaded.xml?UPLOADEDFILE=" + tmpname;
	var xmlHttp = initxmlhttp();
	xmlHttp.open("GET", qUrl , false);
	xmlHttp.send("");
	if(xmlHttp.status== 200)
	{
		var xmldom = xmlHttp.responseXML;
		xmldom.documentElement.normalize();
		var responseNode = xmldom.documentElement.childNodes.item(0);
		if(responseNode.tagName == "response")
		{
			var errno = responseNode.getAttribute("errno");
			if(errno == "0")
			{
				return true
			}
		}
	}
	xmlHttp = null;
	return false;
}

function show_attachs(tmpname, showname, attsize)
{	
	var tObj = _$_('SHOW_UPLOADEDFILES');
	tr = tObj.insertRow(tObj.rows.length);
	tr.setAttribute("id", "file" + tr.rowIndex);
	var td1 = tr.insertCell(0);
	td1.innerHTML = "<input name=\"TMPFILENAME\" id=\"TMPFILENAME\" type=\"hidden\" value=\"" + tmpname + "\"><input type=\"image\" src=\"delete.gif\" onclick=\"del_attach('" + tr.getAttribute('id') + "','" + tmpname + "')\">";
	var td2 = tr.insertCell(1);
	td2.innerHTML = "<a href=\"/api/gettmpfile.cgi?TMPFILENAME=" + tmpname + "\" target=\"_blank\">" + TextToHTML(decodeURIComponent(showname)) + "</a>(" + ( attsize == 0 ? "0" : (Math.round(attsize/1024*100)/100 == 0 ? "0.01" : Math.round(attsize/1024*100)/100)) + "KB)";
}

function uninit()
{
	for(var x = 0; x < uploaded_file_list.length; x++)
	{
		del_uploaded(uploaded_file_list[x]);
	}
	return true;
}

function sel_alluser(sel)
{
	for(var x = 0; x < document.getElementsByName('seluser').length; x++)
	{
		document.getElementsByName('seluser')[x].checked = sel;
	}
}

function append_addrs()
{
	for(var x = 0; x < document.getElementsByName('seluser').length; x++)
	{
		if(document.getElementsByName('seluser')[x].checked == true)
		{
			isChange = true;
			toggle_onbeforeunload(isChange)
			if(_$_('ADDR_BOOK_DIV').getAttribute("who") == "1")
			{
				if(_$_('TO_ADDRS').value != "")
					_$_('TO_ADDRS').value += ", "
				_$_('TO_ADDRS').value += document.getElementsByName('seluser')[x].value;
				
			}
			else if(_$_('ADDR_BOOK_DIV').getAttribute("who") == "2")
			{
				if(_$_('CC_ADDRS').value != "")
					_$_('CC_ADDRS').value += ", "
				_$_('CC_ADDRS').value += document.getElementsByName('seluser')[x].value;
				
			}	
			else if(_$_('ADDR_BOOK_DIV').getAttribute("who") == "3")
			{
				if(_$_('BCC_ADDRS').value != "")
					_$_('BCC_ADDRS').value += ", "
				_$_('BCC_ADDRS').value += document.getElementsByName('seluser')[x].value;
			}	
		}
	}
}

function load_users(who, orderby, desc)
{
	var qUrl = "/api/listusers.xml?ORDER_BY=" + orderby + "&DESC=" + (desc == null ? '' : desc);
	var xmlHttp = initxmlhttp();

	xmlHttp.open("GET", qUrl , false);
	xmlHttp.send("");
	
	if(xmlHttp.status == 200)
	{
		var xmldom = xmlHttp.responseXML;
		xmldom.documentElement.normalize();
		var responseNode = xmldom.documentElement.childNodes.item(0);
		if(responseNode.tagName == "response")
		{
			var errno = responseNode.getAttribute("errno")
			if(errno == "0")
			{
				var strTmp;
				var userList = responseNode.childNodes;

				clear_table_without_header(_$_('USERTBL'));
					
				for(var i = 0; i < userList.length; i++)
				{
					if(userList.item(i).tagName == "user")
					{
						var image;
						if(userList.item(i).getAttribute("status") == "Active")
						{
							if(userList.item(i).getAttribute("role") == "Administrator")
								image = "admin.gif";
							else
							{
								if(userList.item(i).getAttribute("type") == "Group")
									image = "group.gif";
								else		
									image = "member.gif";
							}
						}
						else
						{
							if(userList.item(i).getAttribute("type") == "Group")
								image = "inactive_group.gif";
							else		
								image = "inactive_member.gif";
						}

						tr = _$_('USERTBL').insertRow(_$_('USERTBL').rows.length);

						
						var td0 = tr.insertCell(0);
						td0.valign="middle";
						td0.align="center";
						td0.height="22";
						setStyle(td0, "TD.gray");
						td0.innerHTML = "<input type=\"checkbox\" name=\"seluser\" id=\"seluser\" value=\"" + userList.item(i).getAttribute("name") + "@" + userList.item(i).getAttribute("domain") + "\">";

						var td1 = tr.insertCell(1);
						td1.valign="middle";
						td1.align="center";
						td1.height="22";
						setStyle(td1, "TD.gray");
						td1.innerHTML = "<img src=\""+image +"\" />";
						
						var td2 = tr.insertCell(2);
						td2.valign="middle";
						td2.align="left";
						td2.height="22";
						setStyle(td2, "TD.gray");
						td2.innerHTML = userList.item(i).getAttribute("name");

						var td3 = tr.insertCell(3);
						td3.valign="middle";
						td3.align="left";
						td3.height="22";
						setStyle(td3, "TD.gray");
						td3.innerHTML = userList.item(i).getAttribute("alias");
					}
				}
			}
		}
	}
}

function showbook(who, orderby)
{	
	sel_alluser(false);
	
	$("#ADDR_BOOK_DIV").dialog({
	  height: 500,
	  width: 600,
	  modal: false,
	  title: 'Address Book',
	  buttons: {                  
		  "Ok": function() {
			  append_addrs();
			  $(this).dialog("close");
			  },
		  "Close": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	
	_$_('ADDR_BOOK_DIV').setAttribute("who", who);
	
	_$_('selalluser').checked = false;
	
	if(_$_('ADDR_BOOK_DIV').getAttribute("loaded") != "true")
	{
		_$_('USERTBL').setAttribute('orderby', orderby);
		load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
		
		_$_('ADDR_BOOK_DIV').setAttribute("loaded", "true");
	}
	
	/*_$_('ADDR_BOOK_DIV').style.display = "block";

	_$_('ADDR_BOOK_DIV').style.left = x;
	_$_('ADDR_BOOK_DIV').style.top = y;*/
}

function do_copy_mail(mid, todirs)
{
	var qUrl = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
	var xmlHttp = initxmlhttp();
	
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = window.opener._$_(strmid);

		if(trid == null)
			return false;
		
		if (xmlHttp.readyState == 4 && xmlHttp.status== 200)
		{	
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					if(window.opener && window.opener.refresh)
							window.opener.refresh();
					$( "PROCESSING_DIV" ).dialog("close");
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Processing, please wait...";
			$( "#PROCESSING_DIV" ).dialog();
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
	
	return true;
}

function copy_mail(todir)
{	
	if(_$_('DRAFTID').value == null || _$_('DRAFTID').value == "" || _$_('DRAFTID').value == -1)
	{
		alert("The draft is unsaved yet");
		return;
	}
	do_copy_mail(_$_('DRAFTID').value, todir);
}

function do_move_mail(mid, todirs)
{
	var qUrl = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
	var xmlHttp = initxmlhttp();
	
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = window.opener._$_(strmid);

		if(trid == null)
			return false;
		
		if (xmlHttp.readyState == 4 && xmlHttp.status== 200)
		{
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					$( "#PROCESSING_DIV" ).dialog("close");
					
					window.opener.delete_maillist_row(mid);

					if(window.opener.refresh)
							window.opener.refresh();
				}
				else
				{
				}
			}
		}
		else
		{
			_$_('PROCESSING_TBL').rows[0].cells[0].innerHTML = "Processing, please wait...";
			$( "#PROCESSING_DIV" ).dialog();
		}

	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
			
	return true;
}

function move_mail(todir)
{	
	if(_$_('DRAFTID').value == null || _$_('DRAFTID').value == "" || _$_('DRAFTID').value == -1)
	{
		alert("The draft is unsaved yet");
		return;
	}
	do_copy_mail(_$_('DRAFTID').value, todir);
}

function init_editor()
{
	if(navigator.appName != "Netscape")
	{
		document.frames['richeditor'].document.designMode = 'on';
	}
	else
	{
		document.getElementById('richeditor').contentWindow.document.designMode = 'on';
		document.getElementById('richeditor').contentWindow.document.body.contentEditable = 'true';
	}
}

var isChange = false;

function ok_copy_mail()
{
	var strDirs = "";
	for(var x = 0; x < document.getElementsByName('seldir1').length; x++)
	{
		if(document.getElementsByName('seldir1')[x].checked == true )
		{
			strDirs += document.getElementsByName('seldir1')[x].value + "*";
		}
	}
	if(strDirs == "")
	{
		alert('Not choose any folder yet');
		return -1;
	}
	copy_mail(strDirs);
	
	return 0
}

function ok_move_mail()
{
	var strDirs = "";
	for(var x = 0; x < document.getElementsByName('seldir2').length; x++)
	{
		if(document.getElementsByName('seldir2')[x].checked == true )
		{
			strDirs += document.getElementsByName('seldir2')[x].value + "*";
		}
	}
	if(strDirs == "")
	{
		alert('Not choose any folder yet');
		return -1;
	}
	move_mail(strDirs);
	
	return 0;
}

function show_copy_mail_dir_div()
{
	$("#DIRS_DIV1").dialog({
	  height: 500,
	  width: 300,
	  modal: false,
	  title: 'Folder',
	  buttons: {                  
		  "Ok": function() {
			  if(ok_copy_mail() != -1)
				$(this).dialog("close");
			  },
		  "Cancel": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	_$_('DIRS_DIV2').style.display = "none";
	
	var Pos = GetObjPos(_$_('COPYMAIL'))
	show_dirs(_$_('DIRS_DIV1'), _$_('DIRTBL1'), Pos.x, Pos.y + Pos.h + 2, "seldir1");
}

function show_move_mail_dir_div()
{
	$("#DIRS_DIV2").dialog({
	  height: 500,
	  width: 300,
	  modal: false,
	  title: '文件夹',
	  buttons: {                  
		  "确定": function() {
			  if(ok_move_mail() != -1)
				$(this).dialog("close");
			  },
		  "取消": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	_$_('DIRS_DIV1').style.display = "none";
	
	var Pos = GetObjPos(_$_('MOVEMAIL'))
	show_dirs(_$_('DIRS_DIV2'), _$_('DIRTBL2'), Pos.x, Pos.y + Pos.h + 2, "seldir2");
}

function sort_users(who, orderby)
{
	_$_('selalluser').checked = false;
	
	_$_('USERTBL').setAttribute('orderby', orderby);

	clear_table_without_header(_$_('USERTBL'));

	if(_$_('USERTBL').getAttribute(orderby) == 'true')
		_$_('USERTBL').setAttribute(orderby, 'false');
	else
		_$_('USERTBL').setAttribute(orderby, 'true');

	load_users(who, _$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
}
