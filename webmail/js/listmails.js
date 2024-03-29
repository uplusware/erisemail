function do_delmail(mid) {
    var strmid = "mailtr" + mid;
    var trid = $id(strmid);
    if (trid == null)
        return false;
    var api_url = "/api/delmail.xml?MAILID=" + mid;
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $id('MAILTBL').deleteRow(trid.rowIndex);
                    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
                } else {
                    trid.cells[3].style.textDecoration = "";
                    trid.cells[4].style.textDecoration = "";
                    trid.cells[5].style.textDecoration = "";
                    trid.cells[6].style.textDecoration = "";
                }
            }
        },
        error: function (xmldom) {
            trid.cells[3].style.textDecoration = "line-through";
            trid.cells[4].style.textDecoration = "line-through";
            trid.cells[5].style.textDecoration = "line-through";
            trid.cells[6].style.textDecoration = "line-through";
        }
    });
}

function ontimer() {
    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
    setTimeout('ontimer()', 30000);
}

function do_trashmail(mid) {
    var strmid = "mailtr" + mid;
    var trid = $id(strmid);
    if (trid == null)
        return false;
    var api_url = "/api/trashmail.xml?MAILID=" + mid;
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $id('MAILTBL').deleteRow(trid.rowIndex);
                    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
                } else {

                    trid.cells[3].style.textDecoration = "";
                    trid.cells[4].style.textDecoration = "";
                    trid.cells[5].style.textDecoration = "";
                    trid.cells[6].style.textDecoration = "";
                }
            }
        },
        error: function (xmldom) {
            trid.cells[3].style.textDecoration = "line-through";
            trid.cells[4].style.textDecoration = "line-through";
            trid.cells[5].style.textDecoration = "line-through";
            trid.cells[6].style.textDecoration = "line-through";
        }
    });
}

function delmail(force) {
    var sel_num = 0
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            sel_num++;
        }

    }
    if (sel_num == 0) {
        alert(LANG_RESOURCE['NOSELECTED_MAIL_WARNING']);
    } else {
        for (var x = 0; x < document.getElementsByName('sel').length; x++) {
            if (document.getElementsByName('sel')[x].checked == true) {
                if (force == true) {
                    do_delmail(document.getElementsByName('sel')[x].value);
                } else {
                    if (Request.QueryString('TOP_USAGE') == "trash")
                        do_delmail(document.getElementsByName('sel')[x].value);
                    else
                        do_trashmail(document.getElementsByName('sel')[x].value);
                }
            }
        }
    }
}

function refresh() {
    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
}

function remail() {
    var sel_num = 0
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            sel_num++;
        }

    }
    if (sel_num > 1) {
        alert(LANG_RESOURCE['REPLAY_ONLY_ONE_MAIL_WARNING']);
    } else if (sel_num == 0) {
        alert(LANG_RESOURCE['NOSELECTED_MAIL_WARNING']);
    } else {
        for (var x = 0; x < document.getElementsByName('sel').length; x++) {
            if (document.getElementsByName('sel')[x].checked == true) {
                var url = "writemail.html?TYPE=1&ID=" + document.getElementsByName('sel')[x].value;
                show_mail_detail(url);
                break;
            }

        }
    }
}

function sel_all(sel) {
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        document.getElementsByName('sel')[x].checked = sel;
    }
}

function remail_all() {
    var sel_num = 0
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            sel_num++;
        }

    }
    if (sel_num > 1) {
        alert(LANG_RESOURCE['REPLAY_ONLY_ONE_MAIL_WARNING']);
    } else if (sel_num == 0) {
        alert(LANG_RESOURCE['NOSELECTED_MAIL_WARNING']);
    } else {
        for (var x = 0; x < document.getElementsByName('sel').length; x++) {
            if (document.getElementsByName('sel')[x].checked == true) {
                var url = "writemail.html?TYPE=2&ID=" + document.getElementsByName('sel')[x].value;
                show_mail_detail(url);
                break;
            }

        }
    }
}

