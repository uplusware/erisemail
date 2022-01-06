var sending_status = false;
var sent_status = false;
var saving_status = false;
var uploading_status = false;

var uploaded_file_list = new Array();

function uploading(status) {
    uploading_status = status;
    toggle_onbeforeunload(uploading_status);
}

function toggle_onbeforeunload(val) {
    if (val) {
        window.onbeforeunload = function (e) {
            if (sending_status || sent_status || saving_status || uploading_status) {
                return (LANG_RESOURCE['QUIT_WEB_WARNING_FOR_PROCESSING_MAIL']);
            } else if (isChange) {
                return (LANG_RESOURCE['QUIT_WEB_WARNING_FOR_UNSAVED_UNSENT_MAIL']);
            } else {
                return (LANG_RESOURCE['QUIT_WEB_WARNING']);
            }
        }
    } else {
        if (!isChange)
            window.onbeforeunload = null;
    }
}

function RTC(strid) {
    if (navigator.appName != "Netscape") {
        return this.document.frames[strid].document;
    } else {
        return this.document.getElementById(strid).contentWindow.document;
    }
}

var arrColor = [
    ['000000', '993300', '333300', '003300', '003366', '000080', '333399', '333333'],
    ['800000', 'FF6600', '808000', '008000', '008080', '0000FF', '666699', '808080'],
    ['FF0000', 'FF9900', '99CC00', '339966', '33CCCC', '3366FF', '800080', '999999'],
    ['FF00FF', 'FFCC00', 'FFFF00', '00FF00', '00FFFF', '00CCFF', '993366', 'C0C0C0'],
    ['FF99CC', 'FFCC99', 'FFFF99', 'CCFFCC', 'CCFFFF', '99CCFF', 'CC99FF', 'FFFFFF']
];

function colorCmd(cmd, divid, color) {
    RTC("richeditor").execCommand(cmd, false, color);

    $id(divid).style.display = "none";
}

function sizeCmd(size) {
    if (size == '0' || size == 0)
        return;
    RTC("richeditor").execCommand('FontSize', false, size);

    $id('fontSizeTbl').style.display = "none";
}

function paintColorDiv(cmd, divid) {
    var Str = "<table width='100%' border='0' cellspacing='2' cellpadding='2'><tbody>";
    for (var i = 0; i < arrColor.length; i++) {
        Str += "<tr>";
        for (var j = 0; j < arrColor[i].length; j++) {
            Str += "<td class='td' style='background-color:#" + arrColor[i][j] + ";border:1px solid #fafafa;'"
                + " onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\";' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\";'"
                + " onclick='colorCmd(\"" + cmd + "\", \"" + divid + "\", \"#" + arrColor[i][j] + "\")'>"
                + "<img style='width:12px;height:12px' " + " style='background-color:#" + arrColor[i][j] + ";border:solid 1px #cccccc'/></td>";
        }
        Str += "</tr>";
    }
    Str += "</tbody></table>";
    return Str;
}

function paintSizeDiv(cmd, divid) {
    var Str = "<table width='100%' border='0' cellspacing='2' cellpadding='2'><tbody>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"1\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='1'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"2\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='2'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"3\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='3'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"4\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='4'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"5\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='5'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"6\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='6'>T</font></td></tr>";
    Str += "<tr><td bgcolor=\"#EEEEEE\" onclick='sizeCmd(\"7\");' onmouseover='this.mousepoint = 99; this.style.cursor =\"pointer\"; this.style.background=\"#FFFFCC\"' onmouseout='this.mousepoint = 99;this.style.cursor =\"default\"; this.style.background=\"#EEEEEE\"'><font size='7'>T</font></td></tr>";
    Str += "</tbody></table>";
    return Str;
}

function popforeDiv() {
    if ($id('foreColorTbl').style.display == 'block')
        $id('foreColorTbl').style.display = 'none';
    else
        $id('foreColorTbl').style.display = 'block';

    $id('backColorTbl').style.display = 'none';
    $id('fontSizeTbl').style.display = 'none';

    $id('foreColorTbl').style.left = 89;
    $id('foreColorTbl').style.top = 258;
}

