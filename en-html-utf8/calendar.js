function JHashMap()
{
    this.length = 0;
    this.prefix = "hashmap_prefix_20040716_";
}

JHashMap.prototype.put = function (key, value)
{
    this[this.prefix + key] = value;
    this.length ++;
}

JHashMap.prototype.get = function(key)
{
    return typeof this[this.prefix + key] == "undefined" 
            ? null : this[this.prefix + key];
}

JHashMap.prototype.keySet = function()
{
    var arrKeySet = new Array();
    var index = 0;
    for(var strKey in this)
    {
        if(strKey.substring(0,this.prefix.length) == this.prefix)
            arrKeySet[index ++] = strKey.substring(this.prefix.length);
    }
    return arrKeySet.length == 0 ? null : arrKeySet;
}

JHashMap.prototype.values = function()
{
    var arrValues = new Array();
    var index = 0;
    for(var strKey in this)
    {
        if(strKey.substring(0,this.prefix.length) == this.prefix)
            arrValues[index ++] = this[strKey];
    }
    return arrValues.length == 0 ? null : arrValues;
}

JHashMap.prototype.size = function()
{
    return this.length;
}

JHashMap.prototype.remove = function(key)
{
    delete this[this.prefix + key];
    this.length --;
}

JHashMap.prototype.clear = function()
{
    for(var strKey in this)
    {
        if(strKey.substring(0,this.prefix.length) == this.prefix)
            delete this[strKey];   
    }
    this.length = 0;
}

JHashMap.prototype.isEmpty = function()
{
    return this.length == 0;
}

JHashMap.prototype.containsKey = function(key)
{
    for(var strKey in this)
    {
       if(strKey == this.prefix + key)
          return true;  
    }
    return false;
}

JHashMap.prototype.containsValue = function(value)
{
    for(var strKey in this)
    {
       if(this[strKey] == value)
          return true;  
    }
    return false;
}

JHashMap.prototype.putAll = function(map)
{
    if(map == null)
        return;
    if(map.constructor != JHashMap)
        return;
    var arrKey = map.keySet();
    var arrValue = map.values();
    for(var i in arrKey)
       this.put(arrKey[i],arrValue[i]);
}

//toString
JHashMap.prototype.toString = function()
{
    var str = "";
    for(var strKey in this)
    {
        if(strKey.substring(0,this.prefix.length) == this.prefix)
              str += strKey.substring(this.prefix.length) 
                  + " : " + this[strKey] + "\r\n";
    }
    return str;
}

function strncasecmp(src1, src2, len)
{
	if(src1.length < len || src2.length < len)
		return false;
		
	var tmp1 = src1.toUpperCase();
	var tmp2 = src2.toUpperCase();
	
	var tmp3 = tmp1.substr(0, len);
	var tmp4 = tmp2.substr(0, len);
	
	if (tmp3 == tmp4)
		return true;	
	else
		return false;
}

function strlen(strsrc)
{
	return strsrc.length;
}

function strtrim(str)
{
	if(str)
		return str.replace(/(^\s*)|(\s*$)/g, "");
	else
		return "";
}

function linetrim(str)
{
	return str.replace(/(^[\r\n]*)|([\r\n]*$)/g, "");
}

///////////////////////////////////////////////////////////////////////////////////////////////
// vCalendar
function vCalendar()
{
	
}

vCalendar.prototype.PEOPLE = function (strOrg)
{
	var RSVP = "";
	var CN = "";
	var PARTSTAT = "";
	var ROLE = "";
	var MAILTO = "";
	var strSeg = strOrg.split(";");
	for(var i = 0; i < strSeg.length; i++)
	{
		if(strncasecmp(strSeg[i], "RSVP=", strlen("RSVP=")))
		{
			RSVP = strSeg[i].split("=")[1];
		}
		else if(strncasecmp(strSeg[i], "CN=", strlen("CN=")))
		{
			CN = strSeg[i].split("=")[1];
		}
		else if(strncasecmp(strSeg[i], "PARTSTAT=", strlen("PARTSTAT=")))
		{
			PARTSTAT = strSeg[i].split("=")[1];
		}
		else if(strncasecmp(strSeg[i], "ROLE=", strlen("ROLE=")))
		{
			ROLE = strSeg[i].split("=")[1].split(":")[0];
			MAILTO = strSeg[i].split("=")[1].split(":")[2];
		}
	}
	
	return [strtrim(RSVP), strtrim(CN), strtrim(PARTSTAT), strtrim(ROLE), strtrim(MAILTO)];
}

vCalendar.prototype.DT = function (strDT)
{
	var strSeg = strDT.split(":");
	var strTmp = strSeg[1] == null ? strSeg[0] : strSeg[1];
	return [strTmp.substr(0, 4), strTmp.substr(4, 2), strTmp.substr(6, 2), strTmp.substr(9, 2), strTmp.substr(11, 2), strTmp.substr(13, 2)];
}