function forward_mail() {
    var sel_num = 0
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            sel_num++;
        }

    }
    if (sel_num > 1) {
        alert(LANG_RESOURCE['FORWARD_ONLY_ONE_MAIL_WARNING']);
    } else if (sel_num == 0) {
        alert(LANG_RESOURCE['NOSELECTED_MAIL_WARNING']);
    } else {
        for (var x = 0; x < document.getElementsByName('sel').length; x++) {
            if (document.getElementsByName('sel')[x].checked == true) {
                var url = "writemail.html?TYPE=3&ID=" + document.getElementsByName('sel')[x].value;
                show_mail_detail(url);
                break;
            }

        }
    }
}

function do_flag_mail(flag, mid) {
    var fid = "flag" + mid;
    if (!$id(fid)) {
        return;
    }

    var oldsrc = $id(fid).src;

    var api_url = "/api/flagmail.xml?ID=" + mid + "&FLAG=" + (flag == true ? "yes" : "no");
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id(fid).src = "loading.gif";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strmid = "mailtr" + mid;
                    var trid = $id(strmid);
                    if (flag == true) {
                        trid.cells[1].setAttribute("flag", "yes");
                        $id(fid).src = "hot.gif";
                    } else {
                        trid.cells[1].setAttribute("flag", "no");
                        $id(fid).src = "unhot.gif";
                    }

                } else {
                    $id(fid).src = oldsrc;
                }
            }
        }
    });
}

function flag_mail(flag) {
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            do_flag_mail(flag, document.getElementsByName('sel')[x].value);
        }
    }
}

function do_seen_mail(flag, mid) {
    var seenid = "seen" + mid
    if (!$id(seenid)) {
        return;
    }

    var oldsrc = $id(seenid).src;

    var api_url = "/api/seenmail.xml?ID=" + mid + "&SEEN=" + (flag == true ? "yes" : "no");
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id(seenid).src = "loading.gif";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strmid = "mailtr" + mid;
                    var trid = $id(strmid);

                    if (flag == true) {
                        trid.style.fontWeight = "normal"
                        $id(seenid).src = "seen.gif";
                    } else {
                        trid.style.fontWeight = "bold"
                        $id(seenid).src = "unseen.gif";
                    }
                } else {

                    $id(seenid).src = oldsrc;
                }
            }
        }
    });
}

function seen_mail(flag) {
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            do_seen_mail(flag, document.getElementsByName('sel')[x].value);
        }
    }
}

function do_copy_mail(mid, todir) {
    var trid = $id("mailtr" + mid);
    if (trid == null) {
        return false;
    }

    var api_url = "/api/copymail.xml?MAILID=" + mid + "&TODIRS=" + todir;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            trid.cells[3].style.color = "#FF0000";
            trid.cells[4].style.color = "#FF0000";
            trid.cells[5].style.color = "#FF0000";
            trid.cells[6].style.color = "#FF0000";
        },
        success: function (xmldom) {
            trid.cells[3].style.color = "";
            trid.cells[4].style.color = "";
            trid.cells[5].style.color = "";
            trid.cells[6].style.color = "";

            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
                    return true;
                } else {
                    return false;
                }
            }
        }
    });
    return true;
}

function copy_mail(todirs) {
    if (document.getElementsByName('sel').length == 0) {
        alert(LANG_RESOURCE['NOSELECTED_COPY_MAIL_WARNING']);
        return;
    }

    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            do_copy_mail(document.getElementsByName('sel')[x].value, todirs);
        }
    }
}

function do_move_mail(mid, todir) {
    var trid = $id("mailtr" + mid);
    if (trid == null) {
        return false;
    }

    var api_url = "/api/movemail.xml?MAILID=" + mid + "&TODIRS=" + todir;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            trid.cells[3].style.color = "#C0C0C0";
            trid.cells[4].style.color = "#C0C0C0";
            trid.cells[5].style.color = "#C0C0C0";
            trid.cells[6].style.color = "#C0C0C0";
        },
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strmid = "mailtr" + mid;
                    var trid = $id(strmid);
                    $id('MAILTBL').deleteRow(trid.rowIndex);

                    load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
                } else {
                    trid.cells[3].style.color = "";
                    trid.cells[4].style.color = "";
                    trid.cells[5].style.color = "";
                    trid.cells[6].style.color = "";
                }
            }
        }
    });
    return true;
}

