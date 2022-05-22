//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __EVENTUTILS_H__
#define __EVENTUTILS_H__


namespace Ms {

class XmlWriter;

//---------------------------------------------------------
//   EventList
//   EventMap
//---------------------------------------------------------

class EventList : public QList<Event> {
   public:
      void insert(const Event&);
      void insertNote(int channel, Note*);
      };

class EventMap : public std::multimap<int, NPlayEvent> {
      int _highestChannel = 15;
   public:
      int highestChannel() const { return _highestChannel; }
      void registerChannel(int c) { if (c > _highestChannel) _highestChannel = c; }
      };

typedef EventList::iterator iEvent;
typedef EventList::const_iterator ciEvent;

void midiCoreEvent_write(MidiCoreEvent ev, XmlWriter& xml);
void event_write(Event ev, XmlWriter& xml);
bool nPlayEvent_isMuted(NPlayEvent ev);
void eventMap_fixupMIDI(EventMap& e);

}
#endif

