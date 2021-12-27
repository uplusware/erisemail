function passwd(oldpwd, newpwd, verifypwd) {
    if (newpwd != verifypwd) {
        pwdinfo.innerHTML = "Verified password is not equal with first one";
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
                        alert("Change password successfully. Please relogin");
                        window.parent.logout();
                    } else {
                        document.getElementById("pwdinfo").innerHTML = "Change password failed";
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function alias(stralias) {
    if (stralias == "") {
        aliasinfo.innerHTML = "Alias can not be empty";
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
                        alert("Change alias successfully");
                    } else {
                        document.getElementById("aliasinfo").innerHTML = "Change alias failed";
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
                                    strRole = "Administrator";
                                else if (userList.item(i).getAttribute("role") == "User")
                                    strRole = "User";
                                else if (userList.item(i).getAttribute("role") == "Disabled")
                                    strRole = "Disabled";
                                else
                                    strRole = "Unknown";

                                $id('userrole').innerHTML = strRole;

                                $id('mailsize').innerHTML = Math.round(userList.item(i).getAttribute("mailsize") / 1024);
                                $id('boxsize').innerHTML = Math.round(userList.item(i).getAttribute("boxsize") / 1024);
                                $id('audit').innerHTML = userList.item(i).getAttribute("audit") == "yes" ? "Yes" : "No";

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
