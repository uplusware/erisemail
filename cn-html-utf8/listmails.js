function do_delmail(mid)
{
	var qUrl = "/api/delmail.xml?MAILID=" + mid;
	
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = _$_(strmid);

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
					_$_('MAILTBL').deleteRow(trid.rowIndex);
					load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
				}
				else
				{
					trid.cells[3].style.textDecoration = "";
					trid.cells[4].style.textDecoration = "";
					trid.cells[5].style.textDecoration = "";
					trid.cells[6].style.textDecoration = "";
				}
			}
			
		}
		else
		{
			trid.cells[3].style.textDecoration="line-through";
			trid.cells[4].style.textDecoration="line-through";
			trid.cells[5].style.textDecoration="line-through";
			trid.cells[6].style.textDecoration="line-through";
		}

	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function ontimer()
{
	load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
	
	setTimeout('ontimer()', 30000);
}

function do_trashmail(mid)
{
	var qUrl = "/api/trashmail.xml?MAILID=" + mid;
	var xmlHttp = initxmlhttp();
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = _$_(strmid);

		if(trid == null)
			return false;
			
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
					var strmid = "mailtr" + mid;
					var trid = _$_(strmid); 
					_$_('MAILTBL').deleteRow(trid.rowIndex);

					load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
				}
				else
				{
	
					trid.cells[3].style.textDecoration = "";
					trid.cells[4].style.textDecoration = "";
					trid.cells[5].style.textDecoration = "";
					trid.cells[6].style.textDecoration = "";
				}
			}
		}
		else
		{
			trid.cells[3].style.textDecoration="line-through";
			trid.cells[4].style.textDecoration="line-through";
			trid.cells[5].style.textDecoration="line-through";
			trid.cells[6].style.textDecoration="line-through";
		}

	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}
	
function delmail(force)
{
	var sel_num = 0
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			sel_num++;
		}
		
	}
	if(sel_num == 0)
	{
		alert('没有选定邮件');
	}
	else
	{
		for(var x = 0; x < document.getElementsByName('sel').length; x++)
		{
			if(document.getElementsByName('sel')[x].checked == true )
			{
				if(force ==  true)
				{
					do_delmail(document.getElementsByName('sel')[x].value);
				}
				else
				{
					if(Request.QueryString('TOP_USAGE') == "trash")
						do_delmail(document.getElementsByName('sel')[x].value);
					else
						do_trashmail(document.getElementsByName('sel')[x].value);
				}
			}
		}
	}
}

function refresh()
{
	load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
}

function remail()
{
	var sel_num = 0
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			sel_num++;
		}
		
	}
	if(sel_num > 1)
	{
		alert("一次只能回复一个邮件");
	}
	else if(sel_num == 0)
	{
		alert('没有选定邮件');
	}
	else
	{
		for(var x = 0; x < document.getElementsByName('sel').length; x++)
		{
			if(document.getElementsByName('sel')[x].checked == true )
			{
				var url = "writemail.html?TYPE=1&ID=" + document.getElementsByName('sel')[x].value;
				show_mail_detail(url);
				break;
			}
			
		}
	}
}

function sel_all(sel)
{
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		document.getElementsByName('sel')[x].checked = sel;
	}
}

function remail_all()
{
	var sel_num = 0
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			sel_num++;
		}
		
	}
	if(sel_num > 1)
	{
		alert("一次只能回复一个邮件");
	}
	else if(sel_num == 0)
	{
		alert('没有选定邮件');
	}
	else
	{
		for(var x = 0; x < document.getElementsByName('sel').length; x++)
		{
			if(document.getElementsByName('sel')[x].checked == true )
			{
				var url = "writemail.html?TYPE=2&ID=" + document.getElementsByName('sel')[x].value;
				show_mail_detail(url);
				break;
			}
			
		}
	}
}

