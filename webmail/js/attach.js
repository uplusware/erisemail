function init() {
    window.parent.uploading(false);
    window.parent.$id("attach_flag").innerHTML = "";
    $id('SELFPAGENAME').value = window.location.pathname;

    var strUploaded = Request.QueryString('UPLOADEDFILES');
    if (strUploaded == null)
        strUploaded = "";
    strUploaded = decodeURIComponent(strUploaded);
    var strShow;

    var attachs = strUploaded.split("/");

    for (var x = 0; x < attachs.length; x++) {
        if (attachs[x] != "") {
            var detail = attachs[x].split("|");
            window.parent.uploaded_file_list.push(detail[0]);
            window.parent.show_attachs(detail[0], detail[1], detail[2]);
        }
    }
}

function upfile() {
    if ($id('ATTACHFILEBODY').value != "") {
        window.parent.uploading(true);
        window.parent.$id("attach_flag").innerHTML = "<img src=\"uploading.gif\">";

        window.parent.upfile();
        $id('upfileform').submit();
        return true;
    } else {
        alert(LANG_RESOURCE['NOT_CHOOSE_ATTACH']);
        return false;
    }
}

$(document).ready(function () {
    init();

    $('#UPLOADATT').click(function () {
        return upfile();
    });
});

$(window).on('unload', function () {

})