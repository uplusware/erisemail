function init() {
    window.parent.change_tab("access");
}

function load_config_file(cfgname, ctrlobj, statusobj) {
    var api_url = "/api/loadconfigfile.xml?CONFIG_NAME=" + cfgname;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            statusobj.innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var fileList = responseNode.childNodes;

                    for (var i = 0; i < fileList.length; i++) {
                        if (fileList.item(i).tagName == "list") {
                            ctrlobj.value = fileList.item(i).childNodes[0] == null ? "" : fileList.item(i).childNodes[0].nodeValue;
                            break;
                        }
                    }
                    statusobj.innerHTML = "";
                    statusobj.style.display = "none";
                }
            }
        }
    });
}

function save_config_file(cfgname, ctrlobj, statusobj) {
    var api_data = "CONFIG_NAME=" + encodeURIComponent(cfgname) + "&" + cfgname + "=" + encodeURIComponent($id(cfgname).value);
    var api_url = "/api/saveconfigfile.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        beforeSend: function (xmldom) {
            statusobj.innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    statusobj.innerHTML = "<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td><font color=\"green\">" + LANG_RESOURCE['SAVED'] + "</font></td></tr></table>";
                    statusobj.style.display = "block";
                }
            }
        }
    });
}

function click_div(cfgname, divobj, btnobj, ctrlobj, statusobj) {
    if (divobj.getAttribute('loaded') != 'true') {
        load_config_file(cfgname, ctrlobj, statusobj);
        divobj.setAttribute('loaded', 'true');
    }
}

$(document).ready(function () {
    $('#SAVE_GLOBAL_REJECT_LIST').click(function () {
        save_config_file('GLOBAL_REJECT_LIST', $id('GLOBAL_REJECT_LIST'), $id('GLOBAL_REJECT_LIST_STATUS'));
    });

    $('#SAVE_GLOBAL_PERMIT_LIST').click(function () {
        save_config_file('GLOBAL_PERMIT_LIST', $id('GLOBAL_PERMIT_LIST'), $id('GLOBAL_PERMIT_LIST_STATUS'));
    });

    $('#SAVE_WEBADMIN_PERMIT_LIST').click(function () {
        save_config_file('WEBADMIN_PERMIT_LIST', $id('WEBADMIN_PERMIT_LIST'), $id('WEBADMIN_PERMIT_LIST_STATUS'));
    });

    $('#SAVE_DOMAIN_PERMIT_LIST').click(function () {
        save_config_file('DOMAIN_PERMIT_LIST', $id('DOMAIN_PERMIT_LIST'), $id('DOMAIN_PERMIT_LIST_STATUS'));
    });
});

$(document).ready(function () {
    $("#ACCESS_TABS").tabs();
    click_div('GLOBAL_REJECT_LIST', $id('GLOBAL_REJECT_LIST_DIV'), $id('GLOBAL_REJECT_LIST_BTN'), $id('GLOBAL_REJECT_LIST'), $id('GLOBAL_REJECT_LIST_STATUS'));
    click_div('GLOBAL_PERMIT_LIST', $id('GLOBAL_PERMIT_LIST_DIV'), $id('GLOBAL_PERMIT_LIST_BTN'), $id('GLOBAL_PERMIT_LIST'), $id('GLOBAL_PERMIT_LIST_STATUS'));
    click_div('DOMAIN_PERMIT_LIST', $id('DOMAIN_PERMIT_LIST_DIV'), $id('DOMAIN_PERMIT_LIST_BTN'), $id('DOMAIN_PERMIT_LIST'), $id('DOMAIN_PERMIT_LIST_STATUS'));
    click_div('WEBADMIN_PERMIT_LIST', $id('WEBADMIN_PERMIT_LIST_DIV'), $id('WEBADMIN_PERMIT_LIST_BTN'), $id('WEBADMIN_PERMIT_LIST'), $id('WEBADMIN_PERMIT_LIST_STATUS'));

});

$(window).on('unload', function () {

})