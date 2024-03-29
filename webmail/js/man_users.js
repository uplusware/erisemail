function load_levels_to_option(selobj) {
    var api_url = "/api/listlevels.xml";
    $.ajax({
        url: api_url,
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
                            var varItem = new Option(levelList.item(i).getAttribute("name"), levelList.item(i).getAttribute("lid"));
                            if (levelList.item(i).getAttribute("default") == "true") {
                                $id('NEW_USER_DIV').setAttribute("defaultlevel", levelList.item(i).getAttribute("lid"));
                                varItem.selected = true;
                            }
                            selobj.options.add(varItem);
                        }
                    }

                    $id("STATUS").innerHTML = "";
                    $id("STATUS").style.display = "none";
                }
            }
        }
    });
}

function load_clusters_to_option(selobj) {
    var api_url = "/api/listcluster.xml";
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var clusterList = responseNode.childNodes;

                    for (var i = 0; i < clusterList.length; i++) {
                        if (clusterList.item(i).tagName == "cluster") {
                            var varItem = new Option(clusterList.item(i).getAttribute("host"), clusterList.item(i).getAttribute("host"));
                            selobj.options.add(varItem);
                        }
                    }

                    $id("STATUS").innerHTML = "";
                    $id("STATUS").style.display = "none";
                }
            }
        }
    });
}

function show_edituser_div(username, obj) {
    $("#EDIT_USER_DIV").dialog({
        height: 380,
        width: 360,
        modal: false,
        title: LANG_RESOURCE['EDIT_ID_DESC'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                do_edit_user();
                $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    if ($id('EDIT_USER_DIV').getAttribute("loaded") != "true") {
        load_levels_to_option($id('EDIT_LEVEL'));
        load_clusters_to_option($id('EDIT_HOST'));
        $id('EDIT_USER_DIV').setAttribute("loaded", "true")
    }

    $id('EDIT_USERNAME').value = obj.getAttribute("username");
    $id('EDIT_ALIAS').value = obj.getAttribute("alias");

    for (var i = 0; i < $id('EDIT_LEVEL').options.length; i++) {
        if ($id('EDIT_LEVEL').options[i].value == obj.getAttribute("level")) {
            $id('EDIT_LEVEL').options[i].selected = true;
            break;
        }

    }

    for (var i = 0; i < $id('EDIT_HOST').options.length; i++) {
        if ($id('EDIT_HOST').options[i].value == obj.getAttribute("host")) {
            $id('EDIT_HOST').options[i].selected = true;
            break;
        }

    }

    if (obj.getAttribute("type") == "Group" || obj.getAttribute("type") == "group") {
        $id('EDIT_LEVEL').disabled = true;
        $id('EDIT_HOST').disabled = true;
    } else {
        $id('EDIT_LEVEL').disabled = false;
        $id('EDIT_HOST').disabled = false;
    }

    $id('EDIT_USER_STATUS').checked = (obj.getAttribute("status") == "Active") ? false : true;

    $id('EDIT_USER_DIV').setAttribute("username", username);
}

function show_newuser_div() {
    $("#NEW_USER_DIV").dialog({
        height: 380,
        width: 360,
        modal: false,
        title: LANG_RESOURCE['APPEND_ID_DESC'],
        buttons: {
            [LANG_RESOURCE['OK']]: function () {
                do_add_user();
                $(this).dialog("close");
            },
            [LANG_RESOURCE['CANCEL']]: function () {
                $(this).dialog("close");
            }
        }
    });

    document.getElementsByName('NEW_TYPE')[0].checked = true;

    $id('NEW_USERNAME').value = "";

    $id('NEW_USERPWD').disabled = false;
    $id('NEW_USERPWD2').disabled = false;

    $id('NEW_LEVEL').disabled = false;

    $id('NEW_USERPWD').value = "";
    $id('NEW_USERPWD2').value = "";

    $id('NEW_ALIAS').value = "";

    if ($id('NEW_USER_DIV').getAttribute("loaded") != "true") {
        load_levels_to_option($id('NEW_LEVEL'));
        load_clusters_to_option($id('NEW_HOST'));
        $id('NEW_USER_DIV').setAttribute("loaded", "true")
    }

    $id('NEW_LEVEL').value = $id('NEW_USER_DIV').getAttribute("defaultlevel");
}

function do_delete_user(uname) {
    var api_url = "/api/deluser.xml?USERNAME=" + encodeURIComponent(uname);
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table_without_header($id('USERTBL'));
                    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));

                } else {
                    alert(LANG_RESOURCE['REMOVE_ID_ERROR']);
                }
            }
        }
    });
}