function popbackDiv() {
    if ($id('backColorTbl').style.display == 'block')
        $id('backColorTbl').style.display = 'none';
    else
        $id('backColorTbl').style.display = 'block';

    $id('foreColorTbl').style.display = 'none';
    $id('fontSizeTbl').style.display = 'none';

    $id('backColorTbl').style.left = 115;
    $id('backColorTbl').style.top = 258;
}

function popsizeDiv() {
    if ($id('fontSizeTbl').style.display == 'block')
        $id('fontSizeTbl').style.display = 'none';
    else
        $id('fontSizeTbl').style.display = 'block';

    $id('foreColorTbl').style.display = 'none';
    $id('backColorTbl').style.display = 'none';

    $id('fontSizeTbl').style.left = 150;
    $id('fontSizeTbl').style.top = 258;
}

function ontimer() {
    if ($id("CONTENT").value != $id('richeditor').contentWindow.document.body.innerHTML) {
        isChange = true;

        toggle_onbeforeunload(isChange)

        $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;
    }

    setTimeout('ontimer()', 1000);
}

function init() {
    sending_status = false;
    sent_status = false;
    saving_status = false;
    uploading_status = false;
    isChange = false;

    $id('GLOBALPATH').innerHTML = (Request.QueryString('GPATH') == null ? ("/" + LANG_RESOURCE['MAILBOX_DRAFTS']) : decodeURIComponent(Request.QueryString('GPATH')));

    strID = Request.QueryString('ID');
    strType = Request.QueryString('TYPE');

    if ((strID != null) && (strType != null)) {
        read_mail(strID, strType);
    } else {
        $id('TO_ADDRS').value = Request.QueryString('TOADDRS') == null ? "" : Request.QueryString('TOADDRS');
    }

    $id('foreColorTbl').innerHTML = paintColorDiv("ForeColor", "foreColorTbl");
    $id('backColorTbl').innerHTML = paintColorDiv("BackColor", "backColorTbl");
    $id('fontSizeTbl').innerHTML = paintSizeDiv("FontSize", "fontSizeTbl");

    $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

    ontimer();
}

function upfile() {
    isChange = true;
    toggle_onbeforeunload(isChange);
}

function delmail(force) {
    if ($id('DRAFTID').value == null || $id('DRAFTID').value == "" || $id('DRAFTID').value == -1) {
        alert(LANG_RESOURCE['NON_SAVED_DRAFT_WARNING'])
        return false;
    }
    do_trashmail();

    if (window.opener.refresh)
        window.opener.refresh();

    isChange = false;
    window.onbeforeunload = null;

    window.close();
}

function do_delsentmail() {
    if ($id('DRAFTID').value == null || $id('DRAFTID').value == "" || $id('DRAFTID').value == -1) {
        return;
    }

    isChange = false;
    toggle_onbeforeunload(isChange);

    var api_url = "/api/delmail.xml?MAILID=" + $id('DRAFTID').value;
    $.ajax({
        url: api_url,
        async: false,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    return true;
                }
            }
        }
    });

    return false;
}

function do_trashmail() {
    if ($id('DRAFTID').value == null || $id('DRAFTID').value == "" || $id('DRAFTID').value == -1) {
        return;
    }

    isChange = false;
    toggle_onbeforeunload(isChange);

    var api_url = "/api/trashmail.xml?MAILID=" + $id('DRAFTID').value;
    $.ajax({
        url: api_url,
        async: false,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    return true;
                }
            }
        }
    });
    return false;
}

function flag_mail(flag) {
    if (Request.QueryString('ID') == null || Request.QueryString('ID') == "") {
        alert(LANG_RESOURCE['NON_SAVED_DRAFT_WARNING'])
        return;
    }

    var api_url = "/api/flagmail.xml?ID=" + Request.QueryString('ID') + "&FLAG=" + (flag == true ? "yes" : "no");
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    window.opener.update_mail_flag(Request.QueryString('ID'), flag);
                }
            }
        }
    });
}

