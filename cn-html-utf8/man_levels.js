function ontimer(trid)
{
	if(_$_(trid))
	{
		if(_$_(trid).getAttribute("isOver") == "true")
		{
			_$_('LEVEL_DESCRIPTION').innerHTML = "<table border=\"0\" height=\"100%\" width=\"100%\" cellpadding=\"2\" cellspacing=\"2\" bgcolor=\"#FFFFEE\"><tr><td align=\"left\" valign=\"middle\">" + _$_(trid).getAttribute("description") + "</td></tr></table>";

			var pos = GetObjPos(_$_(trid));
			
			_$_('LEVEL_DESCRIPTION').style.left = pos.x - 1;
			_$_('LEVEL_DESCRIPTION').style.top = pos.y + pos.h + 1;
			
			_$_('LEVEL_DESCRIPTION').style.height= 25;
			_$_('LEVEL_DESCRIPTION').style.width = _$_('LEVELTBL').offsetWidth;

			_$_('LEVEL_DESCRIPTION').style.display = "block";
		}
	}
}

function load_levels()
{
	var qUrl = "/api/listlevels.xml";
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
						var levelList = responseNode.childNodes;
						

						for(var i = 0; i < levelList.length; i++)
						{
							if(levelList.item(i).tagName == "level")
							{
								tr = _$_('LEVELTBL').insertRow(_$_('LEVELTBL').rows.length);
								tr.setAttribute("id", "LEVEL_ID_" + levelList.item(i).getAttribute("lid"));
								tr.setAttribute("lid", levelList.item(i).getAttribute("lid"));
								tr.setAttribute("name", levelList.item(i).getAttribute("name"));
								tr.setAttribute("description", levelList.item(i).getAttribute("description"));
								tr.setAttribute("mailmaxsize", levelList.item(i).getAttribute("mailmaxsize"));
								tr.setAttribute("boxmaxsize", levelList.item(i).getAttribute("boxmaxsize"));
								tr.setAttribute("enableaudit", levelList.item(i).getAttribute("enableaudit"));
								tr.setAttribute("mailsizethreshold", levelList.item(i).getAttribute("mailsizethreshold"));
								tr.setAttribute("attachsizethreshold", levelList.item(i).getAttribute("attachsizethreshold"));
								tr.setAttribute("default", levelList.item(i).getAttribute("default"));
								
								tr.onmouseover = function()
								{										
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('focusbg.gif')";
									this.setAttribute("isOver", "true");
									setTimeout("ontimer(\""+ this.getAttribute("id") +"\")",1200);

									
								}
								tr.onmouseout = function()
								{
									this.style.backgroundImage = this.getAttribute("imagesrc");

									_$_('LEVEL_DESCRIPTION').style.display = "none";

									this.setAttribute("isOver", "false");
								}

								var image1 = "<img src=\"pading.gif\" />";

								if(levelList.item(i).getAttribute("default") == "true")
								{
									image1 = "<img src=\"default.gif\" alt=\"默认策略\" />";
								}

								var image2 = "<img src=\"no.gif\" alt=\"无需审核\" />";

								if(levelList.item(i).getAttribute("enableaudit") == "true")
								{
									image2 = "<img src=\"yes.gif\" alt=\"需要审核\" />";
								}
								
								var td0 = tr.insertCell(0);
								td0.valign="middle";
								td0.align="center";
								td0.height="25";
								setStyle(td0, "TD.gray");
								td0.innerHTML = image1;

								td0.setAttribute("lid", levelList.item(i).getAttribute("lid"));
								
								td0.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
									
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('activebg.gif')";
								}
								
								td0.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";

									this.style.backgroundImage = this.getAttribute("imagesrc");
								}

								td0.onclick = function()
								{
									set_level_deafult(this.parentNode);
								}
								
								var td1 = tr.insertCell(1);
								td1.valign="middle";
								td1.align="left";
								td1.height="25";
								setStyle(td1, "TD.gray");
								td1.innerHTML = levelList.item(i).getAttribute("name");

								var td2 = tr.insertCell(2);
								td2.valign="middle";
								td2.align="left";
								td2.width="100";
								td2.height="25";
								setStyle(td2, "TD.gray");
								td2.innerHTML = Math.round(levelList.item(i).getAttribute("mailmaxsize")/1024*100)/100 + "KB";

								var td3 = tr.insertCell(3);
								td3.valign="middle";
								td3.align="center";
								td3.width="100";
								td3.height="25";
								setStyle(td3, "TD.gray");
								td3.innerHTML = Math.round(levelList.item(i).getAttribute("boxmaxsize")/1024) + "KB";

								var td4 = tr.insertCell(4);
								td4.valign="middle";
								td4.align="center";
								td4.height="25";
								td3.width="100";
								setStyle(td4, "TD.gray");
								td4.innerHTML = image2;
								
								var td5 = tr.insertCell(5);
								td5.valign="middle";
								td5.align="center";
								td5.height="25";
								setStyle(td5, "TD.gray");
								td5.innerHTML = Math.round(levelList.item(i).getAttribute("mailsizethreshold")/1024*100)/100 + "KB";

								var td6 = tr.insertCell(6);
								td6.valign="middle";
								td6.align="center";
								td6.height="25";
								setStyle(td6, "TD.gray");
								td6.innerHTML = Math.round(levelList.item(i).getAttribute("attachsizethreshold")/1024*100)/100 + "KB";

								var td7 = tr.insertCell(7);
								td7.valign="middle";
								td7.align="center";
								td7.height="25";
								setStyle(td7, "TD.gray");
								td7.innerHTML = "<img src=\"edit.gif\">";

								td7.onclick = function()
								{
									_$_('LEVEL_DESCRIPTION').style.display = "none";										
									show_editlevel_div(this.parentNode);
								}
								
								td7.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
									
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('activebg.gif')";
								}
								
								td7.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";

									this.style.backgroundImage = this.getAttribute("imagesrc");
								}

								var td8 = tr.insertCell(8);
								td8.valign="middle";
								td8.align="center";
								td8.height="25";
								setStyle(td8, "TD.gray");
								td8.innerHTML = "<img src=\"delete.gif\">";

								td8.onclick = function()
								{
									if(this.parentNode.getAttribute("default") == "true")
										alert("默认策略，不能删除");
									else
									{
										if(confirm("确定删除吗？") ==  true)
											do_delete_level(this.parentNode.getAttribute("lid"));
									}
								}
								
								td8.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
									
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('activebg.gif')";
								}
								
								td8.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";

									this.style.backgroundImage = this.getAttribute("imagesrc");
								}
								
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

