function passwd(oldpwd, newpwd, verifypwd) {
    if (newpwd != verifypwd) {
        pwdinfo.innerHTML = LANG_RESOURCE['WRONG_VERIFIED_PASSWORD'];
        return;
    }
    var qUrl = "/api/passwd.xml?OLD_PWD=" + oldpwd + "&NEW_PWD=" + newpwd;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        alert(LANG_RESOURCE['PASSWORD_CHANGED_NEED_RELOGIN']);
                        window.parent.logout();
                    } else {
                        document.getElementById("pwdinfo").innerHTML = LANG_RESOURCE['PASSWORD_CHANGED_FAILED_NEED_RETRY'];
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function alias(stralias) {
    if (stralias == "") {
        aliasinfo.innerHTML = LANG_RESOURCE['EMPTY_ALIAS'];
        return;
    }
    var qUrl = "/api/alias.xml?ALIAS=" + stralias;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        alert(LANG_RESOURCE['CHANGE_ALIAS_OK']);
                    } else {
                        document.getElementById("aliasinfo").innerHTML = LANG_RESOURCE['CHANGE_ALIAS_FAILED'];
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function userinfo() {
    var qUrl = "/api/userinfo.xml";
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
                        var userList = responseNode.childNodes;
                        for (var i = 0; i < userList.length; i++) {
                            if (userList.item(i).tagName == "user") {
                                var strRole;
                                if (userList.item(i).getAttribute("role") == "Administrator")
                                    strRole = LANG_RESOURCE['ADMINISTRATOR_DESC'];
                                else if (userList.item(i).getAttribute("role") == "User")
                                    strRole = LANG_RESOURCE['USER_DESC'];
                                else if (userList.item(i).getAttribute("role") == "Disabled")
                                    strRole = LANG_RESOURCE['DISABLED_ID_DESC'];
                                else
                                    strRole = LANG_RESOURCE['UNKNOWN_ID_DESC'];

                                $id('userrole').innerHTML = strRole;

                                $id('mailsize').innerHTML = Math.round(userList.item(i).getAttribute("mailsize") / 1024);
                                $id('boxsize').innerHTML = Math.round(userList.item(i).getAttribute("boxsize") / 1024);
                                $id('audit').innerHTML = userList.item(i).getAttribute("audit") == "yes" ? LANG_RESOURCE['YES'] : LANG_RESOURCE['NO'];

                                $id('ALIAS').value = userList.item(i).getAttribute("alias");
                            }
                        }
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function init() {
    window.parent.$id('MAILBAR').style.display = "none";
    window.parent.$id('DIRBAR').style.display = "none";
    window.parent.$id('CALBAR').style.display = "none";
    window.parent.$id('READCALBAR').style.display = "none";
    window.parent.$id('NULLBAR').style.display = "block";
}

function uninit() {}