function seen_mail(flag) {
    if (Request.QueryString('ID') == null || Request.QueryString('ID') == "") {
        alert(LANG_RESOURCE['NON_SAVED_DRAFT_WARNING'])
        return;
    }

    var api_url = "/api/seenmail.xml?ID=" + Request.QueryString('ID') + "&SEEN=" + (flag == true ? "yes" : "no");
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.opener.update_mail_seen(Request.QueryString('ID'), flag);
                }
            }
        }
    });
}

function do_savesent() {
    var uploadedfiles = "";
    for (var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++) {
        uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
        uploadedfiles += "*"
    }
    $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

    var api_data = "TO_ADDRS=" + encodeURIComponent($id('TO_ADDRS').value) + "&";
    api_data += "CC_ADDRS=" + encodeURIComponent($id('CC_ADDRS').value) + "&";
    api_data += "BCC_ADDRS=" + encodeURIComponent($id('BCC_ADDRS').value) + "&";
    api_data += "SUBJECT=" + encodeURIComponent($id('SUBJECT').value) + "&";
    api_data += "CONTENT=" + encodeURIComponent($id('CONTENT').value) + "&";
    api_data += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
    var api_url = "api/savesent.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        contentType: "application/x-www-form-urlencoded",
        beforeSend: function (xmldom) {
            sent_status = true;
            toggle_onbeforeunload(sent_status);

            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SAVE_MAIL_TO_SENT_FOLDER_ONGOING'] + " - <img src=\"waiting.gif\" />";
            $("#PROCESSING_DIV").dialog();
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    do_delsentmail();

                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SAVE_MAIL_TO_SENT_FOLDER_OK'];
                    $("#PROCESSING_DIV").dialog();

                    if (window.opener && window.opener.refresh)
                        window.opener.refresh();
                    else if (window.opener && window.opener.childframe1 && window.opener.childframe1.refresh)
                        window.opener.childframe1.refresh();

                    isChange = false;
                    window.onbeforeunload = null;

                    window.close();
                } else {
                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>" + LANG_RESOURCE['SAVE_MAIL_TO_SENT_FOLDER_FAILED'] + "</font>";
                    $("#PROCESSING_DIV").dialog();
                }
            }
        },
        error: function (xmldom) {
            sent_status = true;
            toggle_onbeforeunload(sent_status);

            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SAVE_MAIL_TO_SENT_FOLDER_ONGOING'] + " - <img src=\"waiting.gif\" />";
            $("#PROCESSING_DIV").dialog();
        }
    });
}

function send_mail() {
    do_send_mail();
}

function do_send_mail() {
    if ($id('TO_ADDRS').value == "") {
        alert(LANG_RESOURCE['EMPTY_MAIL_TO_WARNING']);
        return;
    }

    if ($id('SUBJECT').value == "") {
        if (confirm(LANG_RESOURCE['EMPTY_SUBJECT_WARNING']) == false)
            return;
    }

    var uploadedfiles = "";
    for (var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++) {
        uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
        uploadedfiles += "*"
    }
    $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

    var api_data = "TO_ADDRS=" + encodeURIComponent($id('TO_ADDRS').value) + "&";
    api_data += "CC_ADDRS=" + encodeURIComponent($id('CC_ADDRS').value) + "&";
    api_data += "BCC_ADDRS=" + encodeURIComponent($id('BCC_ADDRS').value) + "&";
    api_data += "SUBJECT=" + encodeURIComponent($id('SUBJECT').value) + "&";
    api_data += "CONTENT=" + encodeURIComponent($id('CONTENT').value) + "&";
    api_data += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
    var api_url = "api/sendmail.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        contentType: "application/x-www-form-urlencoded",
        beforeSend: function (xmldom) {
            sending_status = true;
            toggle_onbeforeunload(sending_status);
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SEND_MAIL_ONGOING'] + " - <img src=\"waiting.gif\" />";
            $("#PROCESSING_DIV").dialog();
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    isChange = false;
                    toggle_onbeforeunload(isChange);
                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SEND_MAIL_OK'];
                    $("#PROCESSING_DIV").dialog();

                    do_savesent();
                } else {
                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>" + LANG_RESOURCE['SEND_MAIL_FAILED_FOR_HUGE_SIZE'] + "</font>";
                    $("#PROCESSING_DIV").dialog();
                }
            }
        },
        error: function (xmldom) {
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>" + LANG_RESOURCE['SEND_MAIL_FAILED'] + "</font>";
            $("#PROCESSING_DIV").dialog();
        }
    });
}

