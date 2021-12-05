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
								
								tr.onmouseover = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="pointer";
									
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('focusbg.gif')";
								}
								tr.onmouseout = function()
								{
									this.mousepoint = 99;
									this.style.cursor ="default";
									
									this.style.backgroundImage = this.getAttribute("imagesrc");
								}

								tr.setAttribute("selflink", "writemail.html?TOADDRS=" + userList.item(i).getAttribute("name") + "@" + userList.item(i).getAttribute("domain"));
								
								tr.onclick = function ()
								{
									show_mail_detail(this.getAttribute("selflink"));

									this.style.backgroundImage = this.getAttribute("imagesrc");
								}
								
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
								td2.width="500";
								td2.height="25";
								setStyle(td2, "TD.gray");
								td2.innerHTML = userList.item(i).getAttribute("alias");

								var td3 = tr.insertCell(3);
								td3.valign="middle";
								td3.align="center";
								td3.width="50";
								td3.height="25";
								setStyle(td3, "TD.gray");
								td3.innerHTML = userList.item(i).getAttribute("type") == "Group" ? "Group" : "User" ;

								var td4 = tr.insertCell(4);
								td4.valign="middle";
								td4.align="center";
								td4.height="25";
								setStyle(td4, "TD.gray");
								td4.innerHTML = userList.item(i).getAttribute("role") == "Administrator" ? "Administrator" : "Member" ;
								
								var td5 = tr.insertCell(5);
								td5.valign="middle";
								td5.align="center";
								td5.height="25";
								setStyle(td5, "TD.gray");
								td5.innerHTML = userList.item(i).getAttribute("status") == "Active" ? "Active" : "Disabled";

								if(userList.item(i).getAttribute("status") != "Active")
									tr.disabled = "true";
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
	window.parent._$_('MAILBAR').style.display = "none";
	window.parent._$_('DIRBAR').style.display = "none";
	window.parent._$_('CALBAR').style.display = "none";
	window.parent._$_('READCALBAR').style.display = "none";
	window.parent._$_('NULLBAR').style.display = "block";
	
}

function uninit()
{

}

function show_mail_detail(url)
{	
	window.open(url);
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
