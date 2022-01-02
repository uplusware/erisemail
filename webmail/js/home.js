function logout() {
    var qUrl = "/api/logout.xml";
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.location = "/";
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function create_label(dirname, dirid) {
    if (dirname == '')
        return;

    var qUrl = "/api/createlabel.xml";
    var strPostData = "NEW_LABEL=" + encodeURIComponent(dirname) + "&DIRID=" + encodeURIComponent(dirid);
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table(window.parent.leftmenuframe.$id('DIRTBL'));
                    window.parent.leftmenuframe.load_dirs(-1, '', 'common', -1, 0);
                } else {
                    alert(LANG_RESOURCE['CREATE'] + " '" + dirname + "' " + LANG_RESOURCE['FAILED']);
                }
            }
        }
    }
    xmlHttp.open("POST", qUrl, true);
    xmlHttp.send(strPostData);
}

function login_username() {
    var qUrl = "/api/currentusername.xml";
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
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

                    $id('LOGIN_USERNAME').innerHTML = "<b>[" + strTmp + "]</b>" + LANG_RESOURCE['WHOSE_FOLDER'];
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function init() {
    show_frame1_view("listmails.html");

    login_username();
}

function refresh_dir() {
    clear_table(window.parent.leftmenuframe.$id('DIRTBL'));
    window.parent.leftmenuframe.load_dirs(-1, '', 'common', -1, 0);
}

function delmail() {
    if (confirm(LANG_RESOURCE['REMOVE_FOLDER_WARNING']) == true) {
        window.parent.childframe1.delmail(false);
    }
}

function remail() {
    window.parent.childframe1.remail();
}

function remail_all() {
    window.parent.childframe1.remail_all();
}

function sel_all(sel) {
    window.parent.childframe1.sel_all(sel);
}

function flag_mail(flag) {
    window.parent.childframe1.flag_mail(flag);
}

function forward_mail() {
    window.parent.childframe1.forward_mail();
}

function seen_mail(flag) {
    window.parent.childframe1.seen_mail(flag);
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
    window.parent.childframe1.copy_mail(strDirs);

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
    window.parent.childframe1.move_mail(strDirs);
    return 0;
}

function ok_create_label() {
    var strDirs = "";
    var c = 0;
    for (var x = 0; x < document.getElementsByName('seldir3').length; x++) {
        if (document.getElementsByName('seldir3')[x].checked == true) {
            strDirs = document.getElementsByName('seldir3')[x].value;
            c++;
        }
    }
    if (c <= 1) {
        need_reload($id('DIRS_DIV3'));
        create_label($id('NEW_LABEL').value, strDirs);
        $id('NEW_LABEL').value = "";
    } else {
        alert(LANG_RESOURCE['NOSELECTED_FOLDER_WARNING']);

        return -1;
    }

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
    $id('DIRS_DIV3').style.display = "none";
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
    $id('DIRS_DIV3').style.display = "none";

    var Pos = GetObjPos($id('MOVEMAIL'))
    show_dirs($id('DIRS_DIV2'), $id('DIRTBL2'), Pos.x, Pos.y + Pos.h + 2, "seldir2");
}

function newmail() {
    show_popup_view("writemail.html");
}

function show_add_dir_div() {
    $("#DIRS_DIV3").dialog({
        height: 500,
        width: 300,
        modal: false,
        title: LANG_RESOURCE['FOLDER'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                if (ok_create_label() != -1)
                    $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('DIRS_DIV1').style.display = "none";
    $id('DIRS_DIV2').style.display = "none";
    var Pos = GetObjPos($id('NEW_LABEL'))
    show_dirs($id('DIRS_DIV3'), $id('DIRTBL3'), Pos.x, Pos.y + Pos.h + 2, "seldir3");
}

function show_frame1_view(url) {
    window.childframe1.location.href = url;
}

function show_popup_view(url) {
    window.open(url);

}

$(document).ready(function () {
    $('#NEWMAIL').click(function () {
        newmail();
    });

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

    $('#NEWDIR').click(function () {
        window.parent.childframe1.create_root_dir();
    });

    $('#REFRESH_DIR').click(function () {
        refresh_dir();
    });

});