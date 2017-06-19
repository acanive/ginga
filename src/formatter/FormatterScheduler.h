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

#ifndef FORMATTER_SCHEDULER_H
#define FORMATTER_SCHEDULER_H

#include "PlayerAdapter.h"
#include "Converter.h"
#include "FocusManager.h"
#include "NclLinkAssignmentAction.h"
#include "RuleAdapter.h"

#include "ncl/NclDocument.h"
using namespace ::ginga::ncl;

GINGA_FORMATTER_BEGIN

class Converter;

class FormatterScheduler : public INclLinkActionListener,
                           public INclEventListener
{
private:
  map<string, PlayerAdapter *> _objectPlayers;
  Converter *compiler;
  FocusManager *focusManager;
  Settings *settings;
  RuleAdapter *ruleAdapter;

  string file;                        // NCL file path
  NclDocument *doc;                   // NCL document tree
  vector<NclFormatterEvent *> events; // document events
  set<NclLinkSimpleAction *> actions; // document actions

public:
  FormatterScheduler ();
  virtual ~FormatterScheduler ();

  void addAction (NclLinkSimpleAction *) override;
  void removeAction (NclLinkSimpleAction *) override;
  void scheduleAction (NclLinkSimpleAction *) override;

  bool setKeyHandler (bool isHandler);
  FocusManager *getFocusManager ();

  void startEvent (NclFormatterEvent *event);
  void stopEvent (NclFormatterEvent *event);
  void pauseEvent (NclFormatterEvent *event);
  void resumeEvent (NclFormatterEvent *event);

  void startDocument (const string &);

  void eventStateChanged (NclFormatterEvent *someEvent,
                          EventStateTransition transition,
                          EventUtil::EventState previousState) override;

  PlayerAdapter *getObjectPlayer (ExecutionObject *execObj);
  bool removePlayer (ExecutionObject *object);

private:
  PlayerAdapter *initializePlayer (ExecutionObject *object);

  void runAction (NclLinkSimpleAction *action);
  void runAction (NclFormatterEvent *event, NclLinkSimpleAction *action);
  void runActionOverProperty (NclFormatterEvent *event,
                              NclLinkSimpleAction *action);

  void runActionOverApplicationObject (
      ExecutionObjectApplication *executionObject,
      NclFormatterEvent *event, PlayerAdapter *player,
      NclLinkSimpleAction *action);

  void
  runActionOverComposition (ExecutionObjectContext *compositeObject,
                            NclLinkSimpleAction *action);

  void runActionOverSwitch (ExecutionObjectSwitch *switchObject,
                            NclSwitchEvent *event,
                            NclLinkSimpleAction *action);

  void runSwitchEvent (ExecutionObjectSwitch *switchObject,
                       NclSwitchEvent *switchEvent,
                       ExecutionObject *selectedObject,
                       NclLinkSimpleAction *action);

  string solveImplicitRefAssessment (const string &propValue,
                                     NclAttributionEvent *event);

  SDLWindow *prepareFormatterRegion (ExecutionObject *);
  void showObject (ExecutionObject *);
  void hideObject (ExecutionObject *);
};

GINGA_FORMATTER_END

#endif // FORMATTER_SCHEDULER_H
