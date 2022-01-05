function ontimer(trid) {
    if ($id(trid)) {
        if ($id(trid).getAttribute("isOver") == "true") {
            $id('LEVEL_DESCRIPTION').innerHTML = "<table border=\"0\" height=\"100%\" width=\"100%\" cellpadding=\"2\" cellspacing=\"2\" bgcolor=\"#FFFFEE\"><tr><td align=\"left\" valign=\"middle\">" + $id(trid).getAttribute("description") + "</td></tr></table>";

            var pos = GetObjPos($id(trid));

            $id('LEVEL_DESCRIPTION').style.left = pos.x - 1;
            $id('LEVEL_DESCRIPTION').style.top = pos.y + pos.h + 1;

            $id('LEVEL_DESCRIPTION').style.height = 25;
            $id('LEVEL_DESCRIPTION').style.width = $id('LEVELTBL').offsetWidth;

            $id('LEVEL_DESCRIPTION').style.display = "block";
        }
    }
}

function load_levels() {
    var api_url = "/api/listlevels.xml";
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id("STATUS").innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var levelList = responseNode.childNodes;

                    for (var i = 0; i < levelList.length; i++) {
                        if (levelList.item(i).tagName == "level") {
                            tr = $id('LEVELTBL').insertRow($id('LEVELTBL').rows.length);
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

                            tr.onmouseover = function () {
                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('focusbg.gif')";
                                this.setAttribute("isOver", "true");
                                setTimeout("ontimer(\"" + this.getAttribute("id") + "\")", 1200);

                            }
                            tr.onmouseout = function () {
                                this.style.backgroundImage = this.getAttribute("imagesrc");

                                $id('LEVEL_DESCRIPTION').style.display = "none";

                                this.setAttribute("isOver", "false");
                            }

                            var image1 = "<img src=\"pading.gif\" />";

                            if (levelList.item(i).getAttribute("default") == "true") {
                                image1 = "<img src=\"default.gif\" alt=\"" + LANG_RESOURCE['DEFAULT_POLICY_DESC'] + "\" />";
                            }

                            var image2 = "<img src=\"no.gif\" alt=\"" + LANG_RESOURCE['NO_NEED_REVIEW_LEVEL'] + "\" />";

                            if (levelList.item(i).getAttribute("enableaudit") == "true") {
                                image2 = "<img src=\"yes.gif\" alt=\"" + LANG_RESOURCE['NEED_REVIEW_LEVEL'] + "\" />";
                            }

                            var td0 = tr.insertCell(0);
                            td0.valign = "middle";
                            td0.align = "center";
                            td0.height = "25";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = image1;

                            td0.setAttribute("lid", levelList.item(i).getAttribute("lid"));

                            td0.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td0.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            td0.onclick = function () {
                                set_level_deafult(this.parentNode);
                            }

                            var td1 = tr.insertCell(1);
                            td1.valign = "middle";
                            td1.align = "left";
                            td1.height = "25";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = levelList.item(i).getAttribute("name");

                            var td2 = tr.insertCell(2);
                            td2.valign = "middle";
                            td2.align = "left";
                            td2.width = "100";
                            td2.height = "25";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = Math.round(levelList.item(i).getAttribute("mailmaxsize") / 1024 * 100) / 100 + "KB";

                            var td3 = tr.insertCell(3);
                            td3.valign = "middle";
                            td3.align = "center";
                            td3.width = "100";
                            td3.height = "25";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = Math.round(levelList.item(i).getAttribute("boxmaxsize") / 1024) + "KB";

                            var td4 = tr.insertCell(4);
                            td4.valign = "middle";
                            td4.align = "center";
                            td4.height = "25";
                            td3.width = "100";
                            setStyle(td4, "TD.gray");
                            td4.innerHTML = image2;

                            var td5 = tr.insertCell(5);
                            td5.valign = "middle";
                            td5.align = "center";
                            td5.height = "25";
                            setStyle(td5, "TD.gray");
                            td5.innerHTML = Math.round(levelList.item(i).getAttribute("mailsizethreshold") / 1024 * 100) / 100 + "KB";

                            var td6 = tr.insertCell(6);
                            td6.valign = "middle";
                            td6.align = "center";
                            td6.height = "25";
                            setStyle(td6, "TD.gray");
                            td6.innerHTML = Math.round(levelList.item(i).getAttribute("attachsizethreshold") / 1024 * 100) / 100 + "KB";

                            var td7 = tr.insertCell(7);
                            td7.valign = "middle";
                            td7.align = "center";
                            td7.height = "25";
                            setStyle(td7, "TD.gray");
                            td7.innerHTML = "<img src=\"edit.gif\">";

                            td7.onclick = function () {
                                $id('LEVEL_DESCRIPTION').style.display = "none";
                                show_editlevel_div(this.parentNode);
                            }

                            td7.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td7.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            var td8 = tr.insertCell(8);
                            td8.valign = "middle";
                            td8.align = "center";
                            td8.height = "25";
                            setStyle(td8, "TD.gray");
                            td8.innerHTML = "<img src=\"delete.gif\">";

                            td8.onclick = function () {
                                if (this.parentNode.getAttribute("default") == "true")
                                    alert(LANG_RESOURCE['DELETE_DEFAULT_LEVEL_ERROR']);
                                else {
                                    if (confirm(LANG_RESOURCE['DELETE_LEVEL_WARNING']) == true)
                                        do_delete_level(this.parentNode.getAttribute("lid"));
                                }
                            }

                            td8.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td8.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                        }
                    }
                    $id("STATUS").innerHTML = "";
                    $id("STATUS").style.display = "none";
                }
            }
        }
    });
}