function save_draft() {
    if (isChange == true) {
        do_savedraft();
    }
}

function do_savedraft() {
    var uploadedfiles = "";
    for (var x = 0; x < document.getElementsByName('TMPFILENAME').length; x++) {
        uploadedfiles += document.getElementsByName('TMPFILENAME')[x].value;
        uploadedfiles += "*"
    }
    $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

    var api_data = "DRAFTID=" + $id('DRAFTID').value + "&";
    api_data += "TO_ADDRS=" + encodeURIComponent($id('TO_ADDRS').value) + "&";
    api_data += "CC_ADDRS=" + encodeURIComponent($id('CC_ADDRS').value) + "&";
    api_data += "BCC_ADDRS=" + encodeURIComponent($id('BCC_ADDRS').value) + "&";
    api_data += "SUBJECT=" + encodeURIComponent($id('SUBJECT').value) + "&";
    api_data += "CONTENT=" + encodeURIComponent($id('CONTENT').value) + "&";
    api_data += "ATTACHFILES=" + encodeURIComponent(uploadedfiles);
    var api_url = "api/savedraft.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        contentType: "application/x-www-form-urlencoded",
        beforeSend: function (xmldom) {
            saving_status = true;
            toggle_onbeforeunload(saving_status);
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SAVE_DRAFT_ONGOING'] + " - <img src=\"waiting.gif\" />";
            $("#PROCESSING_DIV").dialog();
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    isChange = false;
                    toggle_onbeforeunload(isChange);

                    var bodyList = responseNode.childNodes;
                    for (var i = 0; i < bodyList.length; i++) {
                        if (bodyList.item(i).tagName == "draftid") {
                            $id('DRAFTID').value = bodyList.item(i).childNodes[0].nodeValue;
                        }
                    }

                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['SAVE_DRAFT_OK'];
                    $("#PROCESSING_DIV").dialog();

                } else {

                    $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>" + LANG_RESOURCE['SAVE_DRAFT_ERROR_FOR_HUGE_SIZE'] + "</font>";
                    $("#PROCESSING_DIV").dialog();

                }
            }
        },
        error: function (xmldom) {
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = "<font color='#FF0000'>" + LANG_RESOURCE['SAVE_DRAFT_ERROR'] + "</font>";
            $("#PROCESSING_DIV").dialog();
        }
    });
}

