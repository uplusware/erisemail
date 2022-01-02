function getunseenmail() {
    var xmlHttp = initxmlhttp();
    if (!xmlHttp)
        return;

    var qUrl = "/api/getunseen.xml";

    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200) {
                var xmldom = xmlHttp.responseXML;
                xmldom.documentElement.normalize();
                var mailnum;
                var responseNode = xmldom.documentElement.childNodes.item(0);
                if (responseNode.tagName == "response") {
                    var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        var strTmp;
                        var countList = responseNode.childNodes;
                        for (var i = 0; i < countList.length; i++) {
                            if (countList.item(i).tagName == "unseen") {
                                for (var x = 0; x < document.getElementsByName('UNSEEN').length; x++) {
                                    if (document.getElementsByName('UNSEEN')[x].getAttribute("did") == countList.item(i).getAttribute("did")) {
                                        mailnum = countList.item(i).getAttribute("num");
                                        if (mailnum > 0)
                                            document.getElementsByName('UNSEEN')[x].innerHTML = "(" + mailnum + ")";
                                        else
                                            document.getElementsByName('UNSEEN')[x].innerHTML = "";

                                        break;
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}

function ontimer() {

    getunseenmail();
    setTimeout('ontimer()', 10000);
}

function do_empty_trash() {
    var xmlHttp = initxmlhttp();
    if (!xmlHttp)
        return;

    var qUrl = "/api/emptytrash.xml";

    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200) {
                var xmldom = xmlHttp.responseXML;
                xmldom.documentElement.normalize();
                var mailnum;
                var responseNode = xmldom.documentElement.childNodes.item(0);
                if (responseNode.tagName == "response") {
                    var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        return true;
                    }
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
    return true;
}

function empty_trash() {
    if (confirm(LANG_RESOURCE['CONFIRM_CLEAN_TRASH']) == true)
        do_empty_trash();
}

function output_dir(tblobj, pid, path, topusage, nodeObj, layer, begtr) {
    layer++;
    var a = 0;
    for (var i = 0; i < nodeObj.length; i++) {
        var strDisplayDirName = "";
        if (nodeObj.item(i).tagName == "dir") {
            var childrennum = nodeObj.item(i).getAttribute("childrennum");
            var strRealName = nodeObj.item(i).getAttribute("name");
            var strDirId = nodeObj.item(i).getAttribute("id");

            var tr = tblobj.insertRow(begtr + a);
            var td0 = tr.insertCell(0);

            td0.setAttribute("pid", pid);
            td0.setAttribute("did", strDirId);
            td0.setAttribute("childrennum", childrennum);
            td0.setAttribute("path", path + "/" + strRealName);
            td0.setAttribute("layer", layer);
            td0.setAttribute("extended", "false");
            td0.setAttribute("loaded", "false");
            td0.setAttribute("savedsatus", "block");
            td0.setAttribute("topusage", topusage);

            var tableNew = document.createElement("table");

            tableNew.setAttribute("border", "0");
            tableNew.setAttribute("cellPadding", "0");
            tableNew.setAttribute("cellSpacing", "0");

            var trNew = tableNew.insertRow(0);
            var tdNew0 = trNew.insertCell(0);
            var tdNew1 = trNew.insertCell(1);
            var tdNew2 = trNew.insertCell(2);

            tdNew1.setAttribute("pid", pid);
            tdNew1.setAttribute("did", strDirId);
            tdNew1.setAttribute("childrennum", childrennum);
            tdNew1.setAttribute("path", path + "/" + strRealName);
            tdNew1.setAttribute("layer", layer);
            tdNew1.setAttribute("extended", "false");
            tdNew1.setAttribute("loaded", "false");
            tdNew1.setAttribute("savedsatus", "block");
            tdNew1.setAttribute("topusage", topusage);

            var childnull = "";
            for (var l = 0; l < layer + 1; l++) {
                childnull += "<img src=\"childnull.gif\">";
            }

            if (childrennum == 0) {
                extendedimg = "<img id=\"extend" + strDirId + "\" src=\"pading.gif\">";
            } else {
                extendedimg = "<img id=\"extend" + strDirId + "\" src=\"unextended.gif\">";

                tdNew1.onclick = function () {
                    if (this.getAttribute("extended") == "false") {
                        if (this.getAttribute("loaded") == "false") {
                            load_dirs(this.getAttribute("did"), this.getAttribute("path"), this.getAttribute("topusage"), this.getAttribute("layer"), this.parentNode.parentNode.parentNode.parentNode.parentNode.rowIndex + 1);
                            this.setAttribute("loaded", "true")
                        } else {
                            for (var x = this.parentNode.parentNode.parentNode.parentNode.parentNode.rowIndex + 1; x < tblobj.rows.length; x++) {
                                if (tblobj.rows[x].cells[0].getAttribute("layer") <= this.getAttribute("layer")) {
                                    break;
                                }
                                if (tblobj.rows[x].cells[0].getAttribute("layer") - 1 == this.getAttribute("layer"))
                                    tblobj.rows[x].style.display = "block";
                                else
                                    tblobj.rows[x].style.display = tblobj.rows[x].cells[0].getAttribute("savedsatus");
                            }
                        }
                        this.setAttribute("extended", "true");
                        $id('extend' + this.getAttribute("did")).src = "extended.gif";
                    } else {

                        for (var y = this.parentNode.parentNode.parentNode.parentNode.parentNode.rowIndex + 1; y < tblobj.rows.length; y++) {
                            if (tblobj.rows[y].cells[0].getAttribute("layer") <= this.getAttribute("layer")) {
                                break;
                            }
                            tblobj.rows[y].cells[0].setAttribute("savedsatus", tblobj.rows[y].style.display);
                            tblobj.rows[y].style.display = "none";
                        }
                        this.setAttribute("extended", "false");
                        $id('extend' + this.getAttribute("did")).src = "unextended.gif";
                    }
                }

                tdNew1.onmouseover = function () {
                    this.mousepoint = 99;
                    this.style.cursor = "pointer";
                }

                tdNew1.onmouseout = function () {
                    this.mousepoint = 99;
                    this.style.cursor = "default";
                }
            }

            tdNew0.innerHTML = childnull;
            tdNew1.innerHTML = extendedimg;

            var strIcon = "folder.gif";

            if (strRealName.toLowerCase() == "inbox") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_INBOX'];
                strIcon = "inbox.gif";
            } else if (strRealName.toLowerCase() == "sent") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_SENT'];
                strIcon = "sent.gif";
            } else if (strRealName.toLowerCase() == "drafts") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_DRAFTS'];
                strIcon = "draft.gif";
            } else if (strRealName.toLowerCase() == "trash") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_TRASH'];
                strIcon = "trash.gif";
            } else if (strRealName.toLowerCase() == "junk") {
                strDisplayDirName = LANG_RESOURCE['MAILBOX_JUNK'];
                strIcon = "junk.gif";
            } else {
                strDisplayDirName = strRealName;
                strIcon = "folder.gif";
            }

            var str_tr = "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr>";

            if ((strRealName.toLowerCase() == "drafts") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=drafts&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td>\r\n";
                tdNew1.setAttribute("topusage", "drafts");
                td0.setAttribute("topusage", "drafts");
            } else if ((strRealName.toLowerCase() == "inbox") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=inbox&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td>\t\n";
                tdNew1.setAttribute("topusage", "inbox");
                td0.setAttribute("topusage", "inbox");
            } else if ((strRealName.toLowerCase() == "trash") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=trash&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td><td><a href=\"javascript:empty_trash();\"> [" + LANG_RESOURCE['CLEAN_TRASH'] + "]</a></td>\t\n";
                tdNew1.setAttribute("topusage", "trash");
                td0.setAttribute("topusage", "trash");
            } else if ((strRealName.toLowerCase() == "sent") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=sent&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td>\t\n";
                tdNew1.setAttribute("topusage", "sent");
                td0.setAttribute("topusage", "sent");
            } else if ((strRealName.toLowerCase() == "junk") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=trash&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td><td></td>\t\n";
                tdNew1.setAttribute("topusage", "junk");
                td0.setAttribute("topusage", "junk");
            } else {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td><a href=\"listmails.html?TOP_USAGE=" + td0.getAttribute("topusage") + "&GPATH=" + encodeURIComponent(path) + "/" + encodeURIComponent(strDisplayDirName) + "&DIRID=" + nodeObj.item(i).getAttribute("id") + "\" target=\"childframe1\">" + strDisplayDirName + "</a></td><td><div name=\"UNSEEN\" id=\"UNSEEN\" did=\"" + nodeObj.item(i).getAttribute("id") + "\"></div></td>\t\n";
            }

            str_tr += "</tr></table>";
            tdNew2.innerHTML = str_tr;

            td0.appendChild(tableNew);

            a++;
        }
    }
    layer--;
    return true;
}

function load_dirs(pid, gpath, topusage, layer, begtr) {
    var qUrl = "/api/listdirs.xml?PID=" + pid + "GPATH=" + encodeURIComponent(gpath);
    var xmlHttp = initxmlhttp();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200) {
                var xmldom = xmlHttp.responseXML;
                xmldom.documentElement.normalize();
                var responseNode = xmldom.documentElement.childNodes.item(0);
                if (responseNode.tagName == "response") {
                    var errno = responseNode.getAttribute("errno")
                    if (errno == "0" || errno == 0) {
                        var dirList = responseNode.childNodes;
                        output_dir($id('DIRTBL'), pid, gpath, topusage, dirList, layer, begtr);
                    }
                }
            }
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}
