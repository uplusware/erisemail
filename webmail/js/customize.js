function passwd(oldpwd, newpwd, verifypwd) {
    if (newpwd != verifypwd) {
        pwdinfo.innerHTML = LANG_RESOURCE['WRONG_VERIFIED_PASSWORD'];
        return;
    }
    var api_data = "OLD_PWD=" + encodeURIComponent(oldpwd) + "&NEW_PWD=" + encodeURIComponent(newpwd);
    var api_url = "/api/passwd.xml";
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
                    alert(LANG_RESOURCE['PASSWORD_CHANGED_NEED_RELOGIN']);
                    window.parent.logout();
                } else {
                    document.getElementById("pwdinfo").innerHTML = "<font color='red'>" + LANG_RESOURCE['PASSWORD_CHANGED_FAILED_NEED_RETRY'] + "</font>";
                }
            }
        }
    });
}

function alias(stralias) {
    if (stralias == "") {
        aliasinfo.innerHTML = LANG_RESOURCE['EMPTY_ALIAS'];
        return;
    }
    var api_data = "ALIAS=" + encodeURIComponent(stralias);
    var api_url = "/api/alias.xml";
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
                    document.getElementById("aliasinfo").innerHTML = "<font color='green'>" + LANG_RESOURCE['CHANGE_ALIAS_OK'] + "</font>";
                } else {
                    document.getElementById("aliasinfo").innerHTML = "<font color='red'>" + LANG_RESOURCE['CHANGE_ALIAS_FAILED'] + "</font>";
                }
            }
        }
    });
}

function userinfo() {
    var api_url = "/api/userinfo.xml";
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

$(document).ready(function () {
    init();
    userinfo();

    $('#CHANGE_ALIAS_BUTTON').click(function () {
        alias(document.getElementById('ALIAS').value);
    });

    $('#CHANGE_PASSWORD_BUTTON').click(function () {
        passwd(document.getElementById('OLD_PWD').value, document.getElementById('NEW_PWD').value, document.getElementById('NEW_PWD_VERIFY').value);
    });

    $('input').mouseover(function () {
        this.focus();
        this.select()
    });
});

$(window).on('unload', function () {
    uninit();
})