function read_mail(strID, strType) {
    var api_url = "/api/readmail.xml?EDIT=yes&ID=" + strID;
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno");

                if (errno == "0") {
                    var strFrom,
                        strTo,
                        strCc,
                        strSubject,
                        strTextContent,
                        strHtmlContent,
                        strAttach;
                    strFrom = "";
                    strTo = "";
                    strCc = "";
                    strSubject = "";
                    strTextContent = "";
                    strHtmlContent = "";

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
                        } else if (bodyList.item(i).tagName == "text_content") {
                            strTextContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                            strTextContent = TextToHTML(strTextContent);
                        } else if (bodyList.item(i).tagName == "html_content") {
                            strHtmlContent += bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                        } else if (bodyList.item(i).tagName == "attach") {
                            strAttach = bodyList.item(i).childNodes[0] == null ? "" : bodyList.item(i).childNodes[0].nodeValue;
                        }
                    }

                    if (strType == "4") {
                        $id('DRAFTID').value = strID;

                        $id("TO_ADDRS").value = strTo;
                        $id("CC_ADDRS").value = strCc;
                        $id("SUBJECT").value = strSubject;
                        $id("CONTENT").value = (strHtmlContent != "" ? strHtmlContent : strTextContent);
                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;
                        $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;
                        if (strAttach != "") {
                            extract_att(strID);
                        }

                    } else if (strType == "3") {
                        $id("SUBJECT").value = "Fwd: " + strSubject;

                        $id("CONTENT").value = (strHtmlContent != "" ? strHtmlContent : strTextContent);
                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;

                        strReply = "<br><br><br><br><hr>";
                        strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_FROM_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_TO_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
                        if (strCc != "")
                            strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_CC_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_DATE_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_SUBJECT_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
                        strReply += "</table>";

                        strReply += $id('richeditor').contentWindow.document.body.innerHTML;

                        $id("CONTENT").value = strReply;

                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;
                        $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

                        if (strAttach != "") {
                            extract_att(strID);
                        }

                    } else if (strType == "2") {
                        var to_addrs = strFrom + "," + strTo;
                        var to_addrs_arr = to_addrs.split(",");
                        var to_addrs_result = new Array();
                        to_addrs = "";
                        for (var x = 0; x < to_addrs_arr.length; x++) {
                            var res = retrieve(to_addrs_arr[x]);
                            if (res != null) {
                                var same = false;
                                for (var a = 0; a < to_addrs_result.length; a++) {
                                    if (res == to_addrs_result[a]) {
                                        same = true;
                                        break;
                                    }
                                }
                                if (!same) {
                                    to_addrs_result.push(res);
                                    if (to_addrs != "")
                                        to_addrs += ",";
                                    to_addrs += res;
                                }
                            }
                        }

                        var cc_addrs = strCc;
                        var cc_addrs_arr = cc_addrs.split(",");
                        var cc_addrs_result = new Array();
                        cc_addrs = "";
                        for (var x = 0; x < cc_addrs_arr.length; x++) {
                            var res = retrieve(cc_addrs_arr[x]);
                            if (res != null) {
                                var same = false;
                                for (var a = 0; a < cc_addrs_result.length; a++) {
                                    if (res == cc_addrs_result[a]) {
                                        same = true;
                                        break;
                                    }
                                }

                                for (var a = 0; a < to_addrs_result.length; a++) {
                                    if (res == to_addrs_result[a]) {
                                        same = true;
                                        break;
                                    }
                                }

                                if (!same) {
                                    cc_addrs_result.push(res);
                                    if (cc_addrs != "")
                                        cc_addrs += ","
                                    cc_addrs += res;
                                }
                            }
                        }

                        $id("TO_ADDRS").value = to_addrs;
                        $id("CC_ADDRS").value = cc_addrs;

                        $id("SUBJECT").value = "Re: " + strSubject;

                        $id("CONTENT").value = (strHtmlContent != "" ? strHtmlContent : strTextContent);
                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;

                        strReply = "<br><br><br><br><hr>";
                        strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_FROM_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_TO_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
                        if (strCc != "")
                            strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b" + LANG_RESOURCE['MAIL_CC_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_DATE_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_SUBJECT_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
                        strReply += "</table>";

                        strReply += $id('richeditor').contentWindow.document.body.innerHTML;

                        $id("CONTENT").value = strReply;

                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;
                        $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

                    } else if (strType == "1") {
                        $id("TO_ADDRS").value = strFrom;
                        $id("SUBJECT").value = "Re: " + strSubject;

                        $id("CONTENT").value = (strHtmlContent != "" ? strHtmlContent : strTextContent);
                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;

                        strReply = "<br><br><br><br><hr>";
                        strReply += "<table border='0' bgcolor='#EFEFEF' bordercolorlight='#C0C0C0' bordercolordark='#FFFFFF' cellpadding='5' cellspacing='1' width='100%'>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_FROM_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strFrom) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_TO_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strTo) + "</td></tr>";
                        if (strCc != "")
                            strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_CC_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strCc) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_DATE_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strDate) + "</td></tr>";
                        strReply += "<tr><td bgcolor='#EFEFEF' align='right' width='80'><b>" + LANG_RESOURCE['MAIL_SUBJECT_DESC'] + "</b></td><td bgcolor='#FFFFFF' align='left' width='910'>" + TextToHTML(strSubject) + "</td></tr>";
                        strReply += "</table>";

                        strReply += $id('richeditor').contentWindow.document.body.innerHTML;

                        $id("CONTENT").value = strReply;

                        $id('richeditor').contentWindow.document.body.innerHTML = $id("CONTENT").value;
                        $id("CONTENT").value = $id('richeditor').contentWindow.document.body.innerHTML;

                    }
                }
            }
        }
    });
}

