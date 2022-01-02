function delmail(force) {
    var qUrl;
    if (Request.QueryString('TOP_USAGE') == "trash") {
        qUrl = "/api/delmail.xml?MAILID=" + Request.QueryString('ID');
    } else {
        qUrl = "/api/trashmail.xml?MAILID=" + Request.QueryString('ID');
    }

    var xmlHttp = initxmlhttp();
    xmlHttp.open("GET", qUrl, false);
    xmlHttp.send();

    if (xmlHttp.status == 200) {
        var xmldom = xmlHttp.responseXML;
        xmldom.documentElement.normalize();
        var responseNode = xmldom.documentElement.childNodes.item(0);
        if (responseNode.tagName == "response") {
            var errno = responseNode.getAttribute("errno")
            if (errno == "0" || errno == 0) {
                window.opener.delete_maillist_row(Request.QueryString('ID'));
                window.close();
            }
        }
    }
}

function remail() {
    var url = "writemail.html?TYPE=1&ID=" + Request.QueryString('ID');
    window.location.href = url;
}

function remail_all() {
    var url = "writemail.html?TYPE=2&ID=" + Request.QueryString('ID');
    window.location.href = url;
}

function forward_mail() {
    var url = "writemail.html?TYPE=3&ID=" + Request.QueryString('ID');
    window.location.href = url;
}

function flag_mail(flag) {
    if (flag == true)
        strFlag = "yes";
    else
        strFlag = "no";

    var qUrl = "/api/flagmail.xml?ID=" + Request.QueryString('ID') + "&FLAG=" + strFlag;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.opener.update_mail_flag(Request.QueryString('ID'), flag);
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function seen_mail(flag) {
    if (flag == true)
        strFlag = "yes";
    else
        strFlag = "no";

    var qUrl = "/api/seenmail.xml?ID=" + Request.QueryString('ID') + "&SEEN=" + strFlag;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.opener.update_mail_seen(Request.QueryString('ID'), flag);
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function ok_copy_mail() {
    var strDirs = "";
    for (var x = 0; x < document.getElementsByName('seldir1').length; x++) {
        if (document.getElementsByName('seldir1')[x].checked == true) {
            strDirs += document.getElementsByName('seldir1')[x].value + "*";
        }
    }
    if (strDirs == "") {
        alert(LANG_RESOURCE['NOSELECTED_FOLDER_WARNING']);
        return -1;
    }
    copy_mail(strDirs);

    return 0;
}

function ok_move_mail() {
    var strDirs = "";
    for (var x = 0; x < document.getElementsByName('seldir2').length; x++) {
        if (document.getElementsByName('seldir2')[x].checked == true) {
            strDirs += document.getElementsByName('seldir2')[x].value + "*";
        }
    }
    if (strDirs == "") {
        alert(LANG_RESOURCE['NOSELECTED_FOLDER_WARNING']);
        return -1;
    }
    move_mail(strDirs);

    return 0;
}

function do_copy_mail(mid, todirs) {
    var qUrl = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        var strmid = "mailtr" + mid;
        var trid = window.opener.$id(strmid);

        if (trid == null)
            return false;

        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.opener.refresh();
                    $("#PROCESSING_DIV").dialog("close");
                    return true;
                } else {
                    return false;
                }
            }
        } else {
            $id('PROCESSING_TBL').rows[0].cells[1].innerHTML = LANG_RESOURCE['COPY_MAIL_ONGOING'];
            $("#PROCESSING_DIV").dialog();
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");

    return true;
}