function move_mail(todir) {
    if (document.getElementsByName('sel').length == 0) {
        alert(LANG_RESOURCE['NOSELECTED_MOVE_MAIL_WARNING']);
        return;
    }

    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == true) {
            do_move_mail(document.getElementsByName('sel')[x].value, todir);
        }
    }
}

var num_of_page = 20;

function get_page_num(dirid, npage) {
    var api_url = "/api/getmailnum.xml?DIRID=" + (dirid ? dirid : "");
    $.ajax({
        url: api_url,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var mailnum;
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var countList = responseNode.childNodes;
                    for (var i = 0; i < countList.length; i++) {
                        if (countList.item(i).tagName == "count") {
                            mailnum = countList.item(i).childNodes[0] == null ? "" : countList.item(i).childNodes[0].nodeValue;

                            if (mailnum % num_of_page == 0)
                                $id("PAGE_NUM").value = (mailnum / num_of_page);
                            else
                                $id("PAGE_NUM").value = ((mailnum - mailnum % num_of_page) / num_of_page + 1);

                            $id("PAGE").value = (parseInt(npage) + 1) + "/" + $id("PAGE_NUM").value;

                            update_page_button();
                        }
                    }
                }
            }
        }
    });
}

function get_insert_pos(time, uniqid) {
    var i = 0;
    var tmp = 0;
    if ($id('MAILTBL').rows.length == 0)
        return 0;

    for (i = 0; i < $id('MAILTBL').rows.length; i++) {
        if ($id('MAILTBL').rows[i].getAttribute("uniqid") == uniqid) {
            return -1;
        } else {
            if (parseInt(time) > parseInt($id('MAILTBL').rows[i].getAttribute("sort"))) {
                return i;
            }
        }
    }
    return i;
}

function check_selall() {
    var x = 0;
    for (x = 0; x < document.getElementsByName('sel').length; x++) {
        if (document.getElementsByName('sel')[x].checked == false) {
            break;
        }
    }
    if (document.getElementsByName('sel').length == 0) {
        if ($id('selall') != null)
            $id('selall').checked = false;
    } else {
        if ($id('selall') != null) {
            if (x == document.getElementsByName('sel').length)
                $id('selall').checked = true;
            else
                $id('selall').checked = false;
        }
    }
}

function update_mail_status(mid, flag, seen) {
    var fid = "flag" + mid;
    var strmid = "mailtr" + mid;
    var seenid = "seen" + mid;

    if (!$id(fid) || !$id(strmid) || !$id(seenid))
        return;

    var trid = $id(strmid);
    if (flag == true) {
        trid.cells[1].setAttribute("flag", "yes");
        $id(fid).src = "hot.gif";
    } else {
        trid.cells[1].setAttribute("flag", "no");
        $id(fid).src = "unhot.gif";
    }

    if (seen == true) {
        trid.style.fontWeight = "normal"
        $id(seenid).src = "seen.gif";
    } else {
        trid.style.fontWeight = "bold"
        $id(seenid).src = "unseen.gif";
    }

}

function update_mail_flag(mid, flag) {
    var fid = "flag" + mid;
    var strmid = "mailtr" + mid;

    if (!$id(fid) || !$id(strmid))
        return;

    var trid = $id(strmid);
    if (flag == true) {
        trid.cells[1].setAttribute("flag", "yes");
        $id(fid).src = "hot.gif";
    } else {
        trid.cells[1].setAttribute("flag", "no");
        $id(fid).src = "unhot.gif";
    }
}

function update_mail_seen(mid, seen) {
    var strmid = "mailtr" + mid;
    var seenid = "seen" + mid;

    if (!$id(strmid) || !$id(seenid))
        return;

    var trid = $id(strmid);
    if (seen == true) {
        trid.style.fontWeight = "normal"
        $id(seenid).src = "seen.gif";
    } else {
        trid.style.fontWeight = "bold";
        $id(seenid).src = "unseen.gif";
    }
}

