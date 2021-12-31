function $id(obj) {
    return document.getElementById(obj);
}

function drag(element, e) {
    e = e || (event ? event : window.event);
    if (document.addEventListener) {
        document.addEventListener("mousemove", startDrag, true);
        document.addEventListener("mouseup", stopDrag, true);
    } else {
        document.onmousemove = startDrag;
        document.onmouseup = stopDrag;
    }
    var deltaX = e.clientX - parseInt(element.style.left);
    var deltaY = e.clientY - parseInt(element.style.top);
    function startDrag(e) {
        e = e || (event ? event : window.event);
        element.style.left = e.clientX - deltaX + "px";
        element.style.top = e.clientY - deltaY + "px";
    };

    function stopDrag() {
        if (document.removeEventListener) {
            document.removeEventListener("mousemove", startDrag, true);
            document.removeEventListener("mouseup", stopDrag, true);
        } else {
            document.onmousemove = "";
            document.onmouseup = "";
        }

        var oDiv = document.getElementsByTagName("div");
    };

    return true;
};

function setcolor(obj, color) {
    obj.bgColor = color;
}

function initxmlhttp() {
    var xmlhttp

    try {
        xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
    } catch (e) {
        try {
            xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
        } catch (E) {
            xmlhttp = false;
        }
    }

    if (!xmlhttp && typeof XMLHttpRequest != 'undefined') {
        try {
            xmlhttp = new XMLHttpRequest();
        } catch (e) {

            xmlhttp = false;
        }
    }
    if (!xmlhttp && window.createRequest) {
        try {
            xmlhttp = window.createRequest();
        } catch (e) {
            xmlhttp = false;
        }
    }
    return xmlhttp;
}

Request = {
    QueryString: function (item) {
        var svalue = location.search.match(new RegExp("[\?\&]" + item + "=([^\&]*)(\&?)", "i"));
        return svalue ? svalue[1] : svalue;
    }
}

function HTMLToText(str) {
    if (str == null)
        str = "";

    var regS1 = new RegExp("&lt;", "gi");
    var regS2 = new RegExp("&gt;", "gi");
    var regS3 = new RegExp("&apos;", "gi");
    var regS4 = new RegExp("&quot;", "gi");
    var regS5 = new RegExp("&nbsp;", "gi");
    var regS6 = new RegExp("<BR>", "gi");
    var regS7 = new RegExp("&amp;", "gi");

    str = str.replace(regS1, "<");
    str = str.replace(regS2, ">");
    str = str.replace(regS3, "'");
    str = str.replace(regS4, "\"");
    str = str.replace(regS5, " ");
    str = str.replace(regS6, "\r\n");
    str = str.replace(regS7, "&");

    return str;
}

function TextToHTML(str) {
    if (str == null)
        str = "";
    var regS1 = new RegExp("&", "gi");
    var regS2 = new RegExp("<", "gi");
    var regS3 = new RegExp(">", "gi");
    var regS5 = new RegExp("\"", "gi");
    var regS6 = new RegExp(" ", "gi");
    var regS7 = new RegExp("\t", "gi");
    var regS8 = new RegExp("\r", "gi");
    var regS9 = new RegExp("\n", "gi");

    str = str.replace(regS1, "&amp;");
    str = str.replace(regS2, "&lt;");
    str = str.replace(regS3, "&gt;");
    str = str.replace(regS5, "&quot;");
    str = str.replace(regS6, "&nbsp;");
    str = str.replace(regS7, "&nbsp;&nbsp;&nbsp;&nbsp;");

    str = str.replace(regS8, "");
    str = str.replace(regS9, "<BR>");

    return str;
}

function retrieve(src) {
    var befor,
    after;
    befor = "";
    after = "";
    var ctbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_.";
    var pos = src.indexOf('@');
    if (pos != -1) {
        for (var x = pos - 1; x >= 0; x--) {
            if (ctbl.indexOf(src.charAt(x)) != -1) {
                befor = src.charAt(x) + befor;
            } else
                break;
        }

        for (var y = pos + 1; y < src.length; y++) {
            if (ctbl.indexOf(src.charAt(y)) != -1) {
                after = after + src.charAt(y);
            } else
                break;
        }

        return befor + "@" + after;
    } else
        return null;
}

function CPos(x, y, w, h) {
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
}

function GetObjPos(ATarget) {
    var target = ATarget;
    var pos = new CPos(target.offsetLeft, target.offsetTop, target.offsetWidth, target.offsetHeight);
    var target = target.offsetParent;
    while (target) {
        pos.x += target.offsetLeft;
        pos.y += target.offsetTop;

        target = target.offsetParent
    }

    return pos;
}

function clear_table(tbl) {
    while (tbl.rows.length > 0) {
        tbl.deleteRow(0);
    }
}

function height_table(tbl) {
    var h = 0;
    for (var i = 0; i < tbl.rows.length; i++) {
        h += parseInt(tbl.rows[i].cells[0].height);
    }

    return h;
}

var keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

function encode64(input) {
    var output = "";
    var chr1,
    chr2,
    chr3 = "";
    var enc1,
    enc2,
    enc3,
    enc4 = "";
    var i = 0;
    do {
        chr1 = input.charCodeAt(i++);
        chr2 = input.charCodeAt(i++);
        chr3 = input.charCodeAt(i++);
        enc1 = chr1 >> 2;
        enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
        enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
        enc4 = chr3 & 63;
        if (isNaN(chr2)) {
            enc3 = enc4 = 64;
        } else if (isNaN(chr3)) {
            enc4 = 64;
        }
        output = output +
            keyStr.charAt(enc1) +
            keyStr.charAt(enc2) +
            keyStr.charAt(enc3) +
            keyStr.charAt(enc4);
        chr1 = chr2 = chr3 = "";
        enc1 = enc2 = enc3 = enc4 = "";
    } while (i < input.length);
    return output;
}

