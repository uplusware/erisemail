	function delmail(force)
	{ 
		
		var xmlHttp = initxmlhttp();
		xmlHttp.open("GET", qUrl , false);
		xmlHttp.send();
		
		if (xmlHttp.status== 200)
		{
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					window.location = "listmails.html?DIRID=" + Request.QueryString('DIRID');
				}
			}
		}
	}

	function do_delmail(x)
	{ 
		var xmlHttp = initxmlhttp();
		xmlHttp.open("GET", qUrl , false);
		xmlHttp.send();
		
		if (xmlHttp.status== 200)
		{
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					window.location = "listmails.html?DIRID=" + Request.QueryString('DIRID');
				}
			}
		}
	}
	
	function remail()
	{
		var url = "writemail.html?TYPE=1&ID=" + Request.QueryString('ID');
		window.location= url;	
	}
	
	function remail_all()
	{
		var url = "writemail.html?TYPE=2&ID=" + Request.QueryString('ID');
		window.location= url;	
	}
	
	function forward_mail()
	{
		var url = "writemail.html?TYPE=3&ID=" + Request.QueryString('ID');
		window.location= url;	
	}
	
	function flag_mail(flag)
	{
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
					if(errno == "0" || errno == 0)
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
	
	function do_copy_mail(mid, todirs)
	{
		var qUrl = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
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
	
	function copy_mail(todir)
	{	
		do_copy_mail(Request.QueryString('ID'), todir);
	}

	function do_move_mail(mid, todirs)
	{
		var qUrl = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
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
	
	function move_mail(todir)
	{	
		do_copy_mail(Request.QueryString('ID'), todir);
	}
	
	function read_mail(strID)
	{
		var qUrl = "/api/readmail.xml?ID=" + strID;
		var xmlHttp = initxmlhttp();
	    xmlHttp.onreadystatechange = function()
		{
			if (xmlHttp.readyState == 4)
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
							var strTextContent, strHtmlContent, strFrom, strTo, strCc, strBcc, strDate, strSubject, strAttach;
							strFrom = "";
							strTo = "";
							strCc = "";
							strDate = "";
							strSubject = "";
							strAttach = "";
							strTextContent = "";
							strCalendarContent = "";
							strHtmlContent = "";
							var bodyList = responseNode.childNodes;
							for(var i = 0; i < bodyList.length; i++)
							{
								if(bodyList.item(i).tagName == "from")
								{
									strFrom = bodyList.item(i).childNodes[0]== null ? "" : bodyList.item(i).childNodes[0].nodeValue;
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
									
									_$_('EVENT_SUBJECT').innerHTML = strSubject;
								}
								else if(bodyList.item(i).tagName == "attach")
								{
									strAttach = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
								}
								else if(bodyList.item(i).tagName == "text_content")
								{
									strTextContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue; 
								}
								else if(bodyList.item(i).tagName == "html_content")
								{
									strHtmlContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
								}
								else if(bodyList.item(i).tagName == "calendar_content")
								{
									strCalendarContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
								}
							}
							
							if(strCc == "")
							{
								_$_("cccol").style.display = "none";	
							}
							if(strAttach == "")
							{
								_$_("attcol").style.display = "none";	
							}
							
							var strHrefAttach = "";
							var attachs = strAttach.split(";");
							strHrefAttach = "<table border=\"0\" cellpadding=\"2\" cellspacing=\"2\"><tr align=\"left\" valign=\"middle\">";
							for(var x = 0; x < attachs.length; x++)
							{
								if(attachs[x] !="")
								{
									strHrefAttach += "<td><img src=\"attach.gif\"></td><td style=\"white-space:nowrap\"><a href=\"/api/attachment.cgi?ID="+ strID +"&FILENAME=" + encodeURIComponent(attachs[x]) + "\" target=\"_blank\">" + TextToHTML(attachs[x]) + "</a></td>";
								}
							}
							strHrefAttach +="</tr></table>";
							_$_("mailfrom").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strFrom) + "\" readonly>";
							_$_("mailto").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strTo) + "\" readonly>";
							_$_("mailcc").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strCc) + "\" readonly>";
							_$_("maildate").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strDate) + "\" readonly>";
							_$_("mailsubject").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + (strSubject == "" ? "鏃犱富棰? : TextToHTML(strSubject)) + "\" readonly>";
							_$_("mailattach").innerHTML = strHrefAttach == "" ? "<img src=\"pad.gif\">" : strHrefAttach;

							var strrpl = "/api/attachment.cgi?ID=";
							strrpl += strID;
							strrpl += "&CONTENTID=";
							
							var regS1 = new RegExp("cid:","gi");
							strHtmlContent = strHtmlContent.replace(regS1, strrpl);
							
							_$_("mailcontent").style.width = _$_('mailcontentframe').offsetWidth - 5;
							_$_("mailcontent").style.height = _$_('mailcontentframe').offsetHeight - 5;
							
							_$_("mailcontent").innerHTML = strHtmlContent != "" ? strHtmlContent : (strCalendarContent !="" ? TextToHTML(strCalendarContent) : TextToHTML(strTextContent));
							
							if(_$_('mailattach').scrollWidth > _$_('mailattach').clientWidth)
							{
								_$_('MOVE_LEFT').src = "moveleft2.gif";
								_$_('MOVE_LEFT').disabled = true;
								
								_$_('MOVE_RIGHT').src = "moveright.gif";
								_$_('MOVE_RIGHT').disabled = false;
								
								_$_('MOVE_LEFT').style.display = "block";
								_$_('MOVE_RIGHT').style.display = "block";

								_$_('MOVE_LEFT_TD').style.display = "block";
								_$_('MOVE_RIGHT_TD').style.display = "block";
							}
							else
							{
								_$_('MOVE_LEFT').style.display = "none";
								_$_('MOVE_RIGHT').style.display = "none";

								_$_('MOVE_LEFT_TD').style.display = "none";
								_$_('MOVE_RIGHT_TD').style.display = "none";
							}
						}
						else
						{
							_$_("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>鍔犺浇澶辫触</td></tr></table>";
						}
					}
				}
				else
				{
					_$_("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>鍔犺浇澶辫触</td></tr></table>";
				}
			}
			else
			{
				_$_("mailfrom").innerHTML = "<img src=\"loading.gif\">";
				_$_("mailto").innerHTML = "<img src=\"loading.gif\">";
				_$_("mailcc").innerHTML = "<img src=\"loading.gif\">";
				_$_("maildate").innerHTML = "<img src=\"loading.gif\">";
				_$_("mailsubject").innerHTML = "<img src=\"loading.gif\">";
				_$_("mailattach").innerHTML = "<img src=\"loading.gif\">";
				_$_("mailcontent").innerHTML = "<img src=\"loading.gif\">";
			}
		}
		xmlHttp.open("GET", qUrl , true);
		xmlHttp.send("");
	}

	function refresh()
	{

	}
	
	function init()
	{
		window.parent.parent._$_('MAILBAR').style.display = "none";
		window.parent.parent._$_('DIRBAR').style.display = "none";
		window.parent.parent._$_('CALBAR').style.display = "none";
		window.parent.parent._$_('NULLBAR').style.display = "block";
		
		read_mail(Request.QueryString('ID'));
	}

	function move_left()
	{
		if(_$_('MOVE_LEFT').disabled == false)
		{			
			_$_('mailattach').scrollLeft -= (_$_('mailattach').scrollLeft > 8 ? 8 : _$_('mailattach').scrollLeft);
			
			if(_$_('mailattach').scrollLeft <= 0)
			{
				_$_('mailattach').scrollLeft = 0;

				_$_('MOVE_LEFT').src = "moveleft2.gif";
				_$_('MOVE_LEFT').disabled = true;
			}

			if(_$_('mailattach').scrollLeft < (_$_('mailattach').scrollWidth - _$_('mailattach').clientWidth))
			{
				_$_('MOVE_RIGHT').src = "moveright.gif";
				_$_('MOVE_RIGHT').disabled = false;
			}

			if(_$_('MOVE_LEFT').getAttribute("down") == "true")
			{
				setTimeout('left_timer()', 50);
			}
		
		}
	}

	function move_right()
	{
		if(_$_('MOVE_RIGHT').disabled == false)
		{
			
			if( _$_('mailattach').scrollLeft < ( _$_('mailattach').scrollWidth - _$_('mailattach').clientWidth))
			{
				_$_('mailattach').scrollLeft += (( _$_('mailattach').scrollWidth - _$_('mailattach').clientWidth - _$_('mailattach').scrollLeft ) > 8 ? 8 : ( _$_('mailattach').scrollWidth - _$_('mailattach').clientWidth - _$_('mailattach').scrollLeft ));
			}
			
			if(_$_('mailattach').scrollLeft >= (_$_('mailattach').scrollWidth - _$_('mailattach').clientWidth))
			{
				
				_$_('mailattach').scrollLeft = _$_('mailattach').scrollWidth - _$_('mailattach').clientWidth;

				_$_('MOVE_RIGHT').src = "moveright2.gif";
				_$_('MOVE_RIGHT').disabled = true;				
			}
			
			if(_$_('mailattach').scrollLeft > 0)
			{
				_$_('MOVE_LEFT').src = "moveleft.gif";
				_$_('MOVE_LEFT').disabled = false;
			}

			if(_$_('MOVE_RIGHT').getAttribute("down") == "true")
			{
				setTimeout('right_timer()', 50);
			}
		
		}
	}


	function left_timer()
	{
		if(_$_('MOVE_LEFT').getAttribute("down") == "true")
		{
			move_left();
		}
	}

	function right_timer()
	{
		if(_$_('MOVE_RIGHT').getAttribute("down") == "true")
		{
			move_right();
		}
	}
	
	function close_mail_detail()
	{
		window.parent.close_mail_detail();	
		
		window.parent.parent._$_('MAILBAR').style.display = "block";
		window.parent.parent._$_('DIRBAR').style.display = "none";
		window.parent.parent._$_('CALBAR').style.display = "none";
		window.parent.parent._$_('NULLBAR').style.display = "none";
		
	}