function delete_maillist_row(mid) {
    var strmid = "mailtr" + mid;
    var trid = $id(strmid);
    $id('MAILTBL').deleteRow(trid.rowIndex);

}

function load_mails(dirid, npage, strUsage) {
    dirid = dirid ? dirid : "";
    npage = npage ? npage : 0;
    strUsage = strUsage ? strUsage : "common";

    get_page_num(dirid, npage);

    var row = num_of_page;
    var beg = npage * num_of_page;

    var api_url = "/api/listmails.xml?DIRID=" + (dirid ? dirid : "") + "&BEG=" + beg + "&ROW=" + row;
    $.ajax({
        url: api_url,
        beforeSend: function (xmldom) {
            $id('STATUS').style.display = "block";
            $id('STATUS').innerHTML = "<center><img src=\"waiting.gif\"></center>";
        },
        success: function (xmldom) {
            $id('STATUS').style.display = "none";
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    var strTmp;
                    var mailList = responseNode.childNodes;

                    for (var i = 0; i < mailList.length; i++) {
                        if (mailList.item(i).tagName == "mail") {
                            var isbold,
                                seen,
                                flaged;

                            if (mailList.item(i).getAttribute("seen") == "no") {
                                seen = "<img id=\"seen" + mailList.item(i).getAttribute("id") + "\" src=\"unseen.gif\">";
                                isbold = true;
                            } else {
                                seen = "<img id=\"seen" + mailList.item(i).getAttribute("id") + "\" src=\"seen.gif\">";
                                isbold = false;
                            }

                            if (mailList.item(i).getAttribute("flaged") == "yes")
                                flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"hot.gif\">";
                            else
                                flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"unhot.gif\">";

                            if (mailList.item(i).getAttribute("subject") == "" || mailList.item(i).getAttribute("subject") == null)
                                strSubject = LANG_RESOURCE['NO_SUBJECT'];
                            else
                                strSubject = mailList.item(i).getAttribute("subject");

                            strSubject = abbrString(strSubject, 32);
                            while ($id('MAILTBL').rows.length > num_of_page) {
                                $id('MAILTBL').deleteRow($id('MAILTBL').rows.length - 1);
                            }

                            var pos = get_insert_pos(mailList.item(i).getAttribute("sort"), mailList.item(i).getAttribute("uniqid"));

                            if (pos == -1) {
                                var bFlag,
                                    bSeen;

                                if (mailList.item(i).getAttribute("flaged") == "no") {
                                    bFlag = false;
                                } else {
                                    bFlag = true;
                                }

                                if (mailList.item(i).getAttribute("seen") == "yes") {
                                    bSeen = true;
                                } else {
                                    bSeen = false;
                                }

                                update_mail_status(mailList.item(i).getAttribute("id"), bFlag, bSeen);
                                continue;
                            }
                            tr = $id('MAILTBL').insertRow(pos);

                            if (i % 2 == 1) {
                                tr.style.backgroundColor = "#F0F0F0";
                            } else {
                                tr.style.backgroundColor = "#FFFFFF";
                            }

                            tr.setAttribute("id", "mailtr" + mailList.item(i).getAttribute("id"));
                            tr.setAttribute("mid", mailList.item(i).getAttribute("id"));
                            tr.setAttribute("time", mailList.item(i).getAttribute("time"));
                            tr.setAttribute("sort", mailList.item(i).getAttribute("sort"));
                            tr.setAttribute("uniqid", mailList.item(i).getAttribute("uniqid"));

                            var gotourl = "";

                            if (strUsage == "drafts")
                                gotourl = "writemail.html?GPATH=" + (Request.QueryString('GPATH') == null ? encodeURIComponent("/" + LANG_RESOURCE['MAILBOX_INBOX']) : Request.QueryString('GPATH')) + "&TOP_USAGE=" + (Request.QueryString('TOP_USAGE') == null ? "common" : Request.QueryString('TOP_USAGE')) + "&DIRID=" + dirid + "&ID=" + mailList.item(i).getAttribute("id") + "&TYPE=" + 4;
                            else
                                gotourl = "readmail.html?GPATH=" + (Request.QueryString('GPATH') == null ? encodeURIComponent("/" + LANG_RESOURCE['MAILBOX_INBOX']) : Request.QueryString('GPATH')) + "&TOP_USAGE=" + (Request.QueryString('TOP_USAGE') == null ? "common" : Request.QueryString('TOP_USAGE')) + "&DIRID=" + dirid + "&ID=" + mailList.item(i).getAttribute("id");

                            tr.onmouseover = function () {
                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('focusbg.gif')";
                            }

                            tr.onmouseout = function () {
                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            if (isbold == true)
                                tr.style.fontWeight = "bold";
                            else
                                tr.style.fontWeight = "normal";

                            var td0 = tr.insertCell(0);

                            td0.style.verticalAlign = "middle";
                            td0.style.textAlign = "center";
                            td0.style.height = "26px";
                            td0.style.width = "22px";
                            td0.innerHTML = "<input type=\"checkbox\" name=\"sel\" onclick='check_selall()' value=\"" + mailList.item(i).getAttribute("id") + "\">";

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "center";
                            td1.style.height = "26px";
                            td1.style.width = "22px";
                            td1.setAttribute("mid", mailList.item(i).getAttribute("id"));
                            td1.setAttribute("flag", mailList.item(i).getAttribute("flaged"));
                            td1.innerHTML = flaged;

                            td1.onclick = function () {
                                if (this.getAttribute("flag") == "yes") {
                                    do_flag_mail(false, this.getAttribute("mid"));
                                } else if (this.getAttribute("flag") == "no") {
                                    do_flag_mail(true, this.getAttribute("mid"));
                                }
                            }

                            td1.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td1.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td2 = tr.insertCell(2);
                            td2.style.verticalAlign = "middle";
                            td2.style.textAlign = "center";
                            td2.style.height = "26px";
                            td2.style.width = "22px";
                            td2.setAttribute("selflink", gotourl);
                            td2.innerHTML = seen;
                            td2.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td2.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td2.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td3 = tr.insertCell(3);
                            td3.style.verticalAlign = "middle";
                            td3.style.textAlign = "center";
                            td3.style.height = "26px";
                            td3.style.width = "22px";
                            td3.setAttribute("selflink", gotourl);

                            td3.innerHTML = (parseInt(mailList.item(i).getAttribute("attachcount")) > 0) ? "<img src=\"attach.gif\">" : "<img src=\"pad.gif\">"

                            td3.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td3.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td3.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td4 = tr.insertCell(4);
                            td4.style.verticalAlign = "middle";
                            td4.style.textAlign = "left";
                            td4.style.height = "26px";
                            td4.style.width = "120px";
                            td4.setAttribute("selflink", gotourl);

                            td4.innerHTML = TextToHTML(mailList.item(i).getAttribute("sender"));

                            td4.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td4.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td4.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td5 = tr.insertCell(5);
                            td5.style.verticalAlign = "middle";
                            td5.style.height = "26px";
                            td5.setAttribute("selflink", gotourl);
                            td5.innerHTML = TextToHTML(strSubject);
                            td5.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td5.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td5.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td6 = tr.insertCell(6);
                            td6.style.verticalAlign = "middle";
                            td6.style.textAlign = "right";
                            td6.style.height = "26px";
                            td6.style.width = "80px";
                            td6.setAttribute("selflink", gotourl);
                            td6.innerHTML = Math.round(mailList.item(i).getAttribute("size") / 1024 * 100) / 100 + "KB";
                            td6.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td6.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td6.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }

                            var td7 = tr.insertCell(7);
                            td7.style.verticalAlign = "middle";
                            td7.style.textAlign = "center";
                            td7.style.height = "26px";
                            td7.style.width = "150px";
                            td7.setAttribute("selflink", gotourl);
                            td7.innerHTML = mailList.item(i).getAttribute("time");

                            td7.onclick = function () {
                                show_mail_detail(this.getAttribute("selflink"));
                                update_mail_seen(this.parentNode.getAttribute("mid"), true);

                                this.parentNode.style.backgroundImage = this.parentNode.getAttribute("imagesrc");
                            }

                            td7.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";
                            }

                            td7.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";
                            }
                        }
                    }
                    check_selall();
                }
            }
        },
        error: function (xmldom) {
            $id('STATUS').style.display = "block";
            $id('STATUS').innerHTML = "<center><img src=\"alert.gif\"></center>";
        }
    });
}

