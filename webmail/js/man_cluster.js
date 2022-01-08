function init() {
    window.parent.change_tab("cluster");
    list_cluster();
}

function list_cluster() {
    var api_url = "/api/listcluster.xml";
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
                        if (logList.item(i).tagName == "cluster") {
                            tr = $id('CLUSTERTBL').insertRow($id('CLUSTERTBL').rows.length);
                            tr.setAttribute("host", logList.item(i).getAttribute("host"));

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
                            td0.innerHTML = "<img src=\"man_node.png\">";

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "left";
                            td1.style.height = "22px";
                            setStyle(td1, "TD.gray");
                            td1.innerHTML = "<a target=\"_blank\" href=\"http://" + logList.item(i).getAttribute("host") + ":8080\">" + logList.item(i).getAttribute("host") + "</a>";

                            var td2 = tr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "left";
                            td2.style.height = "22px";
                            setStyle(td2, "TD.gray");
                            td2.innerHTML = logList.item(i).getAttribute("desc");

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