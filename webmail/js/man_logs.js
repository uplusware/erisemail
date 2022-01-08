function init() {
    window.parent.change_tab("log");
    list_logs();
}

function list_logs() {
    var api_url = "/api/listlogs.xml";
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id("STATUS").innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var logList = responseNode.childNodes;

                    for (var i = 0; i < logList.length; i++) {
                        if (logList.item(i).tagName == "log") {
                            tr = $id('LOGTBL').insertRow($id('LOGTBL').rows.length);
                            tr.setAttribute("name", logList.item(i).getAttribute("name"));
                            tr.setAttribute("size", logList.item(i).getAttribute("size"));

                            tr.onmouseover = function () {
                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('focusbg.gif')";
                            }

                            tr.onmouseout = function () {

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            var td0 = tr.insertCell(0);
                            td0.style.verticalAlign = "middle";
                            td0.style.textAlign = "center";
                            td0.style.height = "22px";
                            td0.style.width = "22px";
                            setStyle(td0, "TD.gray");
                            td0.innerHTML = "<img src=\"man_log_titleicon.gif\">";

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "left";
                            td1.style.height = "22px";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = "<a target=\"_blank\" href=" + "api/getlog.cgi?LOG_NAME=" + logList.item(i).getAttribute("name") + ">" + logList.item(i).getAttribute("name") + "</a>";

                            var td2 = tr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "left";
                            td2.style.width = "100px";
                            td2.style.height = "22px";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = Math.round(logList.item(i).getAttribute("size") / 1024 * 100) / 100 + "KB";

                        }
                    }
                    $id("STATUS").innerHTML = "";
                    $id("STATUS").style.display = "none";
                }
            }
        }
    });
}

$(document).ready(function () {
    init();
});

$(window).on('unload', function () {

})