function init() {
    window.parent.$id('REMAIL').src = "remail.gif";
    window.parent.$id('REMAIL').disabled = false;

    window.parent.$id('RETOALL').src = "retoall.gif";
    window.parent.$id('RETOALL').disabled = false;

    window.parent.$id('FORWARD').src = "forward.gif";
    window.parent.$id('FORWARD').disabled = false;

    window.parent.$id('MAILBAR').style.display = "block";
    window.parent.$id('DIRBAR').style.display = "none";
    window.parent.$id('CALBAR').style.display = "none";
    window.parent.$id('READCALBAR').style.display = "none";
    window.parent.$id('NULLBAR').style.display = "none";

    $id('GLOBALPATH').innerHTML = (Request.QueryString('GPATH') == null ? ("/" + LANG_RESOURCE['MAILBOX_INBOX']) : decodeURIComponent(Request.QueryString('GPATH')));

    ontimer();
}

function update_page_button() {
    if ($id('PAGE_CUR').value < 0) {
        $id('PAGE_CUR').value = 0;
    }

    if ($id('PAGE_NUM').value <= 0) {
        $id('PAGE_NUM').value = 0;

        $id('next_page_btn').src = "next2.gif";
        $id('next_page_btn').disabled = true;

        $id('last_page_btn').src = "last2.gif"
        $id('last_page_btn').disabled = true;

        $id('first_page_btn').src = "first2.gif";
        $id('first_page_btn').disabled = true;

        $id('prev_page_btn').src = "prev2.gif"
        $id('prev_page_btn').disabled = true;

    } else {
        if ($id('PAGE_CUR').value <= 0) {
            $id('PAGE_CUR').value = 0;

            $id('first_page_btn').src = "first2.gif";
            $id('first_page_btn').disabled = true;

            $id('prev_page_btn').src = "prev2.gif"
            $id('prev_page_btn').disabled = true;

        } else {
            $id('first_page_btn').src = "first.gif";
            $id('first_page_btn').disabled = false;

            $id('prev_page_btn').src = "prev.gif"
            $id('prev_page_btn').disabled = false;
        }

        if (parseInt($id('PAGE_CUR').value) >= parseInt($id('PAGE_NUM').value - 1)) {
            $id('PAGE_CUR').value = parseInt($id('PAGE_NUM').value) - 1;

            $id('next_page_btn').src = "next2.gif";
            $id('next_page_btn').disabled = true;

            $id('last_page_btn').src = "last2.gif"
            $id('last_page_btn').disabled = true;
        } else {
            $id('next_page_btn').src = "next.gif";
            $id('next_page_btn').disabled = false;

            $id('last_page_btn').src = "last.gif"
            $id('last_page_btn').disabled = false;
        }

    }

}

function show_mail_detail(url) {
    window.open(url);
}

$(document).ready(function () {
    init();

    $('#first_page_btn').click(function () {
        clear_table($id('MAILTBL')); $id('PAGE_CUR').value = 0;
        load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
    });

    $('#prev_page_btn').click(function () {
        clear_table($id('MAILTBL')); $id('PAGE_CUR').value = (parseInt($id('PAGE_CUR').value) - 1);
        load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
    });

    $('#next_page_btn').click(function () {
        clear_table($id('MAILTBL')); $id('PAGE_CUR').value = (parseInt($id('PAGE_CUR').value) + 1);
        load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value, Request.QueryString('TOP_USAGE'));
    });

    $('#last_page_btn').click(function () {
        clear_table($id('MAILTBL')); $id('PAGE_CUR').value = (parseInt($id('PAGE_NUM').value) - 1);
        load_mails(Request.QueryString('DIRID'), $id('PAGE_CUR').value);
    });

    $('#selall').click(function () {
        sel_all(this.checked);
    });
});

$(window).on('unload', function () {

})