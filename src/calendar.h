#ifndef _CALENDER_H_
#define _CALENDER_H_

#include "util/general.h"
/*

BEGIN:VCALENDAR
PRODID:-//Mozilla.org/NONSGML Mozilla Calendar V1.1//EN
VERSION:2.0
METHOD:REQUEST
BEGIN:VTIMEZONE
TZID:Asia/Shanghai
X-LIC-LOCATION:Asia/Shanghai
BEGIN:STANDARD
TZOFFSETFROM:+0800
TZOFFSETTO:+0800
TZNAME:CST
DTSTART:19700101T000000
END:STANDARD
END:VTIMEZONE
BEGIN:VEVENT
CREATED:20120320T172338Z
LAST-MODIFIED:20120321T114955Z
DTSTAMP:20120321T114955Z
UID:34a2250f-19fd-4669-81f7-0e0ec92e2139
SUMMARY:hello world
ORGANIZER;RSVP=TRUE;CN=Sheng Bin;PARTSTAT=ACCEPTED;ROLE=CHAIR:mailto:bshen
 g@erisesoft.com
ATTENDEE;RSVP=TRUE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT:mailto:ylz@e
 risesoft.com
ATTENDEE;RSVP=TRUE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT:mailto:admin
 @erisesoft.com
DTSTART;TZID=Asia/Shanghai:20120321T120000
DTEND;TZID=Asia/Shanghai:20120321T130000
LOCATION:hi
DESCRIPTION:hi
TRANSP:OPAQUE
SEQUENCE:13
END:VEVENT
END:VCALENDAR

*/


typedef enum
{
	eNull = 0,
	eCal,
	eTimeZone,
	eStandard,
	eEvent
} ePhase;


typedef struct 
{
	string rsvp;
	string name;
	string partstat;
	string role;
	string mailto;
}vAttendee;

typedef struct
{
	int offsetfr;
	int offsetto;
	string tzname;
	time_t dtstart;
}vStandard;

typedef struct
{
	string tid;
	string xlic_location;
	vector<vStandard> standards;
}vTimeZone;

typedef struct
{
	time_t dt;
	string tid;
}vDateTime;

typedef struct
{
	time_t created;
	time_t modified;
	time_t stamp;
	string uid;
	string summary;
	vAttendee orgnizer;
	vector<vAttendee> attendees;
	vDateTime start;
	vDateTime end;
	string location;
	string description;
	string transp;
	unsigned int sequence;
}vEvent;


typedef struct 
{
	string prod;
	string version;
	string method;
	vector<vTimeZone> time_zones;
	vector<vEvent> events;
	string text; 	/* Origitial text*/
}vCalendar;

class Calendar
{
public:
	Calendar(const char* owner);
	virtual ~Calendar();

	void parse(const char* text);

	void save();
	void flush();
	void load(const char* path);
	
	vector<vCalendar> m_details;

	string m_strJSON;

private:
	void _parse_();
	FILE* m_file;
	ePhase m_phase;
	string m_owner;
	string calbuf;
	
	vCalendar _calendar;
	vTimeZone _time_zone;
	vEvent _event;
	vAttendee  _attendee;
	vStandard _standard;

	string _text;
};
#endif /* _CALENDER_H_ */