function load_users(orderby, desc) {
    var api_url = "/api/listusers.xml?ORDER_BY=" + orderby + "&DESC=" + (desc == null ? '' : desc);
    $.ajax({
        url: api_url,
        beforeSend: function () {
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
                            if (userList.item(i).getAttribute("type") == "Group") {
                                tr.style.backgroundColor = "#FFFFEE";
                            }
                            tr.onmouseover = function () {
                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('focusbg.gif')";
                            }
                            tr.onmouseout = function () {
                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            tr.setAttribute("username", userList.item(i).getAttribute("name"));
                            tr.setAttribute("alias", userList.item(i).getAttribute("alias"));
                            tr.setAttribute("level", userList.item(i).getAttribute("level"));
                            tr.setAttribute("status", userList.item(i).getAttribute("status"));
                            tr.setAttribute("type", userList.item(i).getAttribute("type"));

                            var td0 = tr.insertCell(0);
                            td0.style.verticalAlign = "middle";
                            td0.style.textAlign = "center";
                            td0.style.height = "25px";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<img src=\"" + image + "\" />";

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "left";
                            td1.style.height = "25px";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = userList.item(i).getAttribute("name");

                            var td2 = tr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "left";
                            td2.style.width = "398px";
                            td2.style.height = "25px";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = userList.item(i).getAttribute("alias");

                            var td3 = tr.insertCell(3);
                            td3.style.verticalAlign = "middle";
                            td3.style.textAlign = "left";
                            td3.style.height = "25px";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = userList.item(i).getAttribute("host");

                            var td4 = tr.insertCell(4);
                            td4.style.verticalAlign = "middle";
                            td4.style.textAlign = "left";
                            td4.style.height = "25px";
                            setStyle(td4, "TD.gray");
                            td4.innerHTML = userList.item(i).getAttribute("lname");

                            var td5 = tr.insertCell(5);
                            td5.style.verticalAlign = "middle";
                            td5.style.textAlign = "center";
                            td5.style.width = "50px";
                            td5.style.height = "25px";
                            setStyle(td5, "TD.gray");
                            td5.innerHTML = userList.item(i).getAttribute("type") == "Group" ? LANG_RESOURCE['MAIL_GROUP_DESC'] : LANG_RESOURCE['USER_DESC'];

                            var td6 = tr.insertCell(6);
                            td6.style.verticalAlign = "middle";
                            td6.style.textAlign = "center";
                            td6.style.height = "25px";
                            setStyle(td6, "TD.gray");
                            td6.innerHTML = userList.item(i).getAttribute("role") == "Administrator" ? LANG_RESOURCE['ADMINISTRATOR_DESC'] : LANG_RESOURCE['USER_DESC'];

                            var td7 = tr.insertCell(7);
                            td7.style.verticalAlign = "middle";
                            td7.style.textAlign = "center";
                            td7.style.height = "25px";
                            setStyle(td7, "TD.gray");
                            td7.innerHTML = userList.item(i).getAttribute("status") == "Active" ? LANG_RESOURCE['ACTIVE_ID_DESC'] : LANG_RESOURCE['DISABLED_ID_DESC'];

                            var td8 = tr.insertCell(8);
                            td8.style.verticalAlign = "middle";
                            td8.style.textAlign = "center";
                            td8.style.height = "25px";

                            td8.setAttribute("username", userList.item(i).getAttribute("name"));
                            td8.innerHTML = "<img src=\"edit.gif\">";

                            td8.onclick = function () {
                                show_edituser_div(this.getAttribute("username"), this.parentNode);
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

                            setStyle(td8, "TD.gray");

                            var td9 = tr.insertCell(9);
                            td9.style.verticalAlign = "middle";
                            td9.style.textAlign = "center";
                            td9.style.height = "25px";
                            setStyle(td9, "TD.gray");
                            td9.setAttribute("username", userList.item(i).getAttribute("name"));
                            td9.innerHTML = "<img src=\"delete.gif\">";

                            td9.onclick = function () {
                                if (confirm(LANG_RESOURCE['REMOVE_ID_WARNING']) == true)
                                    do_delete_user(this.parentNode.getAttribute('username'));
                            }

                            td9.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td9.onmouseout = function () {
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
    window.parent.change_tab("user");
    $id('USERTBL').setAttribute('orderby', 'utime');
    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));
}

function do_add_user() {
    if ($id('NEW_USERPWD').value != $id('NEW_USERPWD2').value) {
        alert(LANG_RESOURCE['VERIFY_PASSWORD_ERROR']);
        return;
    }

    var strType;
    if (document.getElementsByName('NEW_TYPE')[0].checked) {
        strType = "member";
    }
    else if (document.getElementsByName('NEW_TYPE')[1].checked) {
        strType = "group";
    }

    var api_data = "NEW_USERNAME=" + encodeURIComponent($id('NEW_USERNAME').value);
    api_data += "&NEW_USERPWD=" + encodeURIComponent($id('NEW_USERPWD').value);
    api_data += "&NEW_ALIAS=" + encodeURIComponent($id('NEW_ALIAS').value);
    api_data += "&NEW_TYPE=" + encodeURIComponent(strType);
    api_data += "&NEW_LEVEL=" + encodeURIComponent($id('NEW_LEVEL').value);
    api_data += "&NEW_HOST=" + encodeURIComponent($id('NEW_HOST').value);
    var api_url = "/api/adduser.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        contentType: "text/html;application/x-www-form-urlencoded",
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table_without_header($id('USERTBL'));
                    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));

                } else {
                    alert(LANG_RESOURCE['APPEND_ID_ERROR']);
                }
            }
        }
    });

    $id('NEW_USERNAME').value = "";
    $id('NEW_USERPWD').value = "";
    $id('NEW_USERPWD2').value = "";
    $id('NEW_ALIAS').value = "";
}