function extract_att(strID) {
    var api_url = "/api/extractattach.xml?ID=" + strID;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id('attach_flag').innerHTML = "<img src=\"loading.gif\">";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno");

                if (errno == "0") {
                    var strTmpname,
                        strFilename;
                    var attList = responseNode.childNodes;
                    for (var i = 0; i < attList.length; i++) {
                        if (attList.item(i).tagName == "attach") {
                            strTmpname = attList.item(i).getAttribute("tmpname") == null ? "" : attList.item(i).getAttribute("tmpname");
                            strFilename = attList.item(i).getAttribute("filename") == null ? "" : attList.item(i).getAttribute("filename");
                            nAttsize = attList.item(i).getAttribute("attsize") == null ? 0 : attList.item(i).getAttribute("attsize");
                            if (strTmpname != "" && strFilename != "") {
                                show_attachs(strTmpname, strFilename, nAttsize);
                            }
                        }
                    }
                    $id('attach_flag').innerHTML = "";
                } else {
                    $id('attach_flag').innerHTML = "<img src=\"alert.gif\">";
                }
            }
        },
        error: function (xmldom) {
            $id('attach_flag').innerHTML = "<img src=\"alert.gif\">";
        }
    });
}

function del_attach(selfid, tmpname) {
    if (del_uploaded(tmpname)) {
        var tObj = $id('SHOW_UPLOADEDFILES');
        tObj.deleteRow($id(selfid).rowIndex);
    }
}

function del_uploaded(tmpname) {
    var api_url = "/api/deluploaded.xml?UPLOADEDFILE=" + tmpname;
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno");
                if (errno == "0") {
                    return true
                }
            }
        }
    });

    return false;
}

function show_attachs(tmpname, showname, attsize) {
    var tObj = $id('SHOW_UPLOADEDFILES');
    tr = tObj.insertRow(tObj.rows.length);
    tr.setAttribute("id", "file" + tr.rowIndex);
    var td1 = tr.insertCell(0);
    td1.innerHTML = "<input name=\"TMPFILENAME\" id=\"TMPFILENAME\" type=\"hidden\" value=\"" + tmpname + "\"><input type=\"image\" src=\"delete.gif\" onclick=\"del_attach('" + tr.getAttribute('id') + "','" + tmpname + "')\">";
    var td2 = tr.insertCell(1);
    td2.innerHTML = "<a href=\"/api/gettmpfile.cgi?TMPFILENAME=" + tmpname + "\" target=\"_blank\">" + TextToHTML(decodeURIComponent(showname)) + "</a>(" + (attsize == 0 ? "0" : (Math.round(attsize / 1024 * 100) / 100 == 0 ? "0.01" : Math.round(attsize / 1024 * 100) / 100)) + "KB)";
}

function uninit() {
    for (var x = 0; x < uploaded_file_list.length; x++) {
        del_uploaded(uploaded_file_list[x]);
    }
    return true;
}

function sel_alluser(sel) {
    for (var x = 0; x < document.getElementsByName('seluser').length; x++) {
        document.getElementsByName('seluser')[x].checked = sel;
    }
}

