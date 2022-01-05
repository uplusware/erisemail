function get_total_mail_num(dirid, tdobj) {
    var api_url = "/api/getmailnum.xml?DIRID=" + (dirid ? dirid : "");
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            tdobj.innerHTML = "<img src=\"loading.gif\">";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var mailnum;
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var countList = responseNode.childNodes;
                    for (var i = 0; i < countList.length; i++) {
                        if (countList.item(i).tagName == "count") {
                            tdobj.innerHTML = countList.item(i).childNodes[0] == null ? "0" : countList.item(i).childNodes[0].nodeValue + " " + LANG_RESOURCE['MAILS'];

                        }
                    }
                }
            }
        }
    });
}

function delete_label(dirid, dirname, tdobj) {
    var api_data = "DIRID=" + encodeURIComponent(dirid);
    var api_url = "/api/deletelabel.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table(window.parent.leftmenuframe.$id('DIRTBL'));
                    window.parent.leftmenuframe.load_dirs(-1, '', 'common', -1, 0);

                    clear_table($id('DIRTBL'));
                    travel_dirs(-1, '', -1);
                } else {
                    alert(LANG_RESOURCE['REMOVE'] + " '" + dirname + "' " + LANG_RESOURCE['FAILED']);
                }
            }
        }
    });
}

function do_create_label() {
    create_label($id('NEW_LABEL').value, $id('NEW_LABEL_DIV').getAttribute("dirid"));
    $id('NEW_LABEL').value = "";
}

function show_inputlabel_div(dirid) {
    $("#NEW_LABEL_DIV").dialog({
        height: 200,
        width: 300,
        modal: false,
        title: LANG_RESOURCE['NEW_FOLDER'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                do_create_label();
                $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('NEW_LABEL_DIV').setAttribute("dirid", dirid);
}

function output_dir(tblobj, pid, path, nodeObj, layer) {
    layer++;
    var a = 0;

    for (var i = 0; i < nodeObj.length; i++) {
        var strDisplayDirName = "";

        if (nodeObj.item(i).tagName == "dir") {
            var childrennum = nodeObj.item(i).getAttribute("childrennum");
            var strRealName = nodeObj.item(i).getAttribute("name");
            var strId = nodeObj.item(i).getAttribute("id");

            var tr = tblobj.insertRow(tblobj.rows.length);

            tr.onmouseover = function () {
                this.setAttribute("imagesrc", this.style.backgroundImage);
                this.style.backgroundImage = "url('focusbg.gif')";
            }
            tr.onmouseout = function () {
                this.style.backgroundImage = this.getAttribute("imagesrc");
            }

            var colorString = "#FFFF";
            for (var c = 0; c < 2; c++) {
                var color = 15 - layer;
                colorString += color.toString(16)
            }

            tr.style.backgroundColor = colorString;

            var td0 = tr.insertCell(0);
            td0.width = "300";
            var childnull = "";
            for (var y = 0; y < layer + 1; y++) {
                childnull += "<img src=\"childnull.gif\">";
            }

            var strIcon = "folder.gif";

            if (strRealName.toLowerCase() == "inbox") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_INBOX'];
                strIcon = "inbox.gif";
            } else if (strRealName.toLowerCase() == "sent") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_SENT'];
                strIcon = "sent.gif";
            } else if (strRealName.toLowerCase() == "drafts") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_DRAFTS'];
                strIcon = "draft.gif";
            } else if (strRealName.toLowerCase() == "trash") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_TRASH'];
                strIcon = "trash.gif";
            } else if (strRealName.toLowerCase() == "junk") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_JUNK'];
                strIcon = "junk.gif";
            } else {
                strDisplayDirName = strRealName;
                strIcon = "folder.gif";
            }

            var str_tr = "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr>";

            str_tr += "<td>" + childnull + "</td>";
            str_tr += "<td>";

            var isCommon = false;

            if ((strRealName.toLowerCase() == "drafts") && (layer == 0)) {
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            } else if ((strRealName.toLowerCase() == "inbox") && (layer == 0)) {
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            } else if ((strRealName.toLowerCase() == "trash") && (layer == 0)) {
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            } else if ((strRealName.toLowerCase() == "sent") && (layer == 0)) {
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            } else if ((strRealName.toLowerCase() == "junk") && (layer == 0)) {
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            } else {
                isCommon = true;
                str_tr += "<img src=\"" + strIcon + "\"></td><td>" + strDisplayDirName + "</td></tr></table>";
            }

            setStyle(td0, "TD.gray");
            td0.innerHTML = str_tr;

            var td1 = tr.insertCell(1);
            td1.setAttribute("width", "50");
            td1.setAttribute("align", "center");
            setStyle(td1, "TD.gray");
            get_total_mail_num(strId, td1);

            var td2 = tr.insertCell(2);
            td2.setAttribute("width", "30");
            td2.setAttribute("align", "center");
            td2.setAttribute("dirid", strId);
            setStyle(td2, "TD.gray");
            td2.innerHTML = "<img src=\"append.gif\" alt=\"" + LANG_RESOURCE['NEW_FOLDER'] + "\">";
            td2.onclick = function () {
                show_inputlabel_div(this.getAttribute("dirid"));
            }

            td2.onmouseover = function () {
                this.mousepoint = 99;
                this.style.cursor = "pointer";
                this.setAttribute("imagesrc", this.style.backgroundImage);
                this.style.backgroundImage = "url('pathbg.gif')";
            }

            td2.onmouseout = function () {
                this.mousepoint = 99;
                this.style.cursor = "default";

                this.style.backgroundImage = this.getAttribute("imagesrc");
            }

            var td3 = tr.insertCell(3);
            td3.setAttribute("width", "30");
            td3.setAttribute("align", "center");
            td3.setAttribute("dirid", strId);
            setStyle(td3, "TD.gray");
            if (isCommon) {
                td3.innerHTML = "<img src=\"delete.gif\" alt=\"" + LANG_RESOURCE['REMOVE_FOLDER'] + "\">";

                td3.onclick = function () {
                    if (confirm(LANG_RESOURCE['REMOVE_THIS_FOLDER_WARNING']) == true)
                        delete_label(this.getAttribute("dirid"), strDisplayDirName, this);
                }

                td3.onmouseover = function () {
                    this.mousepoint = 99;
                    this.style.cursor = "pointer";

                    this.setAttribute("imagesrc", this.style.backgroundImage);
                    this.style.backgroundImage = "url('pathbg.gif')";
                }

                td3.onmouseout = function () {
                    this.mousepoint = 99;
                    this.style.cursor = "default";

                    this.style.backgroundImage = this.getAttribute("imagesrc");
                }
            } else {
                td3.innerHTML = "<img src=\"childnull.gif\">";
            }

            if (nodeObj.item(i).childNodes.length > 0) {
                if (i == nodeObj.length - 1)
                    output_dir(tblobj, strId, path + "/" + strDisplayDirName, nodeObj.item(i).childNodes, layer);
                else
                    output_dir(tblobj, strId, path + "/" + strDisplayDirName, nodeObj.item(i).childNodes, layer);
            }
            a++;
        }

    }
    layer--;
    return true;
}

