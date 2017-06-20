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

#include "ginga.h"
#include "FormatterEvents.h"

#include "ExecutionObjectContext.h"
#include "ncl/ContentNode.h"

GINGA_FORMATTER_BEGIN

set<NclFormatterEvent *> NclFormatterEvent::_instances;

NclFormatterEvent::NclFormatterEvent (const string &id,
                                      ExecutionObject *exeObj)
{
  _typeSet.insert ("NclFormatterEvent");

  this->_id = id;
  _state = EventState::SLEEPING;
  _occurrences = 0;
  _exeObj = exeObj;
  _type = EventType::UNKNOWN;

  _instances.insert (this);
}

NclFormatterEvent::~NclFormatterEvent ()
{
  auto i = _instances.find (this);
  if (i != _instances.end ())
    {
      _instances.erase (i);
    }

  _listeners.clear ();
}

bool
NclFormatterEvent::hasInstance (NclFormatterEvent *evt, bool remove)
{
  bool inst = false;

  auto i = _instances.find (evt);
  if (i != _instances.end ())
    {
      if (remove)
        {
          _instances.erase (i);
        }
      inst = true;
    }
  return inst;
}

bool
NclFormatterEvent::instanceOf (const string &s)
{
  if (_typeSet.empty ())
    {
      return false;
    }
  else
    {
      return (_typeSet.find (s) != _typeSet.end ());
    }
}

bool
NclFormatterEvent::hasNcmId (NclFormatterEvent *evt, const string &anchorId)
{
  Anchor *anchor;
  string anchorName = " ";

  if (auto anchorEvt = dynamic_cast<NclAnchorEvent *> (evt))
    {
      anchor = anchorEvt->getAnchor ();
      if (anchor != nullptr)
        {
          if (dynamic_cast<IntervalAnchor *> (anchor))
            {
              anchorName = anchor->getId ();
            }
          else if (auto labeledAnchor = dynamic_cast<LabeledAnchor *> (anchor))
            {
              anchorName = labeledAnchor->getLabel ();
            }
          else if (dynamic_cast<LambdaAnchor *> (anchor))
            {
              anchorName = "";
            }

          if (anchorName == anchorId
              && !(dynamic_cast<NclSelectionEvent *> (evt)))
            {
              return true;
            }
        }
    }
  else if (auto attrEvt = dynamic_cast<NclAttributionEvent *> (evt))
    {
      anchor = attrEvt->getAnchor ();
      if (anchor != nullptr)
        {
          auto propAnchor = dynamic_cast<PropertyAnchor *> (anchor);
          g_assert_nonnull (propAnchor);
          anchorName = propAnchor->getName ();
          if (anchorName == anchorId)
            {
              return true;
            }
        }
    }

  return false;
}

void
NclFormatterEvent::addListener (INclEventListener *listener)
{
  this->_listeners.insert (listener);
}

void
NclFormatterEvent::removeListener (INclEventListener *listener)
{
  auto i = _listeners.find (listener);
  if (i != _listeners.end ())
    {
      _listeners.erase (i);
    }
}

EventStateTransition
NclFormatterEvent::getTransition (EventState newState)
{
  return EventUtil::getTransition (_state, newState);
}

bool
NclFormatterEvent::abort ()
{
  if (_state == EventState::OCCURRING || _state == EventState::PAUSED)
    return changeState (EventState::SLEEPING, EventStateTransition::ABORTS);
  else
    return false;
}

bool
NclFormatterEvent::start ()
{
  if (_state == EventState::SLEEPING)
      return changeState (EventState::OCCURRING, EventStateTransition::STARTS);
  else
    return false;
}

bool
NclFormatterEvent::stop ()
{
  if (_state == EventState::OCCURRING || _state == EventState::PAUSED)
    return changeState (EventState::SLEEPING, EventStateTransition::STOPS);
  else
    return false;
}

bool
NclFormatterEvent::pause ()
{
  if (_state == EventState::OCCURRING)
    return changeState (EventState::PAUSED, EventStateTransition::PAUSES);
  else
    return false;
}

