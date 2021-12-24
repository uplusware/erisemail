function load_levels_to_option(selobj)
{
	var qUrl = "/api/listlevels.xml";
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
			var errno = responseNode.getAttribute("errno")
			if(errno == "0" || errno == 0)
			{
				var strTmp;
				var levelList = responseNode.childNodes;
				
				for(var i = 0; i < levelList.length; i++)
				{
					if(levelList.item(i).tagName == "level")
					{
						var varItem = new Option(levelList.item(i).getAttribute("name"), levelList.item(i).getAttribute("lid"));
						if(levelList.item(i).getAttribute("default") == "true")
						{
							_$_('NEW_USER_DIV').setAttribute("defaultlevel", levelList.item(i).getAttribute("lid"));
							varItem.selected = true;
						}
						selobj.options.add(varItem);
					}
				}

				_$_("STATUS").innerHTML = "";
				_$_("STATUS").style.display = "none";
			}
		}
	}
}
function load_clusters_to_option(selobj)
{
	var qUrl = "/api/listcluster.xml";
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
			var errno = responseNode.getAttribute("errno")
			if(errno == "0" || errno == 0)
			{
				var strTmp;
				var clusterList = responseNode.childNodes;
				
				for(var i = 0; i < clusterList.length; i++)
				{
					if(clusterList.item(i).tagName == "cluster")
					{
						var varItem = new Option(clusterList.item(i).getAttribute("host"), clusterList.item(i).getAttribute("host"));
						selobj.options.add(varItem);
					}
				}

				_$_("STATUS").innerHTML = "";
				_$_("STATUS").style.display = "none";
			}
		}
	}
}	
function show_edituser_div(username, obj)
{
	$("#EDIT_USER_DIV").dialog({
	  height: 380,
	  width: 360,
	  modal: false,
	  title: 'Edit Account',
	  buttons: {                  
		  "Ok": function() {
				do_edit_user();
				$(this).dialog("close");
			  },
		  "Cancel": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	if(_$_('EDIT_USER_DIV').getAttribute("loaded") != "true")
	{
		load_levels_to_option(_$_('EDIT_LEVEL'));
		load_clusters_to_option(_$_('EDIT_HOST'));
		_$_('EDIT_USER_DIV').setAttribute("loaded", "true")
	}
	
	_$_('EDIT_USERNAME').value = obj.getAttribute("username");
	_$_('EDIT_ALIAS').value = obj.getAttribute("alias");
	
	for (var i = 0; i < _$_('EDIT_LEVEL').options.length; i++) 
	{
		if (_$_('EDIT_LEVEL').options[i].value == obj.getAttribute("level"))
		{
			_$_('EDIT_LEVEL').options[i].selected = true;            
			break;        
		}        

	}
	
	for (var i = 0; i < _$_('EDIT_HOST').options.length; i++) 
	{
		if (_$_('EDIT_HOST').options[i].value == obj.getAttribute("host"))
		{
			_$_('EDIT_HOST').options[i].selected = true;            
			break;        
		}        

	}
	
	if(obj.getAttribute("type") == "Group" || obj.getAttribute("type") == "group")
	{
		_$_('EDIT_LEVEL').disabled = true;
		_$_('EDIT_HOST').disabled = true;
	}
	else
	{
		_$_('EDIT_LEVEL').disabled = false;
		_$_('EDIT_HOST').disabled = false;
	}
	
	_$_('EDIT_USER_STATUS').checked = (obj.getAttribute("status") == "Active") ? false : true;
	
	_$_('EDIT_USER_DIV').setAttribute("username", username);
}

function show_newuser_div()
{
	$("#NEW_USER_DIV").dialog({
	  height: 380,
	  width: 360,
	  modal: false,
	  title: 'New Account',
	  buttons: {                  
		  "Ok": function() {
				do_add_user();
				$(this).dialog("close");
			  },
		  "Cancel": function() {
			  $(this).dialog("close");
			  }
	 }});
	 
	document.getElementsByName('NEW_TYPE')[0].checked = true;

	_$_('NEW_USERNAME').value = "";
	
	_$_('NEW_USERPWD').disabled = false;
	_$_('NEW_USERPWD2').disabled = false;

	_$_('NEW_LEVEL').disabled = false;

	_$_('NEW_USERPWD').value = "";
	_$_('NEW_USERPWD2').value = "";

	_$_('NEW_ALIAS').value = "";

	if(_$_('NEW_USER_DIV').getAttribute("loaded") != "true")
	{
		load_levels_to_option(_$_('NEW_LEVEL'));
		load_clusters_to_option(_$_('NEW_HOST'));
		_$_('NEW_USER_DIV').setAttribute("loaded", "true")
	}

	_$_('NEW_LEVEL').value = _$_('NEW_USER_DIV').getAttribute("defaultlevel");
}

function do_delete_user(uname)
{
	var qUrl = "/api/deluser.xml?USERNAME=" + encodeURIComponent(uname);
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
					clear_table_without_header(_$_('USERTBL'));
					load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
				
				}
				else
				{
					alert("Fail to remove ID");
				}
			}
		}
	}
	xmlHttp.open("GET", qUrl , true);
	xmlHttp.send("");
}