function forward_mail()
{
	var sel_num = 0
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			sel_num++;
		}
		
	}
	if(sel_num > 1)
	{
		alert("一次只能转发一个邮件");
	}
	else if(sel_num == 0)
	{
		alert('没有选定邮件');
	}
	else
	{
		for(var x = 0; x < document.getElementsByName('sel').length; x++)
		{
			if(document.getElementsByName('sel')[x].checked == true )
			{
				var url = "writemail.html?TYPE=3&ID=" + document.getElementsByName('sel')[x].value;
				show_mail_detail(url);
				break;
			}
			
		}
	}
}

function do_flag_mail(flag, mid)
{
	var fid = "flag" + mid;
	
	if(!_$_(fid))
		return;
	
	if(flag ==  true)
		strFlag="yes";
	else
		strFlag = "no";
	
	var oldsrc = _$_(fid).src;
	
	var qUrl = "/api/flagmail.xml?ID=" + mid + "&FLAG=" + strFlag;
	
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
					var strmid = "mailtr" + mid;
					var trid = _$_(strmid);
					if(flag == true)
					{
						trid.cells[1].setAttribute("flag", "yes");
						_$_(fid).src = "hot.gif";
					}
					else
					{
						trid.cells[1].setAttribute("flag", "no");
						_$_(fid).src = "unhot.gif";
					}
					
				}
				else
				{
					
					_$_(fid).src = oldsrc;
				}
			}
		}
		else
		{
			_$_(fid).src = "loading.gif";
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function flag_mail(flag)
{		
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			do_flag_mail(flag, document.getElementsByName('sel')[x].value);
		}
	}
}

function do_seen_mail(flag, mid)
{
	var seenid = "seen" + mid;
	if(!_$_(seenid))
		return;

	var oldsrc = _$_(seenid).src;
	
	if(flag ==  true)
		strFlag="yes";
	else
		strFlag = "no";
	
	var qUrl = "/api/seenmail.xml?ID=" + mid + "&SEEN=" + strFlag;
	
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
					var strmid = "mailtr" + mid;
					var trid = _$_(strmid);
					
					if(flag == true)
					{
						trid.style.fontWeight="normal"
						_$_(seenid).src = "seen.gif";
					}
					else
					{
						trid.style.fontWeight="bold"
						_$_(seenid).src = "unseen.gif";
					}
				}
				else
				{
					
					_$_(seenid).src = oldsrc;
				}
			}
		}
		else
		{
			_$_(seenid).src = "loading.gif";
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function seen_mail(flag)
{		
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			do_seen_mail(flag, document.getElementsByName('sel')[x].value);
		}
	}
}

function do_copy_mail(mid, todir)
{
	var qUrl = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todir;
	var xmlHttp = initxmlhttp();
	
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = _$_(strmid);

		if(trid == null)
			return false;
		
		if (xmlHttp.readyState == 4 && xmlHttp.status== 200)
		{
			trid.cells[3].style.color = "";
			trid.cells[4].style.color = "";
			trid.cells[5].style.color = "";
			trid.cells[6].style.color = "";
			
			var xmldom = xmlHttp.responseXML;
			xmldom.documentElement.normalize();
			var responseNode = xmldom.documentElement.childNodes.item(0);
			if(responseNode.tagName == "response")
			{
				var errno = responseNode.getAttribute("errno")
				if(errno == "0" || errno == 0)
				{
					load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
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
			trid.cells[3].style.color = "#FF0000";
			trid.cells[4].style.color = "#FF0000";
			trid.cells[5].style.color = "#FF0000";
			trid.cells[6].style.color = "#FF0000";
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
	return true;
}

function copy_mail(todirs)
{	
	if(document.getElementsByName('sel').length == 0)
	{
		alert('请选择要移动的邮件');
		return;
	}
	
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			do_copy_mail(document.getElementsByName('sel')[x].value, todirs);
		}
	}
}

function do_move_mail(mid, todir)
{	
	var qUrl = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todir;
	var xmlHttp = initxmlhttp();
	
	xmlHttp.onreadystatechange = function()
	{
		var strmid = "mailtr" + mid;
		var trid = _$_(strmid);

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
					var strmid = "mailtr" + mid;
					var trid = _$_(strmid);
					_$_('MAILTBL').deleteRow(trid.rowIndex);

					load_mails(Request.QueryString('DIRID'), _$_('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
				}
				else
				{
					trid.cells[3].style.color = "";
					trid.cells[4].style.color = "";
					trid.cells[5].style.color = "";
					trid.cells[6].style.color = "";
				}
			}
		}
		else
		{
			trid.cells[3].style.color = "#C0C0C0";
			trid.cells[4].style.color = "#C0C0C0";
			trid.cells[5].style.color = "#C0C0C0";
			trid.cells[6].style.color = "#C0C0C0";
		}

	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
	
	return true;
}

function move_mail(todir)
{	
	if(document.getElementsByName('sel').length == 0)
	{
		alert('请选择要移动的邮件');
		return;
	}
	
	for(var x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == true )
		{
			do_move_mail(document.getElementsByName('sel')[x].value, todir);
		}
	}
}