bool
NclFormatterEvent::resume ()
{
  if (_state == EventState::PAUSED)
    return changeState (EventState::OCCURRING, EventStateTransition::RESUMES);
  else
    return false;
}

void
NclFormatterEvent::setCurrentState (EventState newState)
{
  _previousState = _state;
  _state = newState;
}

bool
NclFormatterEvent::changeState (EventState newState,
                                EventStateTransition transition)
{
  if (transition == EventStateTransition::STOPS)
    {
      _occurrences++;
    }

  _previousState = _state;
  _state = newState;

  set<INclEventListener *> clone (_listeners);

  for (INclEventListener *listener: clone)
    {
      listener->eventStateChanged (this, transition, _previousState);
    }

  return true;
}

// NclAnchorEvent
NclAnchorEvent::NclAnchorEvent (const string &id,
                                ExecutionObject *executionObject,
                                ContentAnchor *anchor)
    : NclFormatterEvent (id, executionObject)
{
  this->_anchor = anchor;
  _typeSet.insert ("NclAnchorEvent");
}

ContentAnchor *
NclAnchorEvent::getAnchor ()
{
  return _anchor;
}

// NclPresentationEvent
NclPresentationEvent::NclPresentationEvent (const string &id,
                                            ExecutionObject *exeObj,
                                            ContentAnchor *anchor)
    : NclAnchorEvent (id, exeObj, anchor)
{
  _typeSet.insert ("NclPresentationEvent");

  _numPresentations = 1;
  _repetitionInterval = 0;

  if (anchor->instanceOf ("IntervalAnchor"))
    {
      _begin = ((IntervalAnchor *)anchor)->getBegin ();
      _end = ((IntervalAnchor *)anchor)->getEnd ();
    }
  else
    {
      _begin = 0;
      _end = GINGA_TIME_NONE;
    }
}

bool
NclPresentationEvent::stop ()
{
  if (_state == EventState::OCCURRING && _numPresentations > 1)
    {
      _numPresentations--;
    }

  return NclFormatterEvent::stop ();
}

GingaTime
NclPresentationEvent::getDuration ()
{
  if (!GINGA_TIME_IS_VALID (this->_end))
    return GINGA_TIME_NONE;
  return this->_end - this->_begin;
}

GingaTime
NclPresentationEvent::getRepetitionInterval ()
{
  return _repetitionInterval;
}

int
NclPresentationEvent::getRepetitions ()
{
  return (_numPresentations - 1);
}

void
NclPresentationEvent::setEnd (GingaTime end)
{
  this->_end = end;
}

void
NclPresentationEvent::setRepetitionSettings (int repetitions,
                                             GingaTime repetitionInterval)
{
  if (repetitions >= 0)
    {
      this->_numPresentations = repetitions + 1;
    }
  else
    {
      this->_numPresentations = 1;
    }

  this->_repetitionInterval = repetitionInterval;
}

GingaTime
NclPresentationEvent::getBegin ()
{
  return _begin;
}

GingaTime
NclPresentationEvent::getEnd ()
{
  return _end;
}

void
NclPresentationEvent::incrementOccurrences ()
{
  _occurrences++;
}

// NclSelectionEvent
NclSelectionEvent::NclSelectionEvent (const string &id,
                                      ExecutionObject *exeObj,
                                      ContentAnchor *anchor)
    : NclAnchorEvent (id, exeObj, anchor)
{
  selectionCode.assign("NO_CODE");

  _typeSet.insert ("NclSelectionEvent");
}

const string
NclSelectionEvent::getSelectionCode ()
{
  return selectionCode;
}

bool
NclSelectionEvent::start ()
{
  if (NclAnchorEvent::start ())
    return NclAnchorEvent::stop ();
  else
    return false;
}

void
NclSelectionEvent::setSelectionCode (const string &codeStr)
{
   selectionCode = codeStr;
}

