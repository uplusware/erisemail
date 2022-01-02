function init() {
    window.parent.change_tab("status");
    list_status();
}

function display_tbl(tblobj, btnobj) {
    if (tblobj.style.display == "none") {
        btnobj.src = "closediv.gif";
        tblobj.style.display = "block";
    } else {
        btnobj.src = "opendiv.gif";
        tblobj.style.display = "none";
    }
}

function list_status() {
    var qUrl = "/api/systemstatus.xml";
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
                        var strTmp;
                        var statusList = responseNode.childNodes;

                        for (var i = 0; i < statusList.length; i++) {
                            if (statusList.item(i).tagName == "commonMailNumber") {
                                $id('commonMailNumber').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "deletedMailNumber") {
                                $id('deletedMailNumber').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "commonMailSize") {
                                $id('commonMailSize').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : Math.round(statusList.item(i).childNodes[0].nodeValue / 1024 * 100) / 100 + "KB";
                            } else if (statusList.item(i).tagName == "deletedMailSize") {
                                $id('deletedMailSize').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : Math.round(statusList.item(i).childNodes[0].nodeValue / 1024 * 100) / 100 + "KB";
                            } else if (statusList.item(i).tagName == "Encoding") {
                                $id('Encoding').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EmailDomainName") {
                                $id('EmailDomainName').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "LocalHostName") {
                                $id('LocalHostName').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "HostIP") {
                                $id('HostIP').innerHTML = statusList.item(i).childNodes[0] == null ? LANG_RESOURCE['UNBOUND_IP_DESC'] : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "DNSServer") {
                                $id('DNSServer').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "MaxConnPerProtocal") {
                                $id('MaxConnPerProtocal').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "RelayTaskNum") {
                                $id('RelayTaskNum').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EnableRelay") {
                                $id('EnableRelay').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "SMTPHostNameCheck") {
                                $id('SMTPHostNameCheck').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "EnableSMTPTLS") {
                                $id('EnableSMTPTLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "SMTPSEnable") {
                                $id('SMTPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "SMTPSPort") {
                                $id('SMTPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "POP3Enable") {
                                $id('POP3Enable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "POP3Port") {
                                $id('POP3Port').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EnablePOP3TLS") {
                                $id('EnablePOP3TLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "POP3SEnable") {
                                $id('POP3SEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "POP3SPort") {
                                $id('POP3SPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "IMAPEnable") {
                                $id('IMAPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "IMAPPort") {
                                $id('IMAPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EnableIMAPTLS") {
                                $id('EnableIMAPTLS').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "IMAPSEnable") {
                                $id('IMAPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "IMAPSPort") {
                                $id('IMAPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "HTTPEnable") {
                                $id('HTTPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "HTTPPort") {
                                $id('HTTPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "HTTPSEnable") {
                                $id('HTTPSEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "HTTPSPort") {
                                $id('HTTPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "XMPPEnable") {
                                $id('XMPPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "XMPPPort") {
                                $id('XMPPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EncryptXMPP") {
                                var arr = ["Non-encrypted or TLS optional", "TLS required", "Old-SSL-based"];
                                $id('EncryptXMPP').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : arr[statusList.item(i).childNodes[0].nodeValue];
                            } else if (statusList.item(i).tagName == "XMPPWorkerThreadNum") {
                                $id('XMPPWorkerThreadNum').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "LDAPEnable") {
                                $id('LDAPEnable').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            } else if (statusList.item(i).tagName == "LDAPPort") {
                                $id('LDAPPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "LDAPSPort") {
                                $id('LDAPSPort').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue;
                            } else if (statusList.item(i).tagName == "EncryptLDAP") {
                                $id('EncryptLDAP').innerHTML = statusList.item(i).childNodes[0] == null ? "<img src=\"pad.gif\">" : statusList.item(i).childNodes[0].nodeValue == "yes" ? "<img src=\"yes.gif\">" : "<img src=\"no.gif\">";
                            }
                        }
                        $id("STATUS").innerHTML = "";
                        $id("STATUS").style.display = "none";
                    }
                }
            }
        } else {
            $id("STATUS").innerHTML = "<center><img src=\"waiting.gif\"></center>";
        }
    }
    xmlHttp.open("GET", qUrl, true);
    xmlHttp.send("");
}
