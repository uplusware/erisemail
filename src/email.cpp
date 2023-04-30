/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "email.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "base.h"
#include "util/base64.h"
#include "util/general.h"

email::email(const char* domain, const char* username, const char* alias) {
  m_username = username;
  m_alias = alias;
  m_domain = domain;

  m_from_addrs = m_alias;
  m_from_addrs += " <";
  m_from_addrs += username;
  m_from_addrs += "@";
  m_from_addrs += m_domain;
  m_from_addrs += ">";

  m_to_addrs = "";
  m_cc_addrs = "";
  m_bcc_addre = ""; /* do not show in the mail content, just for send*/
  m_reply_addrs = "";
  m_user_agent = "eRisemail Server";
  m_mime_version = "1.0";
  m_subject = "";

  m_attaches.clear();

  char newuid[256];
  sprintf(newuid, "%08x_%08x_%08x_%08x%s%s", time(NULL), getpid(),
          pthread_self(), random(), "@", m_domain.c_str());
  m_message_id = newuid;

  m_content_type = "plain";

  int nboundary = 0;
  char szboundary[128];

  sprintf(szboundary, "----=_NextPart_%03d_%04X_%08X.%08X", nboundary,
          random() % 0xFFFF, time(NULL), this);
  m_boundary = szboundary;
}

email::email(const char* buf, int len) {
  srandom(time(NULL));
  fbufseg seg;
  seg.m_byte_beg = 0;
  seg.m_byte_end = 0;
  for (int x = 0; x < len; x++) {
    if ((buf[x] == '\n') && ((buf[x + 1] != ' ') || (buf[x + 1] != '\t'))) {
      seg.m_byte_end = x;
      m_vsegbuf.push_back(seg);
      seg.m_byte_beg = x + 1;
    }
  }

  m_buffer.bufcat(buf, len);
}

email::~email() {}

void email::set_from_addrs(const char* arg) {
  m_from_addrs = arg;
}

void email::set_to_addrs(const char* arg) {
  m_to_addrs = arg;
}

void email::set_cc_addrs(const char* arg) {
  m_cc_addrs = arg;
}

void email::set_bcc_addrs(const char* arg) {
  m_bcc_addre = arg;
}

void email::set_reply_addrs(const char* arg) {
  m_reply_addrs = arg;
}

void email::set_subject(const char* arg) {
  m_subject = arg;
}

void email::set_content(const char* arg) {
  m_content = arg;
}

void email::set_content_type(const char* arg) {
  m_content_type = arg;
}

int email::output(const char* clientip) {
  string strTime;
  OutTimeString(time(NULL), strTime);
  char header[1024];
  sprintf(header,
          "Received: from %s ([%s]) by %s (eRisemail) with SMTP id %s for <%s>;"
          " %s\r\n",
          "unknown", clientip, CMailBase::m_localhostname.c_str(),
          m_message_id.c_str(), m_to_addrs.c_str(), strTime.c_str());

  string strTmp;
  strTmp += header;
  strTmp += "Message-ID: <";
  strTmp += m_message_id;
  strTmp += ">\r\n";

  strTmp += "Date: ";
  strTmp += strTime;
  strTmp += "\r\n";

  strTmp += "From: ";
  strTmp += m_from_addrs;
  strTmp += "\r\n";

  strTmp += "User-Agent: eRiseMail Web Server\r\n";
  strTmp += "MIME-Version: 1.0\r\n";

  if (m_to_addrs != "") {
    strTmp += "To: ";
    strTmp += m_to_addrs;
    strTmp += "\r\n";
  }
  if (m_cc_addrs != "") {
    strTmp += "Cc: ";
    strTmp += m_cc_addrs;
    strTmp += "\r\n";
  }
  if (m_reply_addrs != "") {
    strTmp += "Reply-To: ";
    strTmp += m_reply_addrs;
    strTmp += "\r\n";
  }

  if (m_subject != "") {
    int nb64Len = BASE64_ENCODE_OUTPUT_MAX_LEN(m_subject.length());
    char* szb64buf = (char*)malloc(nb64Len);
    memset(szb64buf, 0, nb64Len);
    CBase64::Encode((char*)m_subject.c_str(), m_subject.length(), szb64buf,
                    &nb64Len);

    strTmp += "Subject: =?" + CMailBase::m_encoding + "?B?";

    strTmp += szb64buf;
    strTmp += "?=\r\n";
    free(szb64buf);
  } else {
    strTmp += "Subject: \r\n";
  }

  strTmp += "Content-Type: multipart/mixed;\r\n boundary=\"";
  strTmp += m_boundary;
  strTmp += "\"\r\n\r\n";

  strTmp += "This is a multi-part message in MIME format.\r\n";

  strTmp += "--";
  strTmp += m_boundary;
  strTmp += "\r\n";

  strTmp += "Content-Type: text/" + m_content_type +
            "; charset=" + CMailBase::m_encoding + "\r\n";

  if (m_content.length() < 54) {
    strTmp += "Content-Transfer-Encoding: 8bit\r\n\r\n";
    strTmp += m_content;
    strTmp += "\r\n\r\n";
  } else {
    strTmp += "Content-Transfer-Encoding: base64\r\n\r\n";

    char* pbuf = (char*)m_content.c_str();
    unsigned int plen = m_content.length();

    char szb64tmpbuf1[55];
    char szb64tmpbuf2[73];

    int nszb64tmplen2 = 72;

    for (int p = 0; p < ((plen % 54) == 0 ? plen / 54 : (plen / 54 + 1)); p++) {
      memset(szb64tmpbuf1, 0, 55);
      memset(szb64tmpbuf2, 0, 73);

      memcpy(szb64tmpbuf1, pbuf + p * 54,
             (plen - 54 * p) > 54 ? 54 : plen - 54 * p);
      CBase64::Encode(szb64tmpbuf1, (plen - 54 * p) > 54 ? 54 : plen - 54 * p,
                      szb64tmpbuf2, &nszb64tmplen2);

      strTmp += szb64tmpbuf2;
      strTmp += "\r\n";
    }
  }
  m_buffer.bufcat(strTmp.c_str(), strTmp.length());
  strTmp = "";

  for (int z = 0; z < m_attaches.size(); z++) {
    strTmp += "--";
    strTmp += m_boundary;
    strTmp += "\r\n";

    m_buffer.bufcat(strTmp.c_str(), strTmp.length());
    strTmp = "";

    ifstream* ifile = new ifstream(m_attaches[z].c_str(), ios_base::binary);
    if ((ifile) && (ifile->is_open())) {
      char rbuf[MEMORY_BLOCK_SIZE + 1];
      while (1) {
        if (ifile->eof()) {
          break;
        }
        ifile->read(rbuf, MEMORY_BLOCK_SIZE);
        if (ifile->bad())
          break;

        int rlen = ifile->gcount();
        m_buffer.bufcat(rbuf, rlen);
      }
      ifile->close();
    }

    if (ifile)
      delete ifile;
  }

  strTmp = "--";
  strTmp += m_boundary;
  strTmp += "--\r\n";

  m_buffer.bufcat(strTmp.c_str(), strTmp.length());

  return 0;
}

void email::add_attach(const char* file) {
  string strfile = file;
  m_attaches.push_back(strfile);
}

void email::del_attach(const char* file) {
  vector<string>::iterator x;
  for (x = m_attaches.begin(); x != m_attaches.end();) {
    if (*x == file) {
      m_attaches.erase(x);
      break;
    }
  }
}
