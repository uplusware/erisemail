function output_dir(tblobj, pid, path, nodeObj, layer, begtr, checkname) {
    layer++;
    var a = 0;
    for (var i = 0; i < nodeObj.length; i++) {
        var strDisplayDirName = "";
        if (nodeObj.item(i).tagName == "dir") {
            var childrennum = nodeObj.item(i).getAttribute("childrennum");
            var strRealName = nodeObj.item(i).getAttribute("name");
            var strDirId = nodeObj.item(i).getAttribute("id");

            var tr = tblobj.insertRow(begtr + a);
            tr.style.height = "20px";
            var td0 = tr.insertCell(0);

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

            td0.setAttribute("pid", pid);
            td0.setAttribute("did", strDirId);
            td0.setAttribute("childrennum", childrennum);
            td0.setAttribute("path", path + "/" + strRealName);
            td0.setAttribute("layer", layer);
            td0.setAttribute("extended", "false");
            td0.setAttribute("loaded", "false");
            td0.setAttribute("savedsatus", "block");

            var childnull = "";
            for (var y = 0; y < layer; y++) {
                childnull += "<img src=\"childnull.gif\">";
            }

            if (childrennum == 0) {
                extendedimg = "<img id=\"extend" + strDirId + tblobj.getAttribute("id") + "\" src=\"pading.gif\">";
            } else {
                extendedimg = "<img id=\"extend" + strDirId + tblobj.getAttribute("id") + "\" src=\"unextended.gif\">";

                tdNew1.onclick = function () {
                    if (this.getAttribute("extended") == "false") {
                        if (this.getAttribute("loaded") == "false") {
                            load_dirs(tblobj, this.getAttribute("did"), this.getAttribute("path"), this.getAttribute("layer"), this.parentNode.parentNode.parentNode.parentNode.parentNode.rowIndex + 1, checkname);
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
                        $id('extend' + this.getAttribute("did") + tblobj.getAttribute("id")).src = "extended.gif";
                    } else {

                        for (var y = this.parentNode.parentNode.parentNode.parentNode.parentNode.rowIndex + 1; y < tblobj.rows.length; y++) {
                            if (tblobj.rows[y].cells[0].getAttribute("layer") <= this.getAttribute("layer")) {
                                break;
                            }
                            tblobj.rows[y].cells[0].setAttribute("savedsatus", tblobj.rows[y].style.display);
                            tblobj.rows[y].style.display = "none";
                        }
                        this.setAttribute("extended", "false");
                        $id('extend' + this.getAttribute("did") + tblobj.getAttribute("id")).src = "unextended.gif";
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
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\r\n";
            } else if ((strRealName.toLowerCase() == "inbox") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\t\n";
            } else if ((strRealName.toLowerCase() == "trash") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\t\n";
            } else if ((strRealName.toLowerCase() == "sent") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\t\n";
            } else if ((strRealName.toLowerCase() == "junk") && (layer == 0)) {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\t\n";
            } else {
                str_tr += "<td><img src=\"" + strIcon + "\"></td><td width=\"16\"><input class=\"checkbox1\" type=\"checkbox\" value=\"" + nodeObj.item(i).getAttribute("id") + "\" name=\"" + checkname + "\" id=\"" + checkname + "\"></td><td>" + strDisplayDirName + "</td>\t\n";
            }

            str_tr += "</tr></table>";

            tdNew2.innerHTML = str_tr;

            td0.appendChild(tableNew);

            td0.setAttribute("valign", "middle");
            td0.setAttribute("align", "left");
            td0.setAttribute("height", "20");
            a++;
        }
    }
    layer--;
    return true;
}

function load_dirs(tblobj, pid, gpath, layer, begtr, checkname) {
    var strTmp;
    var qUrl = "/api/listdirs.xml?PID=" + pid + "GPATH=" + encodeURIComponent(gpath);
    var xmlHttp = initxmlhttp();
    xmlHttp.open("GET", qUrl, false);
    xmlHttp.send("");
    if (xmlHttp.status == 200) {
        var xmldom = xmlHttp.responseXML;
        xmldom.documentElement.normalize();
        var responseNode = xmldom.documentElement.childNodes.item(0);
        if (responseNode.tagName == "response") {
            var errno = responseNode.getAttribute("errno")
            if (errno == "0" || errno == 0) {
                var dirList = responseNode.childNodes;

                output_dir(tblobj, pid, "", dirList, layer, begtr, checkname);
            }
        }
    }
    return strTmp;
}

function show_dirs(divobj, tblobj, x, y, checkname) {
    if (divobj.getAttribute("loaded") != "true") {
        clear_table(tblobj);
        load_dirs(tblobj, -1, "", -1, 0, checkname);
        divobj.setAttribute("loaded", "true");
    }

    for (var a = 0; a < document.getElementsByName(checkname).length; a++) {
        document.getElementsByName(checkname)[a].checked = false;
    }

    divobj.style.display = "block";

}

function need_reload(divobj) {
    divobj.setAttribute("loaded", "false");
}
