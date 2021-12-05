function init()
{
	window.parent.change_tab("cluster");
	list_cluster();
}

function list_cluster()
{
	var qUrl = "/api/listcluster.xml";
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
						var logList = responseNode.childNodes;
						
						for(var i = 0; i < logList.length; i++)
						{
							if(logList.item(i).tagName == "cluster")
							{
								tr = _$_('CLUSTERTBL').insertRow(_$_('CLUSTERTBL').rows.length);
								tr.setAttribute("host", logList.item(i).getAttribute("host"));
								
								tr.onmouseover = function()
								{
									this.setAttribute("imagesrc", this.style.backgroundImage);
									this.style.backgroundImage = "url('focusbg.gif')";
								}
								
								tr.onmouseout = function()
								{
									
									this.style.backgroundImage = this.getAttribute("imagesrc");
								}
								
								var td0 = tr.insertCell(0);
								td0.valign="middle";
								td0.align="center";
								td0.height="22";
								td0.width="22";
								setStyle(td0, "TD.gray");
								td0.innerHTML = "<img src=\"man_node.png\">";
								
								
								var td1 = tr.insertCell(1);
								td1.valign="middle";
								td1.align="left";
								td1.height="22";
								setStyle(td1, "TD.gray");
								td1.innerHTML = "<a target=\"_blank\" href=\"http://" + logList.item(i).getAttribute("host") + ":8080\">" + logList.item(i).getAttribute("host") + "</a>";
								
								var td2 = tr.insertCell(2);
								td2.valign="middle";
								td2.align="left";
								td2.height="22";
								setStyle(td2, "TD.gray");
								td2.innerHTML = logList.item(i).getAttribute("desc");
								
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
