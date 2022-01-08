function reject_mail() {
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
                do_reject_mail(document.getElementsByName('sel')[x].value);
            }
        }
    }
}

function pass_mail() {
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
                do_pass_mail(document.getElementsByName('sel')[x].value);
            }
        }
    }
}

function do_pass_mail(mid) {
    var trid = $id("mailtr" + mid);
    if (trid == null)
        return false;

    var api_data = "MAILID=" + encodeURIComponent(mid);
    var api_url = "/api/passmail.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $id('MAILTBL').deleteRow(trid.rowIndex);

                    load_unaudied_mails($id('PAGE_CUR').value);
                } else {
                    trid.cells[2].style.color = "";
                    trid.cells[3].style.color = "";
                    trid.cells[4].style.color = "";
                    trid.cells[5].style.color = "";
                }
            }
        },
        error: function () {
            trid.cells[2].style.color = "#00FF00";
            trid.cells[3].style.color = "#00FF00";
            trid.cells[4].style.color = "#00FF00";
            trid.cells[5].style.color = "#00FF00";
        }
    });
}

function do_reject_mail(mid) {
    var trid = $id("mailtr" + mid);
    if (trid == null)
        return false;

    var api_data = "MAILID=" + encodeURIComponent(mid);
    var api_url = "/api/rejectmail.xml";
    $.ajax({
        url: api_url,
        type: "POST",
        data: api_data,
        success: function (xmldom) {
            xmldom.documentElement.normalize();
            var responseNode = xmldom.documentElement.childNodes.item(0);
            if (responseNode.tagName == "response") {
                var errno = responseNode.getAttribute("errno")
                if (errno == "0" || errno == 0) {
                    $id('MAILTBL').deleteRow(trid.rowIndex);

                    load_unaudied_mails($id('PAGE_CUR').value);
                } else {
                    trid.cells[2].style.color = "";
                    trid.cells[3].style.color = "";
                    trid.cells[4].style.color = "";
                    trid.cells[5].style.color = "";
                }
            }
        },
        error: function () {
            trid.cells[2].style.color = "#FF0000";
            trid.cells[3].style.color = "#FF0000";
            trid.cells[4].style.color = "#FF0000";
            trid.cells[5].style.color = "#FF0000";
        }
    });
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

var num_of_page = 10;

function get_auditpage_num(npage) {
    var api_url = "/api/getunauditedmailnum.xml";
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

function sel_all(sel) {
    for (var x = 0; x < document.getElementsByName('sel').length; x++) {
        document.getElementsByName('sel')[x].checked = sel;
    }
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

function load_unaudied_mails(npage) {
    if (!npage) {
        npage = 0;
    }
    get_auditpage_num(npage);

    var row = num_of_page;
    var beg = npage * num_of_page;

    var api_url = "/api/listunauditedmails.xml?BEG=" + beg + "&ROW=" + row;
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
                            if (mailList.item(i).getAttribute("flaged") == "yes")
                                flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"hot.gif\">";
                            else
                                flaged = "<img id=\"flag" + mailList.item(i).getAttribute("id") + "\" src=\"unhot.gif\">";

                            if (mailList.item(i).getAttribute("subject") == "" || mailList.item(i).getAttribute("subject") == null)
                                strSubject = LANG_RESOURCE['NO_SUBJECT'];
                            else
                                strSubject = mailList.item(i).getAttribute("subject");
                            while ($id('MAILTBL').rows.length > num_of_page) {
                                $id('MAILTBL').deleteRow($id('MAILTBL').rows.length - 1);
                            }

                            var pos = get_insert_pos(mailList.item(i).getAttribute("sort"), mailList.item(i).getAttribute("uniqid"));

                            if (pos == -1)
                                continue;

                            tr = $id('MAILTBL').insertRow(pos);

                            tr.setAttribute("id", "mailtr" + mailList.item(i).getAttribute("id"));
                            tr.setAttribute("time", mailList.item(i).getAttribute("time"));
                            tr.setAttribute("sort", mailList.item(i).getAttribute("sort"));
                            tr.setAttribute("uniqid", mailList.item(i).getAttribute("uniqid"));
                            var gotourl = "man_reviewmail.html?ID=" + mailList.item(i).getAttribute("id");

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
                            td0.innerHTML = "<input type=\"checkbox\" name=\"sel\" onclick='check_selall()' value=\"" + mailList.item(i).getAttribute("id") + "\">"

                            var td1 = tr.insertCell(1);
                            td1.style.verticalAlign = "middle";
                            td1.style.textAlign = "center";
                            td1.style.height = "22px";
                            td1.style.width = "22px";
                            setStyle(td1, "TD.gray");
                            td1.setAttribute("selflink", gotourl);

                            td1.innerHTML = (parseInt(mailList.item(i).getAttribute("attachcount")) > 0) ? "<img src=\"attach.gif\">" : "<img src=\"pad.gif\">"

                            td1.onclick = function () {
                                window.open(this.getAttribute("selflink"));
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
                            td2.style.height = "22px";
                            td2.style.width = "120px";;
                            setStyle(td2, "TD.gray");
                            td2.setAttribute("selflink", gotourl);

                            td2.innerHTML = TextToHTML(mailList.item(i).getAttribute("sender"));

                            td2.onclick = function () {
                                window.open(this.getAttribute("selflink"));
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
                            td3.style.height = "22px";
                            setStyle(td3, "TD.gray");
                            td3.setAttribute("selflink", gotourl);
                            td3.innerHTML = TextToHTML(strSubject);

                            td3.onclick = function () {
                                window.open(this.getAttribute("selflink"));
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
                            td4.style.textAlign = "right";
                            td4.style.height = "22px";
                            td4.style.width = "80";
                            setStyle(td4, "TD.gray");
                            td4.setAttribute("selflink", gotourl);
                            td4.innerHTML = Math.round(mailList.item(i).getAttribute("size") / 1024 * 100) / 100 + "KB";
                            td4.onclick = function () {
                                window.open(this.getAttribute("selflink"));
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
                            td5.style.textAlign = "center";
                            td5.style.height = "22px";
                            td5.style.width = "150";
                            setStyle(td5, "TD.gray");
                            td5.setAttribute("selflink", gotourl);
                            td5.innerHTML = mailList.item(i).getAttribute("time");

                            td5.onclick = function () {
                                window.open(this.getAttribute("selflink"));
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
                            td6.style.textAlign = "center";
                            td6.style.height = "22px";
                            td6.style.width = "30";
                            setStyle(td6, "TD.gray");
                            td6.setAttribute("mid", mailList.item(i).getAttribute("id"));
                            td6.innerHTML = "<img src=\"pass.gif\" alt=\"" + LANG_RESOURCE['PASS_DESC'] + "\">";

                            td6.onclick = function () {
                                do_pass_mail(this.getAttribute("mid"));
                            }

                            td6.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td6.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }

                            var td7 = tr.insertCell(7);
                            td7.style.verticalAlign = "middle";
                            td7.style.textAlign = "center";
                            td7.style.height = "22px";
                            td7.style.width = "30";
                            setStyle(td7, "TD.gray");
                            td7.setAttribute("mid", mailList.item(i).getAttribute("id"));
                            td7.innerHTML = "<img src=\"reject.gif\" alt=\"" + LANG_RESOURCE['REJECTED_DESC'] + "\">";

                            td7.onclick = function () {
                                do_reject_mail(this.getAttribute("mid"));
                            }

                            td7.onmouseover = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "pointer";

                                this.setAttribute("imagesrc", this.style.backgroundImage);
                                this.style.backgroundImage = "url('activebg.gif')";
                            }

                            td7.onmouseout = function () {
                                this.mousepoint = 99;
                                this.style.cursor = "default";

                                this.style.backgroundImage = this.getAttribute("imagesrc");
                            }
                        }
                    }
                    check_selall();
                }
            }
        },
        error: function () {
            $id('STATUS').style.display = "block";
            $id('STATUS').innerHTML = "<center><img src=\"alert.gif\"></center>";
        }
    });
}

function init() {
    window.parent.change_tab("mail");
    load_unaudied_mails($id('PAGE_CUR').value);
}

$(document).ready(function () {
    init();

    $('#first_page_btn').click(function () {
        clear_table($id('MAILTBL'));
        $id('PAGE_CUR').value = 0;
        load_unaudied_mails($id('PAGE_CUR').value);
    });

    $('#prev_page_btn').click(function () {
        clear_table($id('MAILTBL'));
        $id('PAGE_CUR').value = (parseInt($id('PAGE_CUR').value) - 1);
        load_unaudied_mails($id('PAGE_CUR').value);
    });

    $('#next_page_btn').click(function () {
        clear_table($id('MAILTBL'));
        $id('PAGE_CUR').value = (parseInt($id('PAGE_CUR').value) + 1);
        load_unaudied_mails($id('PAGE_CUR').value);
    });

    $('#last_page_btn').click(function () {
        clear_table($id('MAILTBL'));
        $id('PAGE_CUR').value = (parseInt($id('PAGE_NUM').value) - 1);
        load_unaudied_mails($id('PAGE_CUR').value);
    });

    $('#selall').click(function () {
        sel_all(this.checked);
    });
});

$(window).on('unload', function () {

})