var num_of_page = 20;

function get_page_num(dirid, npage)
{
	var xmlHttp = initxmlhttp();
	if(!xmlHttp)
		return;
	if(!dirid)
		dirid = "";
	var qUrl = "/api/getmailnum.xml?DIRID=" + dirid;
	

	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState == 4)
		{
			if(xmlHttp.status== 200)
			{
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var mailnum;
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0" || errno == 0)
					{
						var strTmp;
						var countList = responseNode.childNodes;
						for(var i = 0; i < countList.length; i++)
						{
							if(countList.item(i).tagName == "count")
							{
								mailnum = countList.item(i).childNodes[0] == null ? "" : countList.item(i).childNodes[0].nodeValue;
								
								if(mailnum % num_of_page == 0)
									_$_("PAGE_NUM").value = (mailnum/num_of_page);
								else
									_$_("PAGE_NUM").value = ((mailnum - mailnum % num_of_page)/num_of_page + 1);
									
								_$_("PAGE").value = (parseInt(npage) + 1) + "/" + _$_("PAGE_NUM").value;

								update_page_button();
							}
						}
					}
				}
			}
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function get_insert_pos(time, uniqid)
{	
	var i = 0;
	var tmp = 0;
	if(_$_('MAILTBL').rows.length == 0)
		return 0;
	
	for(i = 0; i < _$_('MAILTBL').rows.length; i++)
	{
		if(_$_('MAILTBL').rows[i].getAttribute("uniqid") == uniqid)
		{
			return -1;
		}
		else
		{
			if(parseInt(time) > parseInt(_$_('MAILTBL').rows[i].getAttribute("sort")))
			{
				return i;
			}
		}
	}
	return i;
}

function check_selall()
{
	var x = 0;
	for(x = 0; x < document.getElementsByName('sel').length; x++)
	{
		if(document.getElementsByName('sel')[x].checked == false )
		{
			break;	
		}
	}
	if(document.getElementsByName('sel').length == 0)
	{
		if(_$_('selall') != null)
			_$_('selall').checked = false;
	}
	else
	{
		if(_$_('selall') != null)
		{
			if(x == document.getElementsByName('sel').length)
				_$_('selall').checked = true;
			else
				_$_('selall').checked = false;
		}
	}
}

function update_mail_status(mid, flag, seen)
{	
	var fid = "flag" + mid;
	var strmid = "mailtr" + mid;
	var seenid = "seen" + mid;
	
	if(!_$_(fid) || !_$_(strmid) || !_$_(seenid))
		return;
		
	var trid = _$_(strmid);
	if(flag == true)
	{
		trid.cells[1].setAttribute("flag", "yes");
		_$_(fid).src = "hot.gif";
	}
	else
	{
		trid.cells[1].setAttribute("flag", "no");
		_$_(fid).src = "unhot.gif";
	}
	
	if(seen == true)
	{
		trid.style.fontWeight="normal"
		_$_(seenid).src = "seen.gif";
	}
	else
	{
		trid.style.fontWeight="bold"
		_$_(seenid).src = "unseen.gif";
	}
					
}