function decode64(input) {
    var output = "";
    var chr1,
    chr2,
    chr3 = "";
    var enc1,
    enc2,
    enc3,
    enc4 = "";
    var i = 0;

    if (input.length % 4 != 0) {
        return "";
    }
    var base64test = /[^A-Za-z0-9\+\/\=]/g;
    if (base64test.exec(input)) {
        return "";
    }
    do {
        enc1 = keyStr.indexOf(input.charAt(i++));
        enc2 = keyStr.indexOf(input.charAt(i++));
        enc3 = keyStr.indexOf(input.charAt(i++));
        enc4 = keyStr.indexOf(input.charAt(i++));
        chr1 = (enc1 << 2) | (enc2 >> 4);
        chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
        chr3 = ((enc3 & 3) << 6) | enc4;

        output = output + String.fromCharCode(chr1);
        if (enc3 != 64) {
            output += String.fromCharCode(chr2);
        }
        if (enc4 != 64) {
            output += String.fromCharCode(chr3);
        }
        chr1 = chr2 = chr3 = "";
        enc1 = enc2 = enc3 = enc4 = "";
    } while (i < input.length);
    return output;
}

function clear_table_without_header(tbl) {
    while (tbl.rows.length > 1) {
        tbl.deleteRow(1);
    }
}

function isLegalID(src) {
    if (src == "")
        return false;
    var ctbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_.";
    for (var x = 0; x < src.length; x++) {
        if (ctbl.indexOf(src.charAt(x)) == -1) {
            return false;
        }
    }

    return true;
}

function isNumber(src) {
    if (src == "")
        return false;

    var ctbl = "1234567890";
    for (var x = 0; x < src.length; x++) {
        if (ctbl.indexOf(src.charAt(x)) == -1) {
            return false;
        }
    }

    return true;
}

function genRandNumber(startNum, endNum) {
    var randomNumber;
    randomNumber = Math.round(Math.random() * (endNum - startNum)) + startNum;
    return randomNumber;
}

function randpwd() {
    var strpwd = "";
    var ctbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_.~!@#$%^&*()-=+{}[]|:;',.<>?/";
    for (var x = 0; x < 16; x++) {
        strpwd += ctbl.charAt(genRandNumber(0, ctbl.length));
    }
    return strpwd;
}

function DrawImage(ImgD) {
    var image = new Image();
    image.src = ImgD.src;
    if (image.width > 0 && image.height > 0) {
        var flag = true;
        if (image.width / image.height >= 100 / 100) {
            if (image.width > 100) {
                ImgD.width = 100;
                ImgD.height = (image.height * 100) / image.width;
            } else {
                ImgD.width = image.width;
                ImgD.height = image.height;
            }
            ImgD.alt = image.width + "×" + image.height;
        } else {
            if (image.height > 100) {
                ImgD.height = 100;
                ImgD.width = (image.width * 100) / image.height;
            } else {
                ImgD.width = image.width;
                ImgD.height = image.height;
            }
            ImgD.alt = image.width + "×" + image.height;
        }
    }
}

function getStyle(labname) {
    var styleSheetObj;
    var cssRulesObj;
    var styleRule;
    var styleValue = "";

    for (x = 0; x < document.styleSheets.length; x++) {
        styleSheetObj = document.styleSheets[x];
        cssRulesObj = styleSheetObj.cssRules ? styleSheetObj.cssRules : styleSheetObj.rules
            for (i = 0; i < cssRulesObj.length; i++) {
                styleRule = cssRulesObj[i];
                if (styleRule.selectorText.toLowerCase() == labname.toLowerCase()) {
                    styleValue = styleRule.style.cssText;
                    break;
                }
            }
    }
    return styleValue;
}

function strtrim(str) {
    if (str)
        return str.replace(/(^\s*)|(\s*$)/g, "");
    else
        return "";
}

function rzCC(s) {
    s = s.toLowerCase();
    for (var exp = /-([a-zA-Z])/;
        exp.test(s);
        s = s.replace(exp, RegExp.$1.toUpperCase()));
    return s;
}

function setStyle(element, labelname) {
    var declaration = getStyle(labelname);

    if (declaration.charAt(declaration.length - 1) == ';')
        declaration = declaration.slice(0, -1);
    var k,
    v;
    var splitted = declaration.split(';');
    for (var i = 0, len = splitted.length; i < len; i++) {
        k = rzCC(splitted[i].split(':')[0]);
        v = splitted[i].split(':')[1];
        if (strtrim(k) != "" && strtrim(v) != "") {
            var jscode = "element.style." + strtrim(k) + "=\"" + strtrim(v) + "\";";
            eval(jscode);
        }
    }
}

function abbrString(str, len) {
    var lstr = 0;
    for (var i = 0; i < str.length; i++) {
        if (str.charCodeAt(i) > "0" && str.charCodeAt(i) < "128") {
            lstr += 1;
        } else {
            lstr += 2;
        }
    }

    if (lstr > parseInt(len)) {
        str = str.substr(0, parseInt(len)) + "...";
    }
    return str;
}
