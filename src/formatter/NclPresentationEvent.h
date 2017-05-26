/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef PRESENTATION_EVENT_H
#define PRESENTATION_EVENT_H

#include "ncl/ContentAnchor.h"
using namespace ::ginga::ncl;

#include "NclAnchorEvent.h"

GINGA_FORMATTER_BEGIN

class NclPresentationEvent : public NclAnchorEvent
{
public:
  static const double UNDEFINED_INSTANT;

private:
  double begin;
  double end;
  double duration;
  int numPresentations;
  double repetitionInterval;

public:
  NclPresentationEvent (const string &id,
                        NclExecutionObject *executionObject,
                        ContentAnchor *anchor);
  virtual ~NclPresentationEvent ();
  bool stop ();
  double getDuration ();
  double getRepetitionInterval ();
  int getRepetitions ();
  void setDuration (double dur);
  void setEnd (double e);
  void setRepetitionSettings (int repetitions, double repetitionInterval);
  double getBegin ();
  double getEnd ();
  void incrementOccurrences ();
  static bool isUndefinedInstant (double value);
};

GINGA_FORMATTER_END

#endif /* PRESENTATION_EVENT_H */
