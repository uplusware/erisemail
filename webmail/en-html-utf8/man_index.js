function manlogin() {
    var username = $id("USER_NAME").value;
    var password = $id("USER_PWD").value
        var qUrl = "/api/manlogin.xml?USER_NAME=" + username + "&USER_PWD=" + password;
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {

        if ((xmlHttp.readyState == 4) && (xmlHttp.status == 200)) {

            var xmldom = xmlHttp.responseXML;
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        $id("content").innerHTML = "<img src=\"waiting.gif\">";
                        window.location = "/man_home.html";
                    } else {
                        $id("content").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>Login Failed</td></tr></table>";
                    }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}
