function passwd(oldpwd, newpwd, verifypwd) {
    if (newpwd != verifypwd) {
        pwdinfo.innerHTML = "新密码两次输入不一致";
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
                        alert("密码更改成功，请重新登陆");
                        window.parent.logout();
                    } else {
                        document.getElementById("pwdinfo").innerHTML = "密码更改失败，请重新操作";
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function alias(stralias) {
    if (stralias == "") {
        aliasinfo.innerHTML = "别名不能为空";
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
                        alert("更改别名成功");
                    } else {
                        document.getElementById("aliasinfo").innerHTML = "更改别名失败";
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
                                    strRole = "管理员";
                                else if (userList.item(i).getAttribute("role") == "User")
                                    strRole = "普通用户";
                                else if (userList.item(i).getAttribute("role") == "Disabled")
                                    strRole = "禁用";
                                else
                                    strRole = "未知";

                                $id('userrole').innerHTML = strRole;

                                $id('mailsize').innerHTML = Math.round(userList.item(i).getAttribute("mailsize") / 1024);
                                $id('boxsize').innerHTML = Math.round(userList.item(i).getAttribute("boxsize") / 1024);
                                $id('audit').innerHTML = userList.item(i).getAttribute("audit") == "yes" ? "是" : "否";

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
