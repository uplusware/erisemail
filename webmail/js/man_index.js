function manlogin() {
    var api_data = "USER_NAME=" + encodeURIComponent($id("USER_NAME").value) + "&USER_PWD=" + encodeURIComponent($id("USER_PWD").value);
    var api_url = "/api/manlogin.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        beforeSend: function (xmldom) {
            $id("content").innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $id("content").innerHTML = "<img src=\"waiting.gif\">";
                    window.location = "/man_home.html";
                } else {
                    $id("content").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>" + LANG_RESOURCE['LOGIN_FAILED'] + "</td></tr></table>";
                }
            }
        }
    });
}

$(document).ready(function () {
    $('input').mouseover(function () {
        this.focus();
        this.select()
    });

    $("#manlogin_button").click(function () {
        manlogin();
    });
});

$(window).on('unload', function () {

})