function do_edit_user() {
    var api_data = "EDIT_USERNAME=" + encodeURIComponent($id('EDIT_USERNAME').value);
    api_data += "&EDIT_ALIAS=" + encodeURIComponent($id('EDIT_ALIAS').value);
    api_data += "&EDIT_LEVEL=" + encodeURIComponent($id('EDIT_LEVEL').value);
    api_data += "&EDIT_HOST=" + encodeURIComponent($id('EDIT_HOST').value);
    api_data += "&EDIT_USER_STATUS=" + encodeURIComponent($id('EDIT_USER_STATUS').checked == true ? "Disable" : "Active");
    var api_url = "/api/edituser.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        contentType: "text/html;application/x-www-form-urlencoded",
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    clear_table_without_header($id('USERTBL'));
                    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));

                } else {
                    alert(LANG_RESOURCE['EDIT_ID_ERROR']);
                }
            }
        }
    });
}

function sort_users(orderby) {
    $id('USERTBL').setAttribute('orderby', orderby);

    clear_table_without_header($id('USERTBL'));

    if ($id('USERTBL').getAttribute(orderby) == 'true')
        $id('USERTBL').setAttribute(orderby, 'false');
    else
        $id('USERTBL').setAttribute(orderby, 'true');

    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')))
}

function switch_id_type() {

    if (document.getElementsByName('NEW_TYPE')[0].checked) {
        $id('NEW_USERPWD').disabled = false;
        $id('NEW_USERPWD2').disabled = false;

        $id('NEW_LEVEL').disabled = false;
        $id('NEW_HOST').disabled = false;

        $id('NEW_USERPWD').value = "";
        $id('NEW_USERPWD2').value = "";

    } else if (document.getElementsByName('NEW_TYPE')[1].checked) {
        $id('NEW_USERPWD').disabled = true;
        $id('NEW_USERPWD2').disabled = true;

        $id('NEW_LEVEL').disabled = true;
        $id('NEW_HOST').disabled = true;

        $id('NEW_USERPWD').value = randpwd();
        $id('NEW_USERPWD2').value = $id('NEW_USERPWD').value;

        $id('NEW_LEVEL').value = $id('NEW_USER_DIV').getAttribute("defaultlevel");
    }
}

$(document).ready(function () {
    init();

    $('#uname_col').click(function () {
        sort_users('uname');
    });

    $('#ualias_col').click(function () {
        sort_users('ualias');
    });

    $('#uhost_col').click(function () {
        sort_users('uhost');
    });

    $('#ulevel_col').click(function () {
        sort_users('ulevel');
    });

    $('#utype_col').click(function () {
        sort_users('utype');
    });

    $('#urole_col').click(function () {
        sort_users('urole');
    });

    $('#ustatus_col').click(function () {
        sort_users('ustatus');
    });

    $('#uname_col, #ualias_col, #uhost_col, #ulevel_col, #utype_col, #urole_col, #ustatus_col').mouseover(function () {
        this.mousepoint = 99;
        this.style.cursor = 'pointer';
    });

    $('#uname_col, #ualias_col, #uhost_col, #ulevel_col, #utype_col, #urole_col, #ustatus_col').mouseout(function () {
        this.mousepoint = 99;
        this.style.cursor = 'default';
    });

    $('#append_id').click(function () {
        show_newuser_div();
    });

    $('[name="NEW_TYPE"]').click(function () {
        switch_id_type();
    });
});

$(window).on('unload', function () {

})