function travel_dirs(pid, gpath, layer) {
    var api_url = "/api/traversaldirs.xml?PID=" + pid + "GPATH=" + encodeURIComponent(gpath);
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var dirList = responseNode.childNodes;

                    output_dir($id('DIRTBL'), pid, "", dirList, layer);
                }
            }
        }
    });
}

function create_label(dirname, dirid) {
    if (dirname == ''){
        return;
    }
    var api_data = "NEW_LABEL=" + encodeURIComponent(dirname) + "&DIRID=" + encodeURIComponent(dirid);
    var api_url = "/api/createlabel.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table(window.parent.leftmenuframe.$id('DIRTBL'));
                    window.parent.leftmenuframe.load_dirs(-1, '', 'common', -1, 0);

                    clear_table($id('DIRTBL'));
                    travel_dirs(-1, '', -1);

                } else {
                    alert(LANG_RESOURCE['CREATE'] + " '" + dirname + "' " + LANG_RESOURCE['FAILED']);
                }
            }
        }
    });
}

function init() {
    window.parent.$id('MAILBAR').style.display = "none";
    window.parent.$id('DIRBAR').style.display = "block";
    window.parent.$id('CALBAR').style.display = "none";
    window.parent.$id('READCALBAR').style.display = "none";
    window.parent.$id('NULLBAR').style.display = "none";
}

function uninit() { }

function login_username() {
    var api_url = "/api/currentusername.xml";
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var LoginUsername = responseNode.childNodes;
                    for (var i = 0; i < LoginUsername.length; i++) {
                        if (LoginUsername.item(i).tagName == "loginusername") {
                            strTmp = LoginUsername.item(i).childNodes[0] == null ? "" : LoginUsername.item(i).childNodes[0].nodeValue;
                        }
                    }

                    $id('LOGIN_USERNAME').innerHTML = "<font color=\"#FFFFFF\"><b>[" + strTmp + "]</b>" + LANG_RESOURCE['WHOSE_FOLDER'] + "</font>";
                }
            }
        }
    });
}

function create_root_dir() {
    show_inputlabel_div("-1");
}

$(document).ready(function () {
    init();
    login_username();
    travel_dirs(-1, '', -1);
});

$(window).on('unload', function () {
    uninit();
})