function append_addrs() {
    for (var x = 0; x < document.getElementsByName('seluser').length; x++) {
        if (document.getElementsByName('seluser')[x].checked == true) {
            isChange = true;
            toggle_onbeforeunload(isChange)
            if ($id('ADDR_BOOK_DIV').getAttribute("who") == "1") {
                if ($id('TO_ADDRS').value != "")
                    $id('TO_ADDRS').value += ", "
                $id('TO_ADDRS').value += document.getElementsByName('seluser')[x].value;

            } else if ($id('ADDR_BOOK_DIV').getAttribute("who") == "2") {
                if ($id('CC_ADDRS').value != "")
                    $id('CC_ADDRS').value += ", "
                $id('CC_ADDRS').value += document.getElementsByName('seluser')[x].value;

            } else if ($id('ADDR_BOOK_DIV').getAttribute("who") == "3") {
                if ($id('BCC_ADDRS').value != "")
                    $id('BCC_ADDRS').value += ", "
                $id('BCC_ADDRS').value += document.getElementsByName('seluser')[x].value;
            }
        }
    }
}

function load_users(who, orderby, desc) {
    var api_url = "/api/listusers.xml?ORDER_BY=" + orderby + "&DESC=" + (desc == null ? '' : desc);
    $.ajax({
        url: api_url,
        async: false,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    var strTmp;
                    var userList = responseNode.childNodes;

                    clear_table_without_header($id('USERTBL'));

                    for (var i = 0; i < userList.length; i++) {
                        if (userList.item(i).tagName == "user") {
                            var image;
                            if (userList.item(i).getAttribute("status") == "Active") {
                                if (userList.item(i).getAttribute("role") == "Administrator")
                                    image = "admin.gif";
                                else {
                                    if (userList.item(i).getAttribute("type") == "Group")
                                        image = "group.gif";
                                    else
                                        image = "member.gif";
                                }
                            } else {
                                if (userList.item(i).getAttribute("type") == "Group")
                                    image = "inactive_group.gif";
                                else
                                    image = "inactive_member.gif";
                            }

                            tr = $id('USERTBL').insertRow($id('USERTBL').rows.length);

                            var td0 = tr.insertCell(0);
                            td0.valign = "middle";
                            td0.align = "center";
                            td0.height = "22";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<input type=\"checkbox\" name=\"seluser\" id=\"seluser\" value=\"" + userList.item(i).getAttribute("name") + "@" + userList.item(i).getAttribute("domain") + "\">";

                            var td1 = tr.insertCell(1);
                            td1.valign = "middle";
                            td1.align = "center";
                            td1.height = "22";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = "<img src=\"" + image + "\" />";

                            var td2 = tr.insertCell(2);
                            td2.valign = "middle";
                            td2.align = "left";
                            td2.height = "22";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = userList.item(i).getAttribute("name");

                            var td3 = tr.insertCell(3);
                            td3.valign = "middle";
                            td3.align = "left";
                            td3.height = "22";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = userList.item(i).getAttribute("alias");
                        }
                    }
                }
            }
        }
    });
}

function showbook(who, orderby) {
    sel_alluser(false);
    $("#ADDR_BOOK_DIV").dialog({
        height: 500,
        width: 600,
        modal: false,
        title: LANG_RESOURCE['ADDRESS_BOOK'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                append_addrs();
                $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('ADDR_BOOK_DIV').setAttribute("who", who);

    $id('selalluser').checked = false;

    if ($id('ADDR_BOOK_DIV').getAttribute("loaded") != "true") {
        $id('USERTBL').setAttribute('orderby', orderby);
        load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));

        $id('ADDR_BOOK_DIV').setAttribute("loaded", "true");
    }
}

function do_copy_mail(mid, todirs) {
    var trid = window.opener.$id("mailtr" + mid);
    if (trid == null)
        return false;

    var api_url = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['COPY_MAIL_ONGOING'];
            $("#PROCESSING_DIV").dialog();
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    if (window.opener && window.opener.refresh)
                        window.opener.refresh();
                    $("PROCESSING_DIV").dialog("close");
                    return true;
                } else {
                    return false;
                }
            }
        }
    });

    return true;
}

