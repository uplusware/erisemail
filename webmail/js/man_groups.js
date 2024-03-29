function add_user_to_group(obj, groupname) {
    $("#ADDRS").dialog({
        height: 500,
        width: 500,
        modal: false,
        title: LANG_RESOURCE['APPEND_ID_TO_GROUP_DESC'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                do_add_user_to_group();
                $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    $id('ADDRS').setAttribute("groupname", groupname);

    load_members();
}

function do_del_user_from_group(groupname, userlist) {
    var api_data = "USER_LIST=" + encodeURIComponent(userlist) + "&GROUP_NAME=" + encodeURIComponent(groupname);
    var api_url = "/api/deluserfromgroup.xml";
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
                    $id("GROUP_MEMBER_LIST_TR_" + groupname).setAttribute("loaded", "false");
                    while ($id("GROUP_MEMBER_LIST_TR_" + groupname).cells[0].firstChild != null) {
                        $id("GROUP_MEMBER_LIST_TR_" + groupname).cells[0].firstChild.parentNode.removeChild($id("GROUP_MEMBER_LIST_TR_" + groupname).cells[0].firstChild);
                    }
                    list_members_in_group($id("GROUP_MEMBER_LIST_TR_" + groupname), groupname);

                } else {
                    alert(LANG_RESOURCE['DELETE_ID_FROM_MAILGROUP_ERROR']);
                }
            }
        }
    });
}

function do_add_user_to_group() {
    var strUsers = "";
    for (var x = 0; x < document.getElementsByName('seluser').length; x++) {
        if (document.getElementsByName('seluser')[x].checked == true) {
            strUsers += document.getElementsByName('seluser')[x].value + ";";
        }
    }

    var api_data = "USER_LIST=" + encodeURIComponent(strUsers) + "&GROUP_NAME=" + encodeURIComponent($id('ADDRS').getAttribute("groupname"));
    var api_url = "/api/addusertogroup.xml";
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
                    $id("GROUP_MEMBER_LIST_TR_" + $id('ADDRS').getAttribute("groupname")).setAttribute("loaded", "false");
                    while ($id("GROUP_MEMBER_LIST_TR_" + $id('ADDRS').getAttribute("groupname")).cells[0].firstChild != null) {
                        $id("GROUP_MEMBER_LIST_TR_" + $id('ADDRS').getAttribute("groupname")).cells[0].firstChild.parentNode.removeChild($id("GROUP_MEMBER_LIST_TR_" + $id('ADDRS').getAttribute("groupname")).cells[0].firstChild);
                    }
                    list_members_in_group($id("GROUP_MEMBER_LIST_TR_" + $id('ADDRS').getAttribute("groupname")), $id('ADDRS').getAttribute("groupname"));

                } else {
                    alert(LANG_RESOURCE['APPEND_ID_TO_MAILGROUP_ERROR']);
                }
            }
        }
    });
}

function list_members_in_group(obj, groupname) {
    var api_url = "/api/listmembersofgroup.xml?GROUP_NAME=" + groupname;
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();

            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    var strTmp;
                    var userList = responseNode.childNodes;

                    var newtbl = document.createElement("table");

                    newtbl.setAttribute("id", "GROUP_" + groupname);
                    newtbl.setAttribute("width", "100%");
                    newtbl.setAttribute("height", "100%");
                    newtbl.setAttribute("border", "0");
                    newtbl.setAttribute("cellPadding", "2");
                    newtbl.setAttribute("cellSpacing", "0");
                    newtbl.setAttribute("borderColorDark", "#FFFFFF");
                    newtbl.setAttribute("borderColorLight", "#C0C0C0");

                    setStyle(newtbl, "TABLE.gray");

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

                            var newtr = newtbl.insertRow(newtbl.rows.length);

                            var td0 = newtr.insertCell(0);
                            td0.style.verticalAlign = "middle";
                            td0.style.textAlign = "center";
                            td0.style.height = "25px";
                            td0.style.width = "22px";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<img src=\"" + image + "\" />";

                            var td1 = newtr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "center";
                            td1.style.height = "25px";
                            td1.style.width = "200px";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = userList.item(i).getAttribute("name");

                            var td2 = newtr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "center";
                            td2.style.height = "25px";
                            td2.style.width = "580px";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = userList.item(i).getAttribute("alias");

                            var td3 = newtr.insertCell(3);
                            td3.style.verticalAlign = "middle";
                            td3.style.textAlign = "center";
                            td3.style.height = "25px";
                            td3.style.width = "22px";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = "<img src=\"delete.gif\">";
                            td3.setAttribute("name", userList.item(i).getAttribute("name"));
                            td3.onclick = function () {
                                if (confirm(LANG_RESOURCE['DELETE_ID_FROM_MAILGROUP_WARNING']) == true)
                                    do_del_user_from_group(groupname, this.getAttribute("name"));
                            }

                            td3.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td3.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }
                        }
                    }

                    obj.cells[0].appendChild(newtbl);
                }
            }
        }
    });
}