function show_copy_mail_dir_div() {
    $("#DIRS_DIV1").dialog({
        height: 500,
        width: 300,
        modal: false,
        title: LANG_RESOURCE['FOLDER'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                if (ok_copy_mail() != -1)
                    $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('DIRS_DIV2').style.display = "none";
    var Pos = GetObjPos($id('COPYMAIL'))
    show_dirs($id('DIRS_DIV1'), $id('DIRTBL1'), Pos.x, Pos.y + Pos.h + 2, "seldir1");
}

function show_move_mail_dir_div() {
    $("#DIRS_DIV2").dialog({
        height: 500,
        width: 300,
        modal: false,
        title: LANG_RESOURCE['FOLDER'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                if (ok_move_mail() != -1)
                    $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('DIRS_DIV1').style.display = "none";

    var Pos = GetObjPos($id('MOVEMAIL'))
    show_dirs($id('DIRS_DIV2'), $id('DIRTBL2'), Pos.x, Pos.y + Pos.h + 2, "seldir2");
}

function copy_mail(todirs) {
    do_copy_mail(Request.QueryString('ID'), todirs);
}

function do_move_mail(mid, todirs) {
    var qUrl = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        var strmid = "mailtr" + mid;
        var trid = window.opener.$id(strmid);

        if (trid == null)
            return false;

        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $("#PROCESSING_DIV").dialog("close");

                    window.opener.delete_maillist_row(mid);

                    window.opener.refresh();
                } else { }
            }
        } else {
            $id('PROCESSING_TBL').rows[0].cells[1].innerHTML = LANG_RESOURCE['MOVE_MAIL_ONGOING'];
            $("#PROCESSING_DIV").dialog();
        }

    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");

    return true;
}

function move_mail(todirs) {
    do_move_mail(Request.QueryString('ID'), todirs);
}

function TableFormat(arrText) {

    var strTbl = "<table border=\"0\" cellpadding=\"2\" cellspacing=\"2\"><tr>";
    for (var c = 0; c < arrText.length; c++) {
        if (arrText[c] == null)
            arrText[c] = "";

        strTbl += "<td>" + (arrText[c]) + "</td>";
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
                            $id("cccol").style.display = "none";
                        }

                        $id("mailcontent").style.width = $id('mailcontentframe').offsetWidth - 5;
                        $id("mailcontent").style.height = $id('mailcontentframe').offsetHeight - 5;

                        if (strCalendarContent != "") {
                            var vCal = new vCalendar();
                            calDetail = vCal.Parse(strCalendarContent);
                            strCalendarContent = "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\"><tr bgcolor=\"#C0C0C0\"><td class=\"gray\">";
                            strCalendarContent += "<table border=\"0\"><tr height=\"22\"><td width=\"22\" valign=\"middle\"><img src=\"calendar.gif\"></td><td valign=\"middle\"><b>" + (calDetail.get("METHOD") == "REQUEST" ? LANG_RESOURCE['EVENT_REQUEST'] : LANG_RESOURCE['EVENT_RESPONSE']) + "</b></td></tr></table>";
                            strCalendarContent += "</td></tr></table>";

                            strCalendarContent += "<table border=\"0\" width=\"100%\" bordercolorlight=\"#C0C0C0\" cellpadding=\"0\" cellspacing=\"0\" bordercolordark=\"#FFFFFF\" class=\"gray\">";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_SUMMARY_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/SUMMARY")))) + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_LOCATION_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/LOCATION")))) + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_CREATED_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/CREATED")), "[" + TextToHTML(calDetail.get("VTIMEZONE/TZID")) + "]")) + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" class=\"gray\"align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_MODIFIED_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/LAST-MODIFIED")), "[" + TextToHTML(calDetail.get("VTIMEZONE/TZID")) + "]")) + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_START_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/DTSTART/DT")), "[" + TextToHTML(calDetail.get("VEVENT/DTSTART/TZID")) + "]")) + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_END_DESC'] + "</b></td><td class=\"gray\">" + TableFormat(Array(TextToHTML(calDetail.get("VEVENT/DTEND/DT")), "[" + TextToHTML(calDetail.get("VEVENT/DTEND/TZID")) + "]")) + "</td></tr>";

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

                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_ORGNIZER_DESC'] + "</b></td><td class=\"gray\">" + strORGANIZERbl + "</td></tr>";
                            strCalendarContent += "<tr height=\"22\"><td class=\"gray\" align=\"right\" valign=\"middle\" bgcolor=\"#F0F0F0\" width=\"90\"><b>" + LANG_RESOURCE['EVENT_INVITEE_DESC'] + "</b></td><td class=\"gray\">" + strATTENDEETbl + "</td></tr>";
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

                        $id("mailfrom").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strFrom) + "\" readonly>";
                        $id("mailto").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strTo) + "\" readonly>";
                        $id("mailcc").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strCc) + "\" readonly>";
                        $id("maildate").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + TextToHTML(strDate) + "\" readonly>";
                        $id("mailsubject").innerHTML = "<input class=\"text3\" size=\"110\" type=\"text\" value=\"" + (strSubject == "" ? LANG_RESOURCE['NO_SUBJECT'] : TextToHTML(strSubject)) + "\" readonly>";

                        var strrpl = "/api/attachment.cgi?ID=";
                        strrpl += strID;
                        strrpl += "&CONTENTID=";

                        var regS1 = new RegExp("cid:", "gi");
                        strHtmlContent = strHtmlContent.replace(regS1, strrpl);
                        if (strCalendarContent == "") {
                            $id("mailcontent").innerHTML = (strHtmlContent != "" ? strHtmlContent : TextToHTML(strTextContent)) + (strPreview != "" ? ("<p></p><hr><b>" + att_count + LANG_RESOURCE['ATTACHMENT_NUM'] + "</b>" + strPreview) : "");
                        } else {
                            $id("timecol").style.display = "none";

                            $id("mailcontent").innerHTML = strCalendarContent + (strPreview != "" ? ("<p></p><hr><b>" + att_count + LANG_RESOURCE['ATTACHMENT_NUM'] + "</b>" + strPreview) : "");
                        }
                    } else {
                        $id("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>" + LANG_RESOURCE['LOADING_FAILED'] + "</td></tr></table>";
                    }
                }
            } else {
                $id("mailcontent").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>" + LANG_RESOURCE['LOADING_FAILED'] + "</td></tr></table>";
            }
        } else {
            $id("mailfrom").innerHTML = "<img src=\"loading.gif\">";
            $id("mailto").innerHTML = "<img src=\"loading.gif\">";
            $id("mailcc").innerHTML = "<img src=\"loading.gif\">";
            $id("maildate").innerHTML = "<img src=\"loading.gif\">";
            $id("mailsubject").innerHTML = "<img src=\"loading.gif\">";
            $id("mailcontent").innerHTML = "<img src=\"loading.gif\">";
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function refresh() { }

function init() {

    $id('GLOBALPATH').innerHTML = (Request.QueryString('GPATH') == null ? ("/" + LANG_RESOURCE['MAILBOX_INBOX']) : decodeURIComponent(Request.QueryString('GPATH')));

    read_mail(Request.QueryString('ID'));
}

function uninit() { }

$(document).ready(function () {
    $('#REMAIL').click(function () {
        remail();
    });

    $('#RETOALL').click(function () {
        remail_all();
    });

    $('#FORWARD').click(function () {
        forward_mail();
    });

    $('#FLAG').click(function () {
        flag_mail(true);
    });

    $('#UNFLAG').click(function () {
        flag_mail(false);
    });

    $('#SEEN').click(function () {
        seen_mail(true);
    });

    $('#UNSEEN').click(function () {
        seen_mail(false);
    });

    $('#COPYMAIL').click(function () {
        show_copy_mail_dir_div();
    });

    $('#MOVEMAIL').click(function () {
        show_move_mail_dir_div();
    });

    $('#DELMAIL').click(function () {
        delmail();
    });
});