function init() {
    window.parent.change_tab("level");
    load_levels();
}

function set_level_deafult(obj) {
    var api_data = "LEVELID=" + encodeURIComponent(obj.getAttribute("lid"));
    var api_url = "/api/defaultlevel.xml";
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
                    clear_table_without_header($id('LEVELTBL'));
                    load_levels();
                } else {
                    alert(LANG_RESOURCE['SET_DEFAULT_LEVEL_FAILED']);
                }
            }
        }
    });
}

function show_newlevel_div() {
    $("#NEW_LEVEL_DIV").dialog({
        height: 500,
        width: 500,
        modal: false,
        title: LANG_RESOURCE['APPEND_LEVEL_DESC'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                if (do_create_level())
                    $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('NEW_LEVELNAME').value = "";
    $id('NEW_DESCRIPTION').value = "";
    $id('NEW_MAILMAXSIZE').value = "5000";
    $id('NEW_BOXMAXSIZE').value = "500000";
    $id('NEW_AUDIT').checked = false;
    $id('NEW_MAILSIZETHRESHOLD').value = "5000";
    $id('NEW_ATTACHSIZETHRESHOLD').value = "5000";
    $id('NEW_DEFAULT').checked = false;
}

function show_editlevel_div(obj) {
    $("#EDIT_LEVEL_DIV").dialog({
        height: 500,
        width: 500,
        modal: false,
        title: LANG_RESOURCE['EDIT_LEVEL_DESC'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                if (do_edit_level())
                    $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('EDIT_LEVEL_DIV').setAttribute("lid", obj.getAttribute("lid"));

    $id('EDIT_LEVELNAME').value = obj.getAttribute("name");
    $id('EDIT_DESCRIPTION').value = obj.getAttribute("description");
    $id('EDIT_MAILMAXSIZE').value = Math.round(obj.getAttribute("mailmaxsize") / 1024);
    $id('EDIT_BOXMAXSIZE').value = Math.round(obj.getAttribute("boxmaxsize") / 1024);
    $id('EDIT_AUDIT').checked = obj.getAttribute("enableaudit") == "true" ? true : false;
    $id('EDIT_MAILSIZETHRESHOLD').value = Math.round(obj.getAttribute("mailsizethreshold") / 1024);
    $id('EDIT_ATTACHSIZETHRESHOLD').value = Math.round(obj.getAttribute("attachsizethreshold") / 1024);
    $id('EDIT_DEFAULT').checked = obj.getAttribute("default") == "true" ? true : false;
}

function do_create_level() {
    if (!isLegalID($id('NEW_LEVELNAME').value)
        || !isNumber($id('NEW_MAILMAXSIZE').value)
        || !isNumber($id('NEW_BOXMAXSIZE').value)
        || !isNumber($id('NEW_MAILSIZETHRESHOLD').value)
        || !isNumber($id('NEW_ATTACHSIZETHRESHOLD').value)) {
        alert(LANG_RESOURCE['ILLEGAL_LEVEL_NAME_CHAR']);
        return false;
    }

    if (parseInt($id('NEW_MAILMAXSIZE').value) > 1024 * 1024 * 2) {
        alert(LANG_RESOURCE['MAX_MAIL_SIZE_WARNING'] + 1024 * 1024 * 2 + " KB");
        return false;
    }

    if (parseInt($id('NEW_BOXMAXSIZE').value) > 1024 * 1024 * 4) {
        alert(LANG_RESOURCE['MAX_MAILBOX_SIZE_WARNING'] + 1024 * 1024 * 4 + " KB");
        return false;
    }

    var api_data = "NEW_LEVELNAME=" + encodeURIComponent($id('NEW_LEVELNAME').value);
    api_data += "&NEW_DESCRIPTION=" + encodeURIComponent($id('NEW_DESCRIPTION').value);
    api_data += "&NEW_MAILMAXSIZE=" + encodeURIComponent($id('NEW_MAILMAXSIZE').value);
    api_data += "&NEW_BOXMAXSIZE=" + encodeURIComponent($id('NEW_BOXMAXSIZE').value);
    api_data += "&NEW_AUDIT=" + (($id('NEW_AUDIT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false"));
    api_data += "&NEW_MAILSIZETHRESHOLD=" + encodeURIComponent($id('NEW_MAILSIZETHRESHOLD').value);
    api_data += "&NEW_ATTACHSIZETHRESHOLD=" + encodeURIComponent($id('NEW_ATTACHSIZETHRESHOLD').value);
    api_data += "&NEW_DEFAULT=" + (($id('NEW_DEFAULT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false"));

    var api_url = "/api/createlevel.xml";
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
                    clear_table_without_header($id('LEVELTBL'));
                    load_levels();

                } else {
                    alert(LANG_RESOURCE['CREATE_LEVEL_FAILED']);
                }
            }
        }
    });

    return true;
}

function do_edit_level() {
    if (!isLegalID($id('EDIT_LEVELNAME').value)
        || !isNumber($id('EDIT_MAILMAXSIZE').value)
        || !isNumber($id('EDIT_BOXMAXSIZE').value)
        || !isNumber($id('EDIT_MAILSIZETHRESHOLD').value)
        || !isNumber($id('EDIT_ATTACHSIZETHRESHOLD').value)) {
        alert(LANG_RESOURCE['ILLEGALS_LEVEL_NAME_CHAR']);
        return false;
    }

    if (parseInt($id('EDIT_MAILMAXSIZE').value) > 1024 * 1024 * 2) {
        alert(LANG_RESOURCE['MAX_MAIL_SIZE_WARNING'] + 1024 * 1024 * 2 + " KB");
        return false;
    }

    if (parseInt($id('EDIT_BOXMAXSIZE').value) > 1024 * 1024 * 4) {
        alert(LANG_RESOURCE['MAX_MAILBOX_SIZE_WARNING'] + 1024 * 1024 * 4 + " KB");
        return false;
    }
    var api_data = "EDIT_LEVELID=" + encodeURIComponent($id('EDIT_LEVEL_DIV').getAttribute("lid"));
    api_data += "&EDIT_LEVELNAME=" + encodeURIComponent($id('EDIT_LEVELNAME').value);
    api_data += "&EDIT_DESCRIPTION=" + encodeURIComponent($id('EDIT_DESCRIPTION').value);
    api_data += "&EDIT_MAILMAXSIZE=" + encodeURIComponent($id('EDIT_MAILMAXSIZE').value);
    api_data += "&EDIT_BOXMAXSIZE=" + encodeURIComponent($id('EDIT_BOXMAXSIZE').value);
    api_data += "&EDIT_AUDIT=" + (($id('EDIT_AUDIT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false"));
    api_data += "&EDIT_MAILSIZETHRESHOLD=" + encodeURIComponent($id('EDIT_MAILSIZETHRESHOLD').value);
    api_data += "&EDIT_ATTACHSIZETHRESHOLD=" + encodeURIComponent($id('EDIT_ATTACHSIZETHRESHOLD').value);
    api_data += "&EDIT_DEFAULT=" + (($id('EDIT_DEFAULT').checked == true) ? encodeURIComponent("true") : encodeURIComponent("false"));
    var api_url = "/api/updatelevel.xml";
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
                    clear_table_without_header($id('LEVELTBL'));
                    load_levels();
                } else {
                    alert(LANG_RESOURCE['UPDATE_LEVEL_FAILED']);
                }
            }
        }
    });

    return true;
}

function do_delete_level(levelid) {
    var api_data = "LEVELID=" + encodeURIComponent(levelid);
    var api_url = "/api/deletelevel.xml";
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
                    clear_table_without_header($id('LEVELTBL'));
                    load_levels();
                } else {
                    alert(LANG_RESOURCE['DELETE_LEVEL_FAILED']);
                }
            }
        }
    });

    return true;
}

$(document).ready(function () {
    init();

    $('#append_level').click(function () {
        show_newlevel_div();
    });
});

$(window).on('unload', function () {

})