function load_groups() {
    var api_url = "/api/listgroups.xml";
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
                    var userList = responseNode.childNodes;

                    for (var i = 0; i < userList.length; i++) {
                        if (userList.item(i).tagName == "user") {
                            var tr1 = $id('GROUPTBL').insertRow($id('GROUPTBL').rows.length);
                            tr1.setAttribute("height", "22");
                            var td10 = tr1.insertCell(0);
                            setStyle(td10, "TD.blue");

                            var newtbl = document.createElement("table");
                            newtbl.setAttribute("width", "100%");
                            newtbl.setAttribute("height", "100%");
                            newtbl.setAttribute("border", "0");
                            newtbl.setAttribute("cellPadding", "0");
                            newtbl.setAttribute("cellSpacing", "0");
                            newtbl.setAttribute("borderColorDark", "#FFFFFF");
                            newtbl.setAttribute("borderColorLight", "#C0C0C0");

                            var newtr = newtbl.insertRow(0);
                            var newtd0 = newtr.insertCell(0);
                            var newtd1 = newtr.insertCell(1);
                            var newtd2 = newtr.insertCell(2);
                            var newtd3 = newtr.insertCell(3);

                            newtd0.style.verticalAlign = "middle";
                            newtd0.style.textAlign = "center";
                            newtd0.style.height = "25px";
                            newtd0.style.width = "22px";
                            newtd0.innerHTML = "<img src=\"group.gif\" />";

                            newtd1.style.verticalAlign = "middle";
                            newtd1.style.textAlign = "left";
                            newtd1.setAttribute("groupname", userList.item(i).getAttribute("name"));
                            newtd1.innerHTML = "<font color=\"#FFFFFF\">" + userList.item(i).getAttribute("name") + " - " + userList.item(i).getAttribute("alias") + "</font>";

                            newtd1.onclick = function () {
                                if (this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.getAttribute("loaded") == "false") {
                                    while (this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.cells[0].firstChild != null) {
                                        this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.cells[0].firstChild.parentNode.removeChild(this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.cells[0].firstChild);
                                    }
                                    this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.setAttribute("loaded", "true");
                                    list_members_in_group(this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling, this.getAttribute("groupname"));
                                }

                                if (this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.style.display == "none") {
                                    this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.style.display = "block";
                                } else {
                                    this.parentNode.parentNode.parentNode.parentNode.parentNode.nextSibling.style.display = "none";
                                }
                            }

                            newtd1.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            newtd1.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            newtd2.style.verticalAlign = "middle";
                            newtd2.style.textAlign = "center";
                            newtd2.style.width = "100px";
                            newtd2.setAttribute("groupname", userList.item(i).getAttribute("name"));
                            newtd2.innerHTML = "<table><tr><td><img src=\"member.gif\"></td><td><font color='#FFFFFF'>" + LANG_RESOURCE['APPEND_ID_TO_GROUP_DESC'] + "</font></td></tr></table>";

                            newtd2.onclick = function () {
                                add_user_to_group(this.parentNode, this.getAttribute("groupname"));
                            }

                            newtd2.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            newtd2.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            td10.appendChild(newtbl);

                            var tr2 = $id('GROUPTBL').insertRow($id('GROUPTBL').rows.length);
                            tr2.setAttribute("id", "GROUP_MEMBER_LIST_TR_" + userList.item(i).getAttribute("name"));
                            tr2.style.display = "none";

                            tr2.setAttribute("loaded", "false");
                            var td20 = tr2.insertCell(0);
                            setStyle(td20, "TD.blue");
                            td20.style.backgroundColor = "#FFFFFF";
                        }
                    }
                    $id("STATUS").innerHTML = "";
                    $id("STATUS").style.display = "none";
                }
            }
        }
    });
}

function load_members() {
    var api_url = "/api/listmembers.xml";
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0") {
                    var strTmp;
                    var userList = responseNode.childNodes;

                    clear_table($id('USERTBL'));

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
                            td0.style.verticalAlign = "middle";
                            td0.style.textAlign = "center";
                            td0.style.height = "25px";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<input type=\"checkbox\" name=\"seluser\" id=\"seluser\" value=\"" + userList.item(i).getAttribute("name") + "\">";

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "center";
                            td1.style.height = "25px";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = "<img src=\"" + image + "\" />";

                            var td2 = tr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "center";
                            td2.style.height = "25px";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = userList.item(i).getAttribute("name");

                            var td3 = tr.insertCell(3);
                            td3.style.verticalAlign = "middle";
                            td3.style.textAlign = "center";
                            td3.style.height = "25px";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = userList.item(i).getAttribute("alias");
                        }
                    }

                    $id('ADDRS').style.height = $id('USERTBL_SIZE').offsetHeight;
                }
            }
        }
    });
}

function sel_alluser(sel) {
    for (var x = 0; x < document.getElementsByName('seluser').length; x++) {
        document.getElementsByName('seluser')[x].checked = sel;
    }
}

function init() {
    window.parent.change_tab("group");
    load_groups();
}

$(document).ready(function () {
    init();
});

$(window).on('unload', function () {

})