function init()
{
	window.parent.change_tab("level");
	load_levels();
}

function set_level_deafult(obj)
{
	var qUrl = "/api/defaultlevel.xml";
	var strPostData = "LEVELID=" + encodeURIComponent(obj.getAttribute("lid"));
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
					clear_table_without_header(_$_('LEVELTBL'));
					load_levels();
				
				}
				else
				{
					alert("设置默认策略失败");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);
}
	
function show_newlevel_div()
{
	$("#NEW_LEVEL_DIV").dialog({
	  height: 500,
	  width: 500,
	  modal: false,
	  title: '新增策略',
	  buttons: {                  
		  "确定": function() {
			if(do_create_level())
				$(this).dialog("close");
			  },
		  "取消": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	_$_('NEW_LEVELNAME').value = "";
	_$_('NEW_DESCRIPTION').value = "";
	_$_('NEW_MAILMAXSIZE').value = "5000";
	_$_('NEW_BOXMAXSIZE').value = "500000";
	_$_('NEW_AUDIT').checked = false;
	_$_('NEW_MAILSIZETHRESHOLD').value = "5000";
	_$_('NEW_ATTACHSIZETHRESHOLD').value = "5000";
	_$_('NEW_DEFAULT').checked = false;
}

function show_editlevel_div(obj)
{
	$("#EDIT_LEVEL_DIV").dialog({
	  height: 500,
	  width: 500,
	  modal: false,
	  title: '编辑策略',
	  buttons: {                  
		  "确定": function() {
			if(do_edit_level())
				$(this).dialog("close");
			  },
		  "取消": function() {
			  $(this).dialog("close");
			  }
	 }});
	 

	_$_('EDIT_LEVEL_DIV').setAttribute("lid", obj.getAttribute("lid"));

	_$_('EDIT_LEVELNAME').value = obj.getAttribute("name");
	_$_('EDIT_DESCRIPTION').value = obj.getAttribute("description");
	_$_('EDIT_MAILMAXSIZE').value = Math.round(obj.getAttribute("mailmaxsize")/1024);
	_$_('EDIT_BOXMAXSIZE').value = Math.round(obj.getAttribute("boxmaxsize")/1024);
	_$_('EDIT_AUDIT').checked = obj.getAttribute("enableaudit") == "true" ? true : false;
	_$_('EDIT_MAILSIZETHRESHOLD').value = Math.round(obj.getAttribute("mailsizethreshold")/1024);
	_$_('EDIT_ATTACHSIZETHRESHOLD').value = Math.round(obj.getAttribute("attachsizethreshold")/1024);
	_$_('EDIT_DEFAULT').checked = obj.getAttribute("default") == "true" ? true : false;
}

function do_create_level()
{
	if(!isLegalID(_$_('NEW_LEVELNAME').value) 
		|| !isNumber(_$_('NEW_MAILMAXSIZE').value)
		|| !isNumber(_$_('NEW_BOXMAXSIZE').value)
		|| !isNumber(_$_('NEW_MAILSIZETHRESHOLD').value)
		|| !isNumber(_$_('NEW_ATTACHSIZETHRESHOLD').value))
	{
		alert("输入字符不合法");
		return false;
	}

	if(parseInt(_$_('NEW_MAILMAXSIZE').value) > 1024*1024*2)
	{
		alert("邮件最大值不能大于 "+ 1024*1024*2 + " KB");
		return false;
	}
	
	if(parseInt(_$_('NEW_BOXMAXSIZE').value) > 1024*1024*4)
	{
		alert("邮箱容量不能大于 "+ 1024*1024*4 + " KB");
		return false;
	}
	var qUrl = "/api/createlevel.xml";
	var strPostData = "NEW_LEVELNAME=" + encodeURIComponent(_$_('NEW_LEVELNAME').value);
	strPostData += "&NEW_DESCRIPTION=" + encodeURIComponent(_$_('NEW_DESCRIPTION').value);
	strPostData += "&NEW_MAILMAXSIZE=" + encodeURIComponent(_$_('NEW_MAILMAXSIZE').value);
	strPostData += "&NEW_BOXMAXSIZE=" + encodeURIComponent(_$_('NEW_BOXMAXSIZE').value);
	strPostData += "&NEW_AUDIT=" + ((_$_('NEW_AUDIT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false")) ;
	strPostData += "&NEW_MAILSIZETHRESHOLD=" + encodeURIComponent(_$_('NEW_MAILSIZETHRESHOLD').value);
	strPostData += "&NEW_ATTACHSIZETHRESHOLD=" + encodeURIComponent(_$_('NEW_ATTACHSIZETHRESHOLD').value);
	strPostData += "&NEW_DEFAULT=" + ((_$_('NEW_DEFAULT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false")) ;
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
					clear_table_without_header(_$_('LEVELTBL'));
					load_levels();
				
				}
				else
				{
					alert("创建策略失败");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);

	return true;
}

function do_edit_level()
{		
	if(!isLegalID(_$_('EDIT_LEVELNAME').value) 
		|| !isNumber(_$_('EDIT_MAILMAXSIZE').value)
		|| !isNumber(_$_('EDIT_BOXMAXSIZE').value)
		|| !isNumber(_$_('EDIT_MAILSIZETHRESHOLD').value)
		|| !isNumber(_$_('EDIT_ATTACHSIZETHRESHOLD').value))
	{
		alert("输入字符不合法");
		return false;
	}

	if(parseInt(_$_('EDIT_MAILMAXSIZE').value) > 1024*1024*2)
	{
		alert("邮件最大值不能大于 "+ 1024*1024*2 + " KB");
		return false;
	}
	
	if(parseInt(_$_('EDIT_BOXMAXSIZE').value) > 1024*1024*4)
	{
		alert("邮箱容量不能大于 "+ 1024*1024*4 + " KB");
		return false;
	}
	
	var qUrl = "/api/updatelevel.xml";
	var strPostData = "EDIT_LEVELID=" + encodeURIComponent(_$_('EDIT_LEVEL_DIV').getAttribute("lid"));
	strPostData += "&EDIT_LEVELNAME=" + encodeURIComponent(_$_('EDIT_LEVELNAME').value);
	strPostData += "&EDIT_DESCRIPTION=" + encodeURIComponent(_$_('EDIT_DESCRIPTION').value);
	strPostData += "&EDIT_MAILMAXSIZE=" + encodeURIComponent(_$_('EDIT_MAILMAXSIZE').value);
	strPostData += "&EDIT_BOXMAXSIZE=" + encodeURIComponent(_$_('EDIT_BOXMAXSIZE').value);
	strPostData += "&EDIT_AUDIT=" + ((_$_('EDIT_AUDIT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false")) ;
	strPostData += "&EDIT_MAILSIZETHRESHOLD=" + encodeURIComponent(_$_('EDIT_MAILSIZETHRESHOLD').value);
	strPostData += "&EDIT_ATTACHSIZETHRESHOLD=" + encodeURIComponent(_$_('EDIT_ATTACHSIZETHRESHOLD').value);
	strPostData += "&EDIT_DEFAULT=" + ((_$_('EDIT_DEFAULT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false")) ;
	
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
					clear_table_without_header(_$_('LEVELTBL'));
					load_levels();
				
				}
				else
				{
					alert("更新策略失败");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);

	return true;
}

function do_delete_level(levelid)
{		
	var qUrl = "/api/deletelevel.xml";
	var strPostData = "LEVELID=" + encodeURIComponent(levelid);
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
					clear_table_without_header(_$_('LEVELTBL'));
					load_levels();
				}
				else
				{
					alert("删除策略失败");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);

	return true;
}
