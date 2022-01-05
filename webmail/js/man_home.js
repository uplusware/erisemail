function logout() {
    var api_url = "/api/logout.xml";
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    window.location = "/admin";
                }
            }
        }
    });
}

function change_tab(tabname) {

}

$(document).ready(function () {
    $('#man_status').click(function () {
        window.parent.childframe.location.href = 'man_status.html';
    });

    $('#man_levels').click(function () {
        window.parent.childframe.location.href = 'man_levels.html';
    });

    $('#man_mails').click(function () {
        window.parent.childframe.location.href = 'man_mails.html';
    });

    $('#man_users').click(function () {
        window.parent.childframe.location.href = 'man_users.html';
    });

    $('#man_groups').click(function () {
        window.parent.childframe.location.href = 'man_groups.html';
    });

    $('#man_access').click(function () {
        window.parent.childframe.location.href = 'man_access.html';
    });

    $('#man_logs').click(function () {
        window.parent.childframe.location.href = 'man_logs.html';
    });

    $('#man_cluster').click(function () {
        window.parent.childframe.location.href = 'man_cluster.html';
    });

    $('#man_logout').click(function () {
        logout();
    });
});

$(window).on('unload', function () {

})