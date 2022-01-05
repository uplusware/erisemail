function login() {
    var api_data = "USER_NAME=" +  $id("USER_NAME").value + "&USER_PWD=" + $id("USER_PWD").value;
    var api_url = "/api/login.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        beforeSend: function (xmldom) {
            $id("content").innerHTML = "<img src=\"waiting.gif\">";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.location = "/home.html";
                } else {
                    $id("content").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>" + LANG_RESOURCE['LOGIN_FAILED'] + "</td></tr></table>";
                }
            }
        },
        error: function (xmldom) {
            $id("content").innerHTML = "<table border=\"0\"><tr><td><img src=\"alert.gif\"></td><td>" + LANG_RESOURCE['LOGIN_FAILED'] + "</td></tr></table>";
        }
    });
}


$(document).ready(function () {
    $('input').mouseover(function () {
        this.focus();
        this.select()
    });

    $("#login_button").click(function(){
        login();
    });
});

$(window).on('unload',function(){

})