vCalendar.prototype.TZID = function (strTZID)
{
	var strSeg = strTZID.split("=");
	var strTmp = strSeg[1] == null ? strSeg[0] : strSeg[1];
	return strTmp;
}

vCalendar.prototype.Parse = function (strcal)
{
	var VCALENDAR = (1 << 0);
	var VTIMEZONE = (1 << 1)
	var STANDARD = (1 << 2);
	var VEVENT = (1 << 3);
	
	var SEGMENT = 0;
	
	var calDetail = new JHashMap();
	var lArr = strcal.split("\n");
	
	for(var x = 0; x < lArr.length; x++)
	{
		var strTmp = linetrim(lArr[x]);
		var j = x + 1;
		while(j < lArr.length && (lArr[j].charAt(0) == ' ' || lArr[j].charAt(0) == '\t'))
		{
			strTmp += linetrim(lArr[j].substr(1));
			j++;
		}
		x = j - 1;
		
		if(strncasecmp(strTmp, "BEGIN:VCALENDAR", strlen("BEGIN:VCALENDAR")))
		{
			SEGMENT = SEGMENT | VCALENDAR;
		}
		else if(strncasecmp(strTmp, "END:VCALENDAR", strlen("END:VCALENDAR")))
		{
			SEGMENT = SEGMENT & (~VCALENDAR);
		}
		else if(strncasecmp(strTmp, "BEGIN:VTIMEZONE", strlen("BEGIN:VTIMEZONE")))
		{
			SEGMENT = SEGMENT | VTIMEZONE;
		}
		else if(strncasecmp(strTmp, "END:VTIMEZONE", strlen("END:VTIMEZONE")))
		{
			SEGMENT = SEGMENT & (~VTIMEZONE);
		}
		else if(strncasecmp(strTmp, "BEGIN:STANDARD", strlen("BEGIN:STANDARD")))
		{
			SEGMENT = SEGMENT | STANDARD;
		}
		else if(strncasecmp(strTmp, "END:STANDARD", strlen("END:STANDARD")))
		{
			SEGMENT = SEGMENT & (~STANDARD);
		}
		else if(strncasecmp(strTmp, "BEGIN:VEVENT", strlen("BEGIN:VEVENT")))
		{
			SEGMENT = SEGMENT | VEVENT;
		}
		else if(strncasecmp(strTmp, "END:VEVENT", strlen("END:VEVENT")))
		{
			SEGMENT = SEGMENT & (~VEVENT);
		}
		else
		{	
			if((SEGMENT & VCALENDAR) == VCALENDAR)
			{				
				if((SEGMENT & VTIMEZONE) == VTIMEZONE)
				{
					if(strncasecmp(strTmp, "TZID:", strlen("TZID:")))
					{
						calDetail.put("VTIMEZONE/TZID", strTmp.substr(strlen("TZID:")));
					}
					else if(strncasecmp(strTmp, "X-LIC-LOCATION:", strlen("X-LIC-LOCATION:")))
					{
						calDetail.put("VTIMEZONE/X-LIC-LOCATION", strTmp.substr(strlen("X-LIC-LOCATION:")));
					}
					if((SEGMENT & STANDARD) == STANDARD)
					{
						if(strncasecmp(strTmp, "TZOFFSETFROM:", strlen("TZOFFSETFROM:")))
						{
							calDetail.put("VTIMEZONE/STANDARD/TZOFFSETFROM", strTmp.substr(strlen("TZOFFSETFROM:")));	
						}
						else if(strncasecmp(strTmp, "TZOFFSETTO:", strlen("TZOFFSETTO:")))
						{
							calDetail.put("VTIMEZONE/STANDARD/TZOFFSETTO", strTmp.substr(strlen("TZOFFSETTO:")));
						}
						else if(strncasecmp(strTmp, "TZNAME:", strlen("TZNAME:")))
						{
							calDetail.put("VTIMEZONE/STANDARD/TZNAME", strTmp.substr(strlen("TZNAME:")));	
						}
						else if(strncasecmp(strTmp, "DTSTART:", strlen("DTSTART:")))
						{
							var dt = this.DT(strTmp.substr(strlen("DTSTART:")));
							calDetail.put("VTIMEZONE/STANDARD/DTSTART", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);
						}
						else if(strncasecmp(strTmp, "DTEND:", strlen("DTEND:")))
						{
							var dt = this.DT(strTmp.substr(strlen("DTEND:")));
							calDetail.put("VTIMEZONE/STANDARD/DTEND", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);
						}		
					}
				}
				else if((SEGMENT & VEVENT) == VEVENT)
				{
					if(strncasecmp(strTmp, "CREATED:", strlen("CREATED:")))
					{
						var dt = this.DT(strTmp.substr(strlen("CREATED:")));
						calDetail.put("VEVENT/CREATED", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);	
					}
					else if(strncasecmp(strTmp, "LAST-MODIFIED:", strlen("LAST-MODIFIED:")))
					{
						var dt = this.DT(strTmp.substr(strlen("LAST-MODIFIED:")));
						calDetail.put("VEVENT/LAST-MODIFIED", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4])	
					}
					else if(strncasecmp(strTmp, "DTSTAMP:", strlen("DTSTAMP:")))
					{
						var dt = this.DT(strTmp.substr(strlen("DTSTAMP:")));
						calDetail.put("VEVENT/DTSTAMP", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);	
					}
					else if(strncasecmp(strTmp, "UID:", strlen("UID:")))
					{
						calDetail.put("VEVENT/UID", strTmp.substr(strlen("UID:")));	
					}
					else if(strncasecmp(strTmp, "SUMMARY:", strlen("SUMMARY:")))
					{
						strTmp = strTmp.substr(strlen("SUMMARY:")).replace(/(\\n|\\N)/g, "\n");
						strTmp = strTmp.replace(/\\\\/g, "\\");
						strTmp = strTmp.replace(/\\\"/g, "\"");
						strTmp = strTmp.replace(/\\t/g, "\t");
						strTmp = strTmp.replace(/\\;/g, ";");
						strTmp = strTmp.replace(/\\,/g, ",");
						
						calDetail.put("VEVENT/SUMMARY", strTmp);	
					}
					else if(strncasecmp(strTmp, "LOCATION:", strlen("LOCATION:")))
					{
						strTmp = strTmp.substr(strlen("LOCATION:")).replace(/(\\n|\\N)/g, "\n");
						strTmp = strTmp.replace(/\\\\/g, "\\");
						strTmp = strTmp.replace(/\\\"/g, "\"");
						strTmp = strTmp.replace(/\\t/g, "\t");
						strTmp = strTmp.replace(/\\;/g, ";");
						strTmp = strTmp.replace(/\\,/g, ",");
						
						calDetail.put("VEVENT/LOCATION", strTmp);	
					}
					else if(strncasecmp(strTmp, "DESCRIPTION:", strlen("DESCRIPTION:")))
					{
						strTmp = strTmp.substr(strlen("DESCRIPTION:")).replace(/(\\n|\\N)/g, "\n");
						strTmp = strTmp.replace(/\\\\/g, "\\");
						strTmp = strTmp.replace(/\\\"/g, "\"");
						strTmp = strTmp.replace(/\\t/g, "\t");
						strTmp = strTmp.replace(/\\;/g, ";");
						strTmp = strTmp.replace(/\\,/g, ",");
						
						calDetail.put("VEVENT/DESCRIPTION", strTmp);
					}
					else if(strncasecmp(strTmp, "SEQUENCE:", strlen("SEQUENCE:")))
					{
						calDetail.put("VEVENT/SEQUENCE", strTmp.substr(strlen("SEQUENCE:")));
					}
					else
					{
						if(strncasecmp(strTmp, "ORGANIZER;", strlen("ORGANIZER;")))
						{
							var people = this.PEOPLE(strTmp);
							calDetail.put("VEVENT/ORGANIZER", calDetail.get("VEVENT/ORGANIZER") == null ? (people[4] + "|" + people[2]) : (calDetail.get("VEVENT/ORGANIZER") + ";" + people[4] + "|" + people[2]));
						}
						else if(strncasecmp(strTmp, "ATTENDEE;", strlen("ATTENDEE;")))
						{
							var people = this.PEOPLE(strTmp);
							calDetail.put("VEVENT/ATTENDEE", calDetail.get("VEVENT/ATTENDEE") == null ? (people[4] + "|" + people[2]) : (calDetail.get("VEVENT/ATTENDEE") + ";" + people[4] + "|" + people[2]))
						}
						else if(strncasecmp(strTmp, "DTSTART;", strlen("DTSTART;")))
						{
							var strDTSTART = strTmp.split(";")[1].split(":");
							var tzid = this.TZID(strDTSTART[0]);
							var dt = this.DT(strDTSTART[1]);
							calDetail.put("VEVENT/DTSTART/TZID", tzid);
							calDetail.put("VEVENT/DTSTART/DT", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);
						}
						else if(strncasecmp(strTmp, "DTEND;", strlen("DTEND;")))
						{
							var strDTEND = strTmp.split(";")[1].split(":");
							var tzid = this.TZID(strDTEND[0]);
							var dt = this.DT(strDTEND[1]);
							calDetail.put("VEVENT/DTEND/TZID", tzid);
							calDetail.put("VEVENT/DTEND/DT", dt[0] + "-" + dt[1] + "-" + dt[2] + " " + dt[3] + ":" + dt[4]);
						}
					}
				}
				else
				{
					if(strncasecmp(strTmp, "VERSION:", strlen("VERSION:")))
					{
						calDetail.put("VERSION", strTmp.substr(strlen("VERSION:")));	
					}
					else if(strncasecmp(strTmp, "METHOD:", strlen("METHOD:")))
					{
						calDetail.put("METHOD", strTmp.substr(strlen("METHOD:")));
					}
				}
			}
		}
	}
	
	return calDetail;
}

