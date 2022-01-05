function load_users(orderby, desc) {
    var api_url = "/api/listusers.xml?ORDER_BY=" + orderby + "&DESC=" + (desc == null ? '' : desc);
    $.ajax({
        url: api_url,
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

                            tr.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('focusbg.gif')";
                            }
                            tr.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            tr.setAttribute("selflink", "writemail.html?TOADDRS=" + userList.item(i).getAttribute("name") + "@" + userList.item(i).getAttribute("domain"));

                            tr.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            var td0 = tr.insertCell(0);
                            td0.valign = "middle";
                            td0.align = "center";
                            td0.height = "25";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<img src=\"" + image + "\" />";

                            var td1 = tr.insertCell(1);
                            td1.valign = "middle";
                            td1.align = "left";
                            td1.height = "25";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = userList.item(i).getAttribute("name");

                            var td2 = tr.insertCell(2);
                            td2.valign = "middle";
                            td2.align = "left";
                            td2.width = "500";
                            td2.height = "25";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = userList.item(i).getAttribute("alias");

                            var td3 = tr.insertCell(3);
                            td3.valign = "middle";
                            td3.align = "center";
                            td3.width = "50";
                            td3.height = "25";
                            setStyle(td3, "TD.gray");
                            td3.innerHTML = userList.item(i).getAttribute("type") == "Group" ? LANG_RESOURCE['MAIL_GROUP_DESC'] : LANG_RESOURCE['USER_DESC'];

                            var td4 = tr.insertCell(4);
                            td4.valign = "middle";
                            td4.align = "center";
                            td4.height = "25";
                            setStyle(td4, "TD.gray");
                            td4.innerHTML = userList.item(i).getAttribute("role") == "Administrator" ? LANG_RESOURCE['ADMINISTRATOR_DESC'] : LANG_RESOURCE['USER_DESC'];

                            var td5 = tr.insertCell(5);
                            td5.valign = "middle";
                            td5.align = "center";
                            td5.height = "25";
                            setStyle(td5, "TD.gray");
                            td5.innerHTML = userList.item(i).getAttribute("status") == "Active" ? LANG_RESOURCE['ACTIVE_ID_DESC'] : LANG_RESOURCE['DISABLED_ID_DESC'];

                            if (userList.item(i).getAttribute("status") != "Active")
                                tr.disabled = "true";
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
    window.parent.$id('MAILBAR').style.display = "none";
    window.parent.$id('DIRBAR').style.display = "none";
    window.parent.$id('CALBAR').style.display = "none";
    window.parent.$id('READCALBAR').style.display = "none";
    window.parent.$id('NULLBAR').style.display = "block";

}

function uninit() {

}

function show_mail_detail(url) {
    window.open(url);
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

$(document).ready(function () {
    init();
    $id('USERTBL').setAttribute('orderby', 'utime');
    load_users($id('USERTBL').getAttribute('orderby'), $id('USERTBL').getAttribute($id('USERTBL').getAttribute('orderby')));

    $('#uname_col').click(function () {
        sort_users('uname');
    });

    $('#ualias_col').click(function () {
        sort_users('ualias');
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

    $('#uname_col, #ualias_col, #utype_col, #urole_col, #ustatus_col').mouseover(function () {
        this.mousepoint = 99;
        this.style.cursor = 'pointer';
    });

    $('#uname_col, #ualias_col, #utype_col, #urole_col, #ustatus_col').mouseout(function () {
        this.mousepoint = 99;
        this.style.cursor = 'default';
    });
});

$(window).on('unload', function () {
    uninit();
})