function load_users(orderby, desc)
{
	var qUrl = "/api/listusers.xml?ORDER_BY=" + orderby + "&DESC=" + (desc == null ? '' : desc);
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
						var userList = responseNode.childNodes;

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
								if(userList.item(i).getAttribute("type") == "Group")
								{
									tr.style.backgroundColor = "#FFFFEE";
								}
								tr.onmouseover = function()
								{	
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('focusbg.gif')";
								}
								tr.onmouseout = function()
								{
									this.style.backgroundImage = this.getAttribute("imagesrc");
								}

								tr.setAttribute("username", userList.item(i).getAttribute("name"));
								tr.setAttribute("alias", userList.item(i).getAttribute("alias"));
								tr.setAttribute("level", userList.item(i).getAttribute("level"));
								tr.setAttribute("status", userList.item(i).getAttribute("status"));
								tr.setAttribute("type", userList.item(i).getAttribute("type"));
								
								var td0 = tr.insertCell(0);
								td0.valign="middle";
								td0.align="center";
								td0.height="25";
								setStyle(td0, "TD.gray");
								td0.innerHTML = "<img src=\""+image +"\" />";

								var td1 = tr.insertCell(1);
								td1.valign="middle";
								td1.align="left";
								td1.height="25";
								setStyle(td1, "TD.gray");
								td1.innerHTML = userList.item(i).getAttribute("name");

								var td2 = tr.insertCell(2);
								td2.valign="middle";
								td2.align="left";
								td2.width="398";
								td2.height="25";
								setStyle(td2, "TD.gray");
								td2.innerHTML = userList.item(i).getAttribute("alias");
								
								var td3 = tr.insertCell(3);
								td3.valign="middle";
								td3.align="left";
								td3.height="25";
								setStyle(td3, "TD.gray");
								td3.innerHTML = userList.item(i).getAttribute("host");
								
								var td4 = tr.insertCell(4);
								td4.valign="middle";
								td4.align="left";
								td4.height="25";
								setStyle(td4, "TD.gray");
								td4.innerHTML = userList.item(i).getAttribute("lname");
								
								var td5 = tr.insertCell(5);
								td5.valign="middle";
								td5.align="center";
								td5.width="50";
								td5.height="25";
								setStyle(td5, "TD.gray");
								td5.innerHTML = userList.item(i).getAttribute("type") == "Group" ? "Group" : "User" ;

								var td6 = tr.insertCell(6);
								td6.valign="middle";
								td6.align="center";
								td6.height="25";
								setStyle(td6, "TD.gray");
								td6.innerHTML = userList.item(i).getAttribute("role") == "Administrator" ? "Administrator" : "General User" ;
								
								var td7 = tr.insertCell(7);
								td7.valign="middle";
								td7.align="center";
								td7.height="25";
								setStyle(td7, "TD.gray");
								td7.innerHTML = userList.item(i).getAttribute("status") == "Active" ? "Active" : "Disabled";

								var td8 = tr.insertCell(8);
								td8.valign="middle";
								td8.align="center";
								td8.height="25";
								
								td8.setAttribute("username", userList.item(i).getAttribute("name"));
								td8.innerHTML = "<img src=\"edit.gif\">";

								td8.onclick = function()
								{
									show_edituser_div(this.getAttribute("username"), this.parentNode);
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

								setStyle(td8, "TD.gray");
								
								var td9 = tr.insertCell(9);
								td9.valign="middle";
								td9.align="center";
								td9.height="25";
								setStyle(td9, "TD.gray");
								td9.setAttribute("username", userList.item(i).getAttribute("name"));
								td9.innerHTML = "<img src=\"delete.gif\">";

								td9.onclick = function()
								{
									if(confirm("Sure to remove?") ==  true)
										do_delete_user(this.parentNode.getAttribute('username'));
								}
								
								td9.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
									
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('activebg.gif')";
								}
								
								td9.onmouseout = function()
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
	window.parent.change_tab("user");
	_$_('USERTBL').setAttribute('orderby', 'utime');
	load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
}