function copy_mail(todir) {
    if ($id('DRAFTID').value == null || $id('DRAFTID').value == "" || $id('DRAFTID').value == -1) {
        alert(LANG_RESOURCE['NON_SAVED_DRAFT_WARNING']);
        return;
    }
    do_copy_mail($id('DRAFTID').value, todir);
}

function do_move_mail(mid, todirs) {
    var trid = window.opener.$id("mailtr" + mid);
    if (trid == null)
        return false;

    var api_url = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todirs;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id('PROCESSING_TBL').rows[0].cells[0].innerHTML = LANG_RESOURCE['MOVE_MAIL_ONGOING'];
            $("#PROCESSING_DIV").dialog();
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $("PROCESSING_DIV").dialog("close");

                    window.opener.delete_maillist_row(mid);

                    if (window.opener.refresh)
                        window.opener.refresh();
                } else {
                    return false;
                }
            }
        }
    });

    return true;
}

function move_mail(todir) {
    if ($id('DRAFTID').value == null || $id('DRAFTID').value == "" || $id('DRAFTID').value == -1) {
        alert(LANG_RESOURCE['NON_SAVED_DRAFT_WARNING']);
        return;
    }
    do_copy_mail($id('DRAFTID').value, todir);
}

function init_editor() {
    if (navigator.appName != "Netscape") {
        document.frames['richeditor'].document.designMode = 'on';
    } else {
        document.getElementById('richeditor').contentWindow.document.designMode = 'on';
        document.getElementById('richeditor').contentWindow.document.body.contentEditable = 'true';
    }
}

var isChange = false;

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

function sort_users(who, orderby) {
    $id('selalluser').checked = false;

    $id('USERTBL').setAttribute('orderby', orderby);

    clear_table_without_header($id('USERTBL'));

    if ($id('USERTBL').getAttribute(orderby) == 'true')
        $id('USERTBL').setAttribute(orderby, 'false');
    else
        $id('USERTBL').setAttribute(orderby, 'true');

    load_users(who, $id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));
}


$(document).ready(function () {
    init();

    $('#ADDRESS_BOOK1').click(function () {
        showbook(1, 'uname');
    });

    $('#ADDRESS_BOOK2').click(function () {
        showbook(2, 'uname');
    });

    $('#ADDRESS_BOOK3').click(function () {
        showbook(3, 'uname');
    });

    $('#SENDMAIL').click(function () {
        send_mail();
    });

    $('#SAVEDRAFT').click(function () {
        save_draft();
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

    $('#TO_ADDRS').change(function () {
        isChange = true;
        toggle_onbeforeunload(isChange);
    });

    $('#CC_ADDRS').change(function () {
        isChange = true;
        toggle_onbeforeunload(isChange);
    });

    $('#BCC_ADDRS').change(function () {
        isChange = true;
        toggle_onbeforeunload(isChange);
    });

    $('#SUBJECT').change(function () {
        isChange = true;
        toggle_onbeforeunload(isChange);
    });

    $('#CONTENT').change(function () {
        isChange = true;
        toggle_onbeforeunload(isChange);
    });

    $('#selalluser').click(function () {
        sel_alluser(this.checked);
    });

    $('#sort_by_name').click(function () {
        sort_users($id('ADDR_BOOK_DIV').getAttribute('who'), 'uname');
    });

    $('#sort_by_alias').click(function () {
        sort_users($id('ADDR_BOOK_DIV').getAttribute('who'), 'ualias')
    });

    $('#sort_by_name, #sort_by_alias').mouseover(function () {
        this.mousepoint = 99;
        this.style.cursor = 'pointer';
    });

    $('#sort_by_name, #sort_by_alias').mouseout(function () {
        this.mousepoint = 99;
        this.style.cursor = 'default';
    });
});

$(window).on('unload', function () {
    uninit();
})