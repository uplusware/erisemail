/*
        Copyright (c) openheap, uplusware
        uplusware@gmail.com
*/
#include "calendar.h"
#include "base.h"

Calendar::Calendar(const char* owner) {
  m_owner = owner;
  m_details.clear();
  m_phase = eNull;
  calbuf = "";
}

Calendar::~Calendar() {}

void Calendar::_parse_() {
  if (calbuf != "") {
    _calendar.text += calbuf;

    string strTmp;

    _strdelete_(calbuf, "\r\n ");
    _strdelete_(calbuf, "\r\n \t");
    _strdelete_(calbuf, "\n ");
    _strdelete_(calbuf, "\n\t");

    if (m_phase == eNull && strncasecmp(calbuf.c_str(), "BEGIN:VCALENDAR",
                                        sizeof("BEGIN:VCALENDAR") - 1) == 0) {
      _calendar.text = calbuf;
      _calendar.method = "";
      _calendar.prod = "";
      _calendar.version = "";
      _calendar.events.clear();
      _calendar.time_zones.clear();

      m_phase = eCal;
    } else if (m_phase == eCal &&
               strncasecmp(calbuf.c_str(), "PRODID:", sizeof("PRODID:") - 1) ==
                   0) {
      fnfy_strcut(calbuf.c_str(), "PRODID:", " \t\r\n", " \t\r\n",
                  _calendar.prod);
    } else if (m_phase == eCal && strncasecmp(calbuf.c_str(), "VERSION:",
                                              sizeof("VERSION:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "VERSION:", " \t\r\n", " \t\r\n",
                  _calendar.version);
    } else if (m_phase == eCal &&
               strncasecmp(calbuf.c_str(), "METHOD:", sizeof("METHOD:") - 1) ==
                   0) {
      fnfy_strcut(calbuf.c_str(), "METHOD:", " \t\r\n", " \t\r\n",
                  _calendar.method);
    } else if (m_phase == eCal &&
               strncasecmp(calbuf.c_str(), "BEGIN:VTIMEZONE",
                           sizeof("BEGIN:VTIMEZONE") - 1) == 0) {
      m_phase = eTimeZone;

      _time_zone.tid = "";
      _time_zone.xlic_location = "";
      _time_zone.standards.clear();
    } else if (m_phase == eTimeZone &&
               strncasecmp(calbuf.c_str(), "TZID:", sizeof("TZID:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "TZID:", " \t\r\n", " \t\r\n",
                  _time_zone.tid);
    } else if (m_phase == eTimeZone &&
               strncasecmp(calbuf.c_str(), "X-LIC-LOCATION:",
                           sizeof("X-LIC-LOCATION:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "X-LIC-LOCATION:", " \t\r\n", " \t\r\n",
                  _time_zone.xlic_location);
    }

    else if (m_phase == eTimeZone &&
             strncasecmp(calbuf.c_str(), "BEGIN:STANDARD",
                         sizeof("BEGIN:STANDARD") - 1) == 0) {
      m_phase == eStandard;

      _standard.offsetfr = 0;
      _standard.offsetto = 0;
      _standard.tzname = "";
      _standard.dtstart = 0;
    } else if (m_phase == eStandard &&
               strncasecmp(calbuf.c_str(),
                           "TZOFFSETFROM:", sizeof("TZOFFSETFROM:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "TZOFFSETFROM:", " \t\r\n", " \t\r\n",
                  strTmp);
      sscanf(strTmp.c_str(), "%d", _standard.offsetfr);
    } else if (m_phase == eStandard &&
               strncasecmp(calbuf.c_str(),
                           "TZOFFSETTO:", sizeof("TZOFFSETTO:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "TZOFFSETFROM:", " \t\r\n", " \t\r\n",
                  strTmp);
      sscanf(strTmp.c_str(), "%d", _standard.offsetto);
    } else if (m_phase == eStandard &&
               strncasecmp(calbuf.c_str(), "TZNAME:", sizeof("TZNAME:") - 1) ==
                   0) {
      fnfy_strcut(calbuf.c_str(), "TZNAME:", " \t\r\n", " \t\r\n",
                  _standard.tzname);
    } else if (m_phase == eStandard &&
               strncasecmp(calbuf.c_str(),
                           "DTSTART:", sizeof("DTSTART:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "DTSTART:", " \t\r\n", " \t\r\n", strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02dZ", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _standard.dtstart = mktime(&dtTmp);

    } else if (m_phase == eStandard &&
               strncasecmp(calbuf.c_str(), "END:STANDARD",
                           sizeof("END:STANDARD") - 1) == 0) {
      _time_zone.standards.push_back(_standard);
      m_phase == eTimeZone;
    } else if (m_phase == eTimeZone &&
               strncasecmp(calbuf.c_str(), "END:VTIMEZONE",
                           sizeof("END:VTIMEZONE") - 1) == 0) {
      m_phase = eCal;

      _calendar.time_zones.push_back(_time_zone);
    } else if (m_phase == eCal &&
               strncasecmp(calbuf.c_str(), "BEGIN:VEVENT",
                           sizeof("BEGIN:VEVENT") - 1) == 0) {
      m_phase = eEvent;

      _event.created = 0;
      _event.modified = 0;
      _event.stamp = 0;

      _event.uid = "";
      _event.summary = "";

      _event.orgnizer.rsvp = true;
      _event.orgnizer.name = "";
      _event.orgnizer.role = "";
      _event.orgnizer.mailto = "";

      _event.attendees.clear();

      _event.start.tid = "";
      _event.start.dt = 0;

      _event.end.tid = "";
      _event.end.dt = 0;

      _event.location = "";
      _event.description = "";
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "CREATED:",
                                                sizeof("CREATED:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "CREATED:", " \t\r\n", " \t\r\n", strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02dZ", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _event.created = mktime(&dtTmp);
    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(), "LAST-MODIFIED:",
                           sizeof("LAST-MODIFIED:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "LAST-MODIFIED:", " \t\r\n", " \t\r\n",
                  strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02dZ", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _event.modified = mktime(&dtTmp);

    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "DTSTAMP:",
                                                sizeof("DTSTAMP:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "DTSTAMP:", " \t\r\n", " \t\r\n", strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02dZ", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _event.stamp = mktime(&dtTmp);

    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(), "UID:", sizeof("UID:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "UID:", " \t\r\n", " \t\r\n", _event.uid);
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "SUMMARY:",
                                                sizeof("SUMMARY:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "UID:", " \t\r\n", " \t\r\n", _event.summary);
    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(), "ORGANIZER;",
                           sizeof("ORGANIZER;") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "RSVP=", " \t\r\n", "; \t\r\n",
                  _event.orgnizer.rsvp);

      fnfy_strcut(calbuf.c_str(), "CN=", " \t\r\n", "; \t\r\n",
                  _event.orgnizer.name);

      fnfy_strcut(calbuf.c_str(), "PARTSTAT=", " \t\r\n", "; \t\r\n",
                  _event.orgnizer.partstat);

      fnfy_strcut(calbuf.c_str(), "ROLE=", " \t\r\n", ":; \t\r\n",
                  _event.orgnizer.role);

      fnfy_strcut(calbuf.c_str(), "mailto:", " \t\r\n", "; \t\r\n",
                  _event.orgnizer.mailto);

    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "ATTENDEE;",
                                                sizeof("ATTENDEE;") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "RSVP=", " \t\r\n", "; \t\r\n",
                  _attendee.rsvp);

      fnfy_strcut(calbuf.c_str(), "CN=", " \t\r\n", "; \t\r\n", _attendee.name);

      fnfy_strcut(calbuf.c_str(), "PARTSTAT=", " \t\r\n", "; \t\r\n",
                  _attendee.partstat);

      fnfy_strcut(calbuf.c_str(), "ROLE=", " \t\r\n", ":; \t\r\n",
                  _attendee.role);

      fnfy_strcut(calbuf.c_str(), "mailto:", " \t\r\n", "; \t\r\n",
                  _attendee.mailto);

      _event.attendees.push_back(_attendee);
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "DTSTART;",
                                                sizeof("DTSTART;") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), ":", " \t\r\n", " \t\r\n", strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02d", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _event.start.dt = mktime(&dtTmp);

      fnfy_strcut(calbuf.c_str(), "TZID=", " \t\r\n", ": \t\r\n", strTmp);
      _event.start.tid = strTmp;
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "DTEND;",
                                                sizeof("DTEND;") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), ":", " \t\r\n", " \t\r\n", strTmp);

      struct tm dtTmp;

      sscanf(strTmp.c_str(), "%04d%02d%02dT%02d%02d%02d", &dtTmp.tm_year,
             &dtTmp.tm_mon, &dtTmp.tm_mday, &dtTmp.tm_hour, &dtTmp.tm_min,
             &dtTmp.tm_sec);
      dtTmp.tm_year -= 1900;
      dtTmp.tm_mon -= 1;

      _event.end.dt = mktime(&dtTmp);

      fnfy_strcut(calbuf.c_str(), "TZID=", " \t\r\n", ": \t\r\n", strTmp);
      _event.end.tid = strTmp;
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "LOCATION:",
                                                sizeof("LOCATION:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "LOCATION:", " \t\r\n", " \t\r\n",
                  _event.location);
    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(),
                           "DESCRIPTION:", sizeof("DESCRIPTION:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "DESCRIPTION:", " \t\r\n", " \t\r\n",
                  _event.description);
    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(), "TRANSP:", sizeof("TRANSP:") - 1) ==
                   0) {
      fnfy_strcut(calbuf.c_str(), "TRANSP:", " \t\r\n", " \t\r\n",
                  _event.transp);
    } else if (m_phase == eEvent && strncasecmp(calbuf.c_str(), "SEQUENCE:",
                                                sizeof("SEQUENCE:") - 1) == 0) {
      fnfy_strcut(calbuf.c_str(), "SEQUENCE:", " \t\r\n", " \t\r\n", strTmp);
      sscanf(strTmp.c_str(), "%lu", &_event.sequence);
    } else if (m_phase == eEvent &&
               strncasecmp(calbuf.c_str(), "END:VEVENT",
                           sizeof("END:VEVENT") - 1) == 0) {
      _calendar.events.push_back(_event);
      m_phase = eCal;
    } else if (m_phase == eCal &&
               strncasecmp(calbuf.c_str(), "END:VCALENDAR",
                           sizeof("END:VCALENDAR") - 1) == 0) {
      m_phase = eNull;
      m_details.push_back(_calendar);
    }
  }
}

void Calendar::parse(const char* text) {
  if (text[0] == ' ' || text[0] == '\t') {
    calbuf += text;
  } else {
    _parse_();

    calbuf = text;
  }
}

void Calendar::save() {
  flush();

  char calpath[1024];
  struct tm* tmTmp;
  for (int i = 0; i < m_details.size(); i++) {
    for (int x = 0; x < m_details[i].events.size(); x++) {
      tmTmp = gmtime(&m_details[i].events[x].start.dt);
      int a = sprintf(calpath, "%s/cal/%s", CMailBase::m_private_path.c_str(),
                      m_owner.c_str());
      mkdir(calpath, 0744);

      int b = sprintf(calpath + a, "/%d%02d%02d", tmTmp->tm_year + 1900,
                      tmTmp->tm_mon + 1, tmTmp->tm_mday);
      mkdir(calpath, 0744);

      sprintf(calpath + a + b, "/%08x_%08x_%016lx_%08x.cal", time(NULL),
              getpid(), pthread_self(), random());

      m_file = fopen(calpath, "wb+");
      if (m_file) {
        fwrite(m_details[i].text.c_str(), m_details[i].text.length(), 1,
               m_file);
        fclose(m_file);
      }
    }
  }
}

void Calendar::load(const char* path) {
  FILE* pfile;
  pfile = fopen(path, "rb");

  char buf[4096];
  if (pfile) {
    while (feof(pfile) == 0) {
      fgets(buf, 4096, pfile);
      parse(buf);
    }
    flush();
    fclose(pfile);
  }
}

void Calendar::flush() {
  _parse_();
  calbuf = "";
}