function update_mail_flag(mid, flag)
{	
	var fid = "flag" + mid;
	var strmid = "mailtr" + mid;
	
	if(!_$_(fid) || !_$_(strmid))
		return;
		
	var trid = _$_(strmid);
	if(flag == true)
	{
		trid.cells[1].setAttribute("flag", "yes");
		_$_(fid).src = "hot.gif";
	}
	else
	{
		trid.cells[1].setAttribute("flag", "no");
		_$_(fid).src = "unhot.gif";
	}				
}

function update_mail_seen(mid, seen)
{	
	var strmid = "mailtr" + mid;
	var seenid = "seen" + mid;
	
	if(!_$_(strmid) || !_$_(seenid))
		return;
		
	var trid = _$_(strmid);		
	if(seen == true)
	{
		trid.style.fontWeight="normal"
		_$_(seenid).src = "seen.gif";
	}
	else
	{
		trid.style.fontWeight="bold";
		_$_(seenid).src = "unseen.gif";
	}
}

function delete_maillist_row(mid)
{
	var strmid = "mailtr" + mid;
	var trid = _$_(strmid);
	_$_('MAILTBL').deleteRow(trid.rowIndex);
					
}

function load_mails(dirid, npage, strUsage)
{
	var xmlHttp = initxmlhttp();
	if(!xmlHttp)
		return;
	if(!dirid)
		dirid = "";
	if(!npage)
		npage = 0;
	if(!strUsage)
		strUsage = "common";
	
	get_page_num(dirid, npage);
	
	var row = num_of_page;
	var beg = npage * num_of_page ;
	
	var qUrl = "/api/listmails.xml?DIRID=" + dirid + "&BEG=" + beg + "&ROW=" + row;
	
	xmlHttp.onreadystatechange = function()
	{
		if (xmlHttp.readyState == 4)
		{
			if(xmlHttp.status == 200)
			{
				_$_('STATUS').style.display = "none";
				
				var xmldom = xmlHttp.responseXML;
				xmldom.documentElement.normalize();
				var responseNode = xmldom.documentElement.childNodes.item(0);
				if(responseNode.tagName == "response")
				{
					var errno = responseNode.getAttribute("errno")
					if(errno == "0" || errno == 0)
					{							
						var strTmp;
						var mailList = responseNode.childNodes;
						
						for(var i = 0; i < mailList.length; i++)
						{
							if(mailList.item(i).tagName == "mail")
							{
								var isbold, seen, flaged;
								
								if(mailList.item(i).getAttribute("seen") == "no")
								{
									seen = "<img id=\"seen" + mailList.item(i).getAttribute("id") + "\" src=\"unseen.gif\">";
									isbold = true;
								}
								else
								{
									seen = "<img id=\"seen" + mailList.item(i).getAttribute("id") + "\" src=\"seen.gif\">";
									isbold = false;
								}
								
								if(mailList.item(i).getAttribute("flaged") == "yes")
									flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"hot.gif\">";
								else
									flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"unhot.gif\">";
								
								if(mailList.item(i).getAttribute("subject") == "" || mailList.item(i).getAttribute("subject") == null)
									strSubject = "无主题";
								else
									strSubject = mailList.item(i).getAttribute("subject");

								strSubject = abbrString(strSubject, 32);
								
								while(_$_('MAILTBL').rows.length > num_of_page)
								{
									_$_('MAILTBL').deleteRow(_$_('MAILTBL').rows.length - 1 );
								}

								var pos = get_insert_pos(mailList.item(i).getAttribute("sort"), mailList.item(i).getAttribute("uniqid"));
								
								if(pos == -1)
								{
									var bFlag, bSeen;
									
									if(mailList.item(i).getAttribute("flaged") == "no")
									{
										bFlag = false;
									}
									else
									{
										bFlag = true;
									}
									
									if(mailList.item(i).getAttribute("seen") == "yes")
									{
										bSeen = true;	
									}	
									else
									{
										bSeen = false;	
									}
									
									update_mail_status(mailList.item(i).getAttribute("id"), bFlag, bSeen);
									continue;
								}
								tr = _$_('MAILTBL').insertRow(pos);

								if(i%2 == 1)
								{
									tr.style.backgroundColor = "#F0F0F0";
								}
								else
								{
									tr.style.backgroundColor = "#FFFFFF";
								}
								
								tr.setAttribute("id", "mailtr" + mailList.item(i).getAttribute("id"));
								tr.setAttribute("mid", mailList.item(i).getAttribute("id"));
								tr.setAttribute("time" , mailList.item(i).getAttribute("time"));
								tr.setAttribute("sort" , mailList.item(i).getAttribute("sort"));
								tr.setAttribute("uniqid" , mailList.item(i).getAttribute("uniqid"));
								
								var gotourl = "";
								
									if(strUsage == "drafts")
										gotourl = "writemail.html?GPATH="+(Request.QueryString('GPATH') == null? encodeURIComponent("/收件箱") : Request.QueryString('GPATH'))+"&TOP_USAGE="+ (Request.QueryString('TOP_USAGE') == null ? "common" : Request.QueryString('TOP_USAGE')) + "&DIRID=" + dirid + "&ID="+ mailList.item(i).getAttribute("id") +"&TYPE=" + 4;
									else 
										gotourl = "readmail.html?GPATH="+(Request.QueryString('GPATH') == null? encodeURIComponent("/收件箱") : Request.QueryString('GPATH'))+"&TOP_USAGE="+ (Request.QueryString('TOP_USAGE') == null ? "common" : Request.QueryString('TOP_USAGE')) + "&DIRID=" + dirid + "&ID="+ mailList.item(i).getAttribute("id");
								
								tr.onmouseover = function()
								{
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('focusbg.gif')";
								}
								
								tr.onmouseout = function()
								{
									this.style.backgroundImage= this.getAttribute("imagesrc");
								}
								
								if(isbold == true)
									tr.style.fontWeight="bold";
								else
									tr.style.fontWeight="normal";
								
								var td0 = tr.insertCell(0);
								
								td0.valign="middle";
								td0.align="center";
								td0.height="26";
								td0.width="22";
								td0.innerHTML = "<input type=\"checkbox\" name=\"sel\" onclick='check_selall()' value=\""+ mailList.item(i).getAttribute("id") +"\">";

								var td1 = tr.insertCell(1);
								td1.valign="middle";
								td1.align="center";
								td1.height="26";
								td1.width="22";
								td1.setAttribute("mid", mailList.item(i).getAttribute("id"));
								td1.setAttribute("flag", mailList.item(i).getAttribute("flaged"));
								td1.innerHTML = flaged;

								td1.onclick = function()
								{
									if(this.getAttribute("flag") == "yes")
									{
										do_flag_mail(false, this.getAttribute("mid"));
									}
									else if(this.getAttribute("flag") == "no")
									{
										do_flag_mail(true, this.getAttribute("mid"));
									}
								}

								td1.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td1.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}
								
								var td2 = tr.insertCell(2);
								td2.valign="middle";
								td2.align="center";
								td2.height="26";
								td2.width="22";
								td2.setAttribute("selflink", gotourl);
								td2.innerHTML = seen;
								td2.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td2.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td2.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}

								var td3 = tr.insertCell(3);
								td3.valign="middle";
								td3.align="center";
								td3.height="26";
								td3.width="22";
								td3.setAttribute("selflink", gotourl);
								
								td3.innerHTML = (parseInt(mailList.item(i).getAttribute("attachcount")) > 0) ? "<img src=\"attach.gif\">" : "<img src=\"pad.gif\">"

								td3.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td3.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td3.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}
								
								var td4 = tr.insertCell(4);
								td4.valign="middle";
								td4.align="left";
								td4.height="26";
								td4.width="120";
								td4.setAttribute("selflink", gotourl);
								
								td4.innerHTML = TextToHTML(mailList.item(i).getAttribute("sender"));

								td4.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td4.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td4.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}
								
								var td5 = tr.insertCell(5);
								td5.valign="middle";
								td5.height="26";
								td5.setAttribute("selflink", gotourl);
								td5.innerHTML = TextToHTML(strSubject);
								td5.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td5.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td5.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}

								var td6 = tr.insertCell(6);
								td6.valign="middle";
								td6.align="right";
								td6.height="26";
								td6.width="80";
								td6.setAttribute("selflink", gotourl);
								td6.innerHTML = Math.round(mailList.item(i).getAttribute("size")/1024*100)/100 +"KB";
								td6.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td6.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td6.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}
								
								var td7 = tr.insertCell(7);
								td7.valign="middle";
								td7.align="center";
								td7.height="26";
								td7.width="150";
								td7.setAttribute("selflink", gotourl);
								td7.innerHTML = mailList.item(i).getAttribute("time");
								
								td7.onclick = function()
								{
									show_mail_detail(this.getAttribute("selflink"));
									update_mail_seen(this.parentNode.getAttribute("mid"), true);
									
									this.parentNode.style.backgroundImage= this.parentNode.getAttribute("imagesrc");
								}

								td7.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
								}
								
								td7.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
								}
							}
						}
						check_selall();						
					}
				}
			}
			else
			{
				_$_('STATUS').style.display = "block";
			  _$_('STATUS').innerHTML = "<center><img src=\"alert.gif\"></center>";

			}
		}
		else
		{
			_$_('STATUS').style.display = "block";
			_$_('STATUS').innerHTML = "<center><img src=\"waiting.gif\"></center>";

		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function init()
{				
	window.parent._$_('REMAIL').src="remail.gif";
	window.parent._$_('REMAIL').disabled = false;
	
	window.parent._$_('RETOALL').src="retoall.gif";
	window.parent._$_('RETOALL').disabled = false;

	window.parent._$_('FORWARD').src="forward.gif";
	window.parent._$_('FORWARD').disabled = false;
	
	window.parent._$_('MAILBAR').style.display = "block";
	window.parent._$_('DIRBAR').style.display = "none";
	window.parent._$_('CALBAR').style.display = "none";
	window.parent._$_('READCALBAR').style.display = "none";
	window.parent._$_('NULLBAR').style.display = "none";
	
	_$_('GLOBALPATH').innerHTML = (Request.QueryString('GPATH') == null? ("/收件箱") : decodeURIComponent(Request.QueryString('GPATH')));
	
	ontimer();
}

function update_page_button()
{
	if(_$_('PAGE_CUR').value < 0)
	{
		_$_('PAGE_CUR').value = 0;
	}

	if(_$_('PAGE_NUM').value <= 0)
	{
		_$_('PAGE_NUM').value = 0;
		
		_$_('next_page_btn').src="next2.gif";
		_$_('next_page_btn').disabled = true;
		
		_$_('last_page_btn').src="last2.gif"
		_$_('last_page_btn').disabled = true;

		_$_('first_page_btn').src="first2.gif";
		_$_('first_page_btn').disabled = true;
		
		_$_('prev_page_btn').src="prev2.gif"
		_$_('prev_page_btn').disabled = true;
		
	}
	else
	{
		if(_$_('PAGE_CUR').value <= 0)
		{
			_$_('PAGE_CUR').value = 0;
			
			_$_('first_page_btn').src="first2.gif";
			_$_('first_page_btn').disabled = true;
			
			_$_('prev_page_btn').src="prev2.gif"
			_$_('prev_page_btn').disabled = true;

			
		}
		else
		{
			_$_('first_page_btn').src="first.gif";
			_$_('first_page_btn').disabled = false;
			
			_$_('prev_page_btn').src="prev.gif"
			_$_('prev_page_btn').disabled = false;
		}

		if(parseInt(_$_('PAGE_CUR').value) >= parseInt(_$_('PAGE_NUM').value - 1))
		{
			_$_('PAGE_CUR').value = parseInt(_$_('PAGE_NUM').value) - 1;
			
			_$_('next_page_btn').src="next2.gif";
			_$_('next_page_btn').disabled = true;
			
			_$_('last_page_btn').src="last2.gif"
			_$_('last_page_btn').disabled = true;
		}
		else
		{
			_$_('next_page_btn').src="next.gif";
			_$_('next_page_btn').disabled = false;
			
			_$_('last_page_btn').src="last.gif"
			_$_('last_page_btn').disabled = false;
		}

	}

}

function show_mail_detail(url)
{
	window.open(url);
}