// NclAttributionEvent
NclAttributionEvent::NclAttributionEvent (const string &id,
                                          ExecutionObject *exeObj,
                                          PropertyAnchor *anchor,
                                          Settings *settings)
    : NclFormatterEvent (id, exeObj)
{
  Entity *entity;
  NodeEntity *dataObject;

  _typeSet.insert ("NclAttributionEvent");

  this->anchor = anchor;
  this->valueMaintainer = nullptr;
  this->settingNode = false;
  this->settings = settings;

  dataObject = (NodeEntity *)(exeObj->getDataObject ());

  if (dataObject->instanceOf ("ContentNode")
      && ((ContentNode *)dataObject)->isSettingNode ())
    {
      settingNode = true;
    }

  if (dataObject->instanceOf ("ReferNode"))
    {
      if (((ReferNode *)dataObject)->getInstanceType () == "instSame")
        {
          entity = ((ReferNode *)dataObject)->getDataEntity ();
          if (entity->instanceOf ("ContentNode")
              && ((ContentNode *)entity)->isSettingNode ())
            {
              settingNode = true;
            }
        }
    }
}

NclAttributionEvent::~NclAttributionEvent ()
{
  assessments.clear ();
}

PropertyAnchor *
NclAttributionEvent::getAnchor ()
{
  return anchor;
}

string
NclAttributionEvent::getCurrentValue ()
{
  string propName;
  string maintainerValue = "";

  if (unlikely (anchor == nullptr))
    {
      ERROR ("trying to set a nullptr property anchor of object '%s'",
             _id.c_str ());
    }

  if (settingNode)
    {
      propName = anchor->getName ();
      if (propName != "")
        {
          maintainerValue = settings->get (propName);
        }
    }
  else
    {
      if (valueMaintainer != nullptr)
        {
          maintainerValue = valueMaintainer->getProperty (this);
        }

      if (maintainerValue == "")
        {
          maintainerValue = anchor->getValue ();
        }
    }

  return maintainerValue;
}

bool
NclAttributionEvent::setValue (const string &newValue)
{
  if (anchor->getValue () != newValue)
    {
      anchor->setValue (newValue);
      return true;
    }
  return false;
}

void
NclAttributionEvent::setValueMaintainer (
    INclAttributeValueMaintainer *valueMaintainer)
{
  this->valueMaintainer = valueMaintainer;
}

INclAttributeValueMaintainer *
NclAttributionEvent::getValueMaintainer ()
{
  return this->valueMaintainer;
}

void
NclAttributionEvent::setImplicitRefAssessmentEvent (
    const string &roleId, NclFormatterEvent *event)
{
  assessments[roleId] = event;
}

NclFormatterEvent *
NclAttributionEvent::getImplicitRefAssessmentEvent (const string &roleId)
{
  if (assessments.count (roleId) == 0)
    {
      return nullptr;
    }

  return assessments[roleId];
}

// NclSwitchEvent
NclSwitchEvent::NclSwitchEvent (const string &id,
                                ExecutionObject *executionObjectSwitch,
                                InterfacePoint *interfacePoint,
                                EventType type, const string &key)
    : NclFormatterEvent (id, executionObjectSwitch)
{
  this->interfacePoint = interfacePoint;
  this->_type = type;
  this->key = key;
  this->mappedEvent = nullptr;

  _typeSet.insert ("NclSwitchEvent");
}

NclSwitchEvent::~NclSwitchEvent ()
{
  if (NclFormatterEvent::hasInstance (mappedEvent, false))
    {
      mappedEvent->removeListener (this);
      mappedEvent = nullptr;
    }
}

InterfacePoint *
NclSwitchEvent::getInterfacePoint ()
{
  return interfacePoint;
}

string
NclSwitchEvent::getKey ()
{
  return key;
}

void
NclSwitchEvent::setMappedEvent (NclFormatterEvent *event)
{
  if (mappedEvent != nullptr)
    {
      mappedEvent->removeListener (this);
    }

  mappedEvent = event;
  if (mappedEvent != nullptr)
    {
      mappedEvent->addListener (this);
    }
}

NclFormatterEvent *
NclSwitchEvent::getMappedEvent ()
{
  return mappedEvent;
}

void
NclSwitchEvent::eventStateChanged (
    arg_unused (NclFormatterEvent *someEvent),
    EventStateTransition transition,
    arg_unused (EventState _previousState))
{
  changeState (EventUtil::getNextState (transition), transition);
}

GINGA_FORMATTER_END
