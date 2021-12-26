function TableFormat(arrText) {

    var strTbl = "<table border=\"0\" cellpadding=\"2\" cellspacing=\"2\"><tr>";
    for (var c = 0; c < arrText.length; c++) {
        if (arrText[c] == null)
            arrText[c] = "";

        strTbl += "<td>" + (arrText[c]) + "<td>";
    }
    strTbl += "</tr></table>";
    return strTbl;
}

function read_mail(strID) {
    var qUrl = "/api/readmail.xml?ID=" + strID;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200) {
                var xmldom = xmlHttp.responseXML;
                xmldom.documentElement.normalize();
                var responseNode = xmldom.documentElement.childNodes.item(0);
                if (responseNode.tagName == "response") {
                    var errno = responseNode.getAttribute("errno")
                        if (errno == "0" || errno == 0) {
                            var strTextContent,
                            strHtmlContent,
                            strCalendarContent,
                            strFrom,
                            strTo,
                            strCc,
                            strBcc,
                            strDate,
                            strSubject,
                            strAttach;
                            strFrom = "";
                            strTo = "";
                            strCc = "";
                            strDate = "";
                            strSubject = "";
                            strAttach = "";
                            strTextContent = "";
                            strHtmlContent = "";
                            strCalendarContent = "";

                            var calDetail = null;

                            var bodyList = responseNode.childNodes;
                            for (var i = 0; i < bodyList.length; i++) {
                                if (bodyList.item(i).tagName == "from") {
                                    strFrom = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "to") {
                                    strTo = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "cc") {
                                    strCc = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "date") {
                                    strDate = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "subject") {
                                    strSubject = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "attach") {
                                    strAttach = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "text_content") {
                                    strTextContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "html_content") {
                                    strHtmlContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                } else if (bodyList.item(i).tagName == "calendar_content") {
                                    strCalendarContent = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                                }
                            }
                            if (strCc == "") {
                                _$_("cccol").style.display = "none";
                            }

                            _$_("mailcontent").style.width = _$_('mailcontentframe').offsetWidth - 5;
                            _$_("mailcontent").style.height = _$_('mailcontentframe').offsetHeight - 5;

                            if (strCalendarContent != "") {
                                var vCal = new vCalendar();
                                calDetail = vCal.Parse(strCalendarContent);
                                strCalendarContent = "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\"><tr bgcolor=\"#FFFFCC\"><td class=\"gray\">";
                                strCalendarContent += "<table border=\"0\"><tr height=\"22\"><td width=\"22\" valign=\"middle\"><img src=\"calendar.gif\"></td><td valign=\"middle\"><b>" + (calDetail.get("METHOD") == "REQUEST" ? "Event Request" : "Event Replay") + "</b></td></tr></table>";
                                strCalendarContent += "</td></tr></table>";

                                strCalendarContent += "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\">";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Summary:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/SUMMARY")))) + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Location:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/LOCATION")))) + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Created:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/CREATED")), "[" + TextToHTML(calDetail.get("VTIMEZONE/TZID")) + "]")) + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Modified:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/LAST-MODIFIED")), "[" + TextToHTML(calDetail.get("VTIMEZONE/TZID")) + "]")) + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Start:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/DTSTART/DT")), "[" + TextToHTML(calDetail.get("VEVENT/DTSTART/TZID")) + "]")) + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>End:</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/DTEND/DT")), "[" + TextToHTML(calDetail.get("VEVENT/DTEND/TZID")) + "]")) + "</td></tr>";

                                var strORGANIZERbl = "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\">";
                                var ORGANIZERPeople = calDetail.get("VEVENT/ORGANIZER").split(";");
                                for (var q = 0; q < ORGANIZERPeople.length; q++) {
                                    var tmp = ORGANIZERPeople[q].split("|");
                                    if (tmp[1] == "ACCEPTED" || tmp[1] == "DELEGATED") {
                                        strORGANIZERbl += "<tr height=\"22\" bgcolor=\"#99FF99\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    } else if (tmp[1] == "DECLINED") {
                                        strORGANIZERbl += "<tr height=\"22\" bgcolor=\"#FF0000\"><td class=\"gray\">" + TableFormat(Array("<font color=\"#FFFFFF\">" + TextToHTML(tmp[0]) + "</font>")) + "</td></tr>";
                                    } else if (tmp[1] == "TENTATIVE") {
                                        strORGANIZERbl += "<tr height=\"22\" bgcolor=\"#FFFFCC\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    } else {
                                        strORGANIZERbl += "<tr height=\"22\" bgcolor=\"#FFFFFF\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    }
                                }
                                strORGANIZERbl += "</table>";

                                var strATTENDEETbl = "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\">";
                                var ATTENDEEPeople = calDetail.get("VEVENT/ATTENDEE").split(";");
                                for (var q = 0; q < ATTENDEEPeople.length; q++) {
                                    var tmp = ATTENDEEPeople[q].split("|");
                                    if (tmp[1] == "ACCEPTED" || tmp[1] == "DELEGATED") {
                                        strATTENDEETbl += "<tr height=\"22\" bgcolor=\"#99FF99\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    } else if (tmp[1] == "DECLINED") {
                                        strATTENDEETbl += "<tr height=\"22\" bgcolor=\"#FF0000\"><td class=\"gray\">" + TableFormat(Array("<font color=\"#FFFFFF\">" + TextToHTML(tmp[0]) + "</font>")) + "</td></tr>";
                                    } else if (tmp[1] == "TENTATIVE") {
                                        strATTENDEETbl += "<tr height=\"22\" bgcolor=\"#FFFFCC\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    } else {
                                        strATTENDEETbl += "<tr height=\"22\" bgcolor=\"#FFFFFF\"><td class=\"gray\">" + TableFormat(Array(TextToHTML(tmp[0]))) + "</td></tr>";
                                    }
                                }
                                strATTENDEETbl += "</table>";

                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Organizer:</b></td><td class=\"gray\">" + strORGANIZERbl + "</td></tr>";
                                strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>Attendee:</b></td><td class=\"gray\">" + strATTENDEETbl + "</td></tr>";
                                strCalendarContent += "</table>";

                                strCalendarContent += "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\">";
                                strCalendarContent += "<tr><td>" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/DESCRIPTION")))) + "</td></tr>"
                                strCalendarContent += "</table>";
                            }
                            var strPreview = "";

                            var attachs = strAttach.split("*");
                            var att_count = 0;
                            for (var x = 0; x < attachs.length; x++) {
                                if (attachs[x] != "") {
                                    att_count++;

                                    var filename;
                                    var filetype;
                                    var detail = attachs[x].split("|");
                                    filename = detail[0];
                                    filetype = detail[1];

                                    var imgsrc = "/api/attachment.cgi?ID=" + strID + "&FILENAME=" + encodeURIComponent(filename);

                                    if (filetype == "image" || filetype == "Image") {
                                        strPreview += "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\"><tr><td width='16'><img src=\"attach.gif\"></td><td align='left'><a href='" + imgsrc + "' target='_blank' >" + TextToHTML(filename) + "</a></td></tr><tr><td colspan='2'><a href='" + imgsrc + "' target='_blank' ><img src='" + imgsrc + "' border='0' width=\"100\" onload='DrawImage(this)'></a></td></tr></table>"
                                    } else {
                                        strPreview += "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\"><tr><td width='16'><img src=\"attach.gif\"></td><td align='left'><a href='" + imgsrc + "' target='_blank' >" + TextToHTML(filename) + "</a></td></tr></table>";
                                    }
                                }
                            }

                            _$_("mailfrom").innerHTML = "<input class=\"text3\" size=\"100\" type=\"text\" value=\"" + TextToHTML(strFrom) + "\" readonly>";
                            _$_("mailto").innerHTML = "<input class=\"text3\" size=\"100\" type=\"text\" value=\"" + TextToHTML(strTo) + "\" readonly>";
                            _$_("mailcc").innerHTML = "<input class=\"text3\" size=\"100\" type=\"text\" value=\"" + TextToHTML(strCc) + "\" readonly>";
                            _$_("maildate").innerHTML = "<input class=\"text3\" size=\"100\" type=\"text\" value=\"" + TextToHTML(strDate) + "\" readonly>";
                            _$_("mailsubject").innerHTML = "<input class=\"text3\" size=\"100\" type=\"text\" value=\"" + (strSubject == "" ? "No Subject" : TextToHTML(strSubject)) + "\" readonly>";

                            var strrpl = "/api/attachment.cgi?ID=";
                            strrpl += strID;
                            strrpl += "&CONTENTID=";

                            var regS1 = new RegExp("cid:", "gi");
                            strHtmlContent = strHtmlContent.replace(regS1, strrpl);
                            if (strCalendarContent == "") {
                                _$_("mailcontent").innerHTML = (strHtmlContent != "" ? strHtmlContent : TextToHTML(strTextContent)) + (strPreview != "" ? ("<p></p><hr><b>" + att_count + " attachment(s)</b>" + strPreview) : "");
                            } else {
                                _$_("timecol").style.display = "none";
                                _$_("mailcontent").innerHTML = strCalendarContent + (strPreview != "" ? ("<p></p><hr><b>" + att_count + " attachment(s)</b>" + strPreview) : "");
                            }
                        } else {
                            _$_("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>Load Failed</td></tr></table>";
                        }
                }
            } else {
                _$_("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>Load Failed</td></tr></table>";
            }
        } else {
            _$_("mailfrom").innerHTML = "<img src=\"loading.gif\">";
            _$_("mailto").innerHTML = "<img src=\"loading.gif\">";
            _$_("mailcc").innerHTML = "<img src=\"loading.gif\">";
            _$_("maildate").innerHTML = "<img src=\"loading.gif\">";
            _$_("mailsubject").innerHTML = "<img src=\"loading.gif\">";
            _$_("mailcontent").innerHTML = "<img src=\"loading.gif\">";
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function init() {
    read_mail(Request.QueryString('ID'));
}

function pass_mail() {
    window.opener.do_pass_mail(Request.QueryString('ID'));
    window.close();
}

function reject_mail() {
    window.opener.do_reject_mail(Request.QueryString('ID'));
    window.close();
}