function do_add_user()
{
	if(_$_('NEW_USERPWD').value != _$_('NEW_USERPWD2').value)
	{
		alert("Passwod is not equal with verified one.");
		return;
	}

	var strType;
	if(document.getElementsByName('NEW_TYPE')[0].checked)
		strType = "member";
	else if(document.getElementsByName('NEW_TYPE')[1].checked)
		strType = "group";
	
	var qUrl = "/api/adduser.xml";
	var strPostData = "NEW_USERNAME=" + encodeURIComponent(_$_('NEW_USERNAME').value);
	strPostData += "&NEW_USERPWD=" + encodeURIComponent(_$_('NEW_USERPWD').value);
	strPostData += "&NEW_ALIAS=" + encodeURIComponent(_$_('NEW_ALIAS').value);
	strPostData += "&NEW_TYPE=" + encodeURIComponent(strType);
	strPostData += "&NEW_LEVEL=" + encodeURIComponent(_$_('NEW_LEVEL').value);
	strPostData += "&NEW_HOST=" + encodeURIComponent(_$_('NEW_HOST').value);
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
					clear_table_without_header(_$_('USERTBL'));
					load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
				
				}
				else
				{
					alert("Fail to create ID");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);

	_$_('NEW_USERNAME').value = "";
	_$_('NEW_USERPWD').value = "";
	_$_('NEW_USERPWD2').value = "";
	_$_('NEW_ALIAS').value = "";
}

function do_edit_user()
{
	var qUrl = "/api/edituser.xml";
	var strPostData = "EDIT_USERNAME=" + encodeURIComponent(_$_('EDIT_USERNAME').value);
	strPostData += "&EDIT_ALIAS=" + encodeURIComponent(_$_('EDIT_ALIAS').value);
	strPostData += "&EDIT_LEVEL=" + encodeURIComponent(_$_('EDIT_LEVEL').value);
	strPostData += "&EDIT_HOST=" + encodeURIComponent(_$_('EDIT_HOST').value);
	strPostData += "&EDIT_USER_STATUS=" + encodeURIComponent(_$_('EDIT_USER_STATUS').checked == true ? "Disable" : "Active");
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
					clear_table_without_header(_$_('USERTBL'));
					load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')));
				
				}
				else
				{
					alert("Fail to update ID");
				}
			}
		}
	}
	xmlHttp.open("POST", qUrl , true);
	xmlHttp.send(strPostData);
}

function sort_users(orderby)
{
	_$_('USERTBL').setAttribute('orderby', orderby);

	clear_table_without_header(_$_('USERTBL'));

	if(_$_('USERTBL').getAttribute(orderby) == 'true')
		_$_('USERTBL').setAttribute(orderby, 'false');
	else
		_$_('USERTBL').setAttribute(orderby, 'true');

	load_users(_$_('USERTBL').getAttribute('orderby'), _$_('USERTBL').getAttribute(_$_('USERTBL').getAttribute('orderby')))
}

function switch_id_type()
{

	if(document.getElementsByName('NEW_TYPE')[0].checked)
	{
		_$_('NEW_USERPWD').disabled = false;
		_$_('NEW_USERPWD2').disabled = false;

		_$_('NEW_LEVEL').disabled = false;
		_$_('NEW_HOST').disabled = false;

		_$_('NEW_USERPWD').value = "";
		_$_('NEW_USERPWD2').value = "";

		
	}
	else if(document.getElementsByName('NEW_TYPE')[1].checked)
	{
		_$_('NEW_USERPWD').disabled = true;
		_$_('NEW_USERPWD2').disabled = true;

		_$_('NEW_LEVEL').disabled = true;
		_$_('NEW_HOST').disabled = true;

		_$_('NEW_USERPWD').value = randpwd();
		_$_('NEW_USERPWD2').value = _$_('NEW_USERPWD').value;

		_$_('NEW_LEVEL').value = _$_('NEW_USER_DIV').getAttribute("defaultlevel");
	}
}
