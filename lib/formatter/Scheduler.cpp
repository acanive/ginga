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

#include "ginga-internal.h"
#include "Scheduler.h"
#include "Converter.h"

#include "mb/Display.h"
using namespace ::ginga::mb;

GINGA_FORMATTER_BEGIN


// Public.

Scheduler::Scheduler ()
{
  _converter = new Converter (new RuleAdapter (new Settings ()));
  _converter->setLinkActionListener (this);
}

Scheduler::~Scheduler ()
{
  _events.clear ();
  delete _converter;
}

void
Scheduler::scheduleAction (NclSimpleAction *action)
{
  runAction (action->getEvent (), action);
}

void
Scheduler::startDocument (const string &file)
{
  string id;
  Context *body;
  const vector<Port *> *ports;
  vector<NclEvent *> *entryevts;
  NclNodeNesting *persp;
  int w, h;

  // Parse document.
  Ginga_Display->getSize (&w, &h);
  _doc = Parser::parse (file, w, h);
  g_assert_nonnull (_doc);

  id = _doc->getId ();
  body = _doc->getBody ();
  if (unlikely (body == nullptr))
    ERROR_SYNTAX ("document has no body");

  // Get entry events (i.e., those mapped by ports).
  ports = body->getPorts ();
  if (unlikely (ports == nullptr))
    ERROR ("document has no ports");

  persp = new NclNodeNesting ();
  persp->insertAnchorNode (body);
  entryevts = new vector<NclEvent *>;
  for (auto port: *ports)
    {
      NclEvent *evt = _converter->insertContext (persp, port);
      g_assert_nonnull (evt);
      entryevts->push_back (evt);
    }
  delete persp;

  if (unlikely (entryevts->empty ()))
    {
      WARNING ("document has no ports");
      return;
    }

  // Create execution object for settings node and initialize it.
  vector <Node *> *settings = _doc->getSettingsNodes ();
  for (auto node: *settings)
    {
      Media *content;
      ExecutionObject *execobj;

      persp = new NclNodeNesting (node->getPerspective ());
      execobj = _converter->getExecutionObjectFromPerspective (persp, nullptr);
      g_assert_nonnull (execobj);

      TRACE ("processing '%s'", persp->getId ().c_str ());
      delete persp;

      content = (Media *) node;
      for (auto anchor: *content->getAnchors ())
        {
          Property *prop;
          string name;
          string value;

          if (!instanceof (Property *, anchor))
            continue;           // nothing to do

          prop = cast (Property *, anchor);
          name = prop->getName ();
          value = prop->getValue ();
          if (value == "")
            continue;           // nothing to do

          TRACE ("settings: %s='%s'", name.c_str (), value.c_str ());
          if (name == "service.currentFocus")
            Player::setCurrentFocus (value);
        }
    }
  delete settings;

  // Start entry events.
  for (auto event: *entryevts)
    {
      NclSimpleAction *fakeAction;
      _events.push_back (event);
      fakeAction = new NclSimpleAction (event, SimpleAction::START);
      runAction (event, fakeAction);
      delete fakeAction;
    }
  delete entryevts;

  ExecutionObject::refreshCurrentFocus ();
}


// Private.

void
Scheduler::runAction (NclEvent *event, NclSimpleAction *action)
{
  ExecutionObject *obj;

  obj = event->getExecutionObject ();
  g_assert_nonnull (obj);

  TRACE ("running action '%d' over event '%s' (object '%s')",
         action->getType (),
         event->getId ().c_str (),
         obj->getId ().c_str ());

  if (instanceof (SelectionEvent *, event))
    {
      event->start ();
      delete action;
      return;
    }

  if (instanceof (ExecutionObjectSwitch *, obj)
      && instanceof (SwitchEvent *, event))
    {
      this->runActionOverSwitch ((ExecutionObjectSwitch *) obj,
                                 (SwitchEvent *) event, action);
      return;
    }

  if (instanceof (ExecutionObjectContext *, obj))
    {
      this->runActionOverComposition
        ((ExecutionObjectContext *) obj, action);
      return;
    }

  if (instanceof (AttributionEvent *, event))
    {
      AttributionEvent *attevt;
      NclAssignmentAction *attact;
      Property *property;

      string name;
      string from;
      string to;

      GingaTime dur;

      g_assert (instanceof (NclAssignmentAction *, action));
      g_assert (action->getType () == SimpleAction::START);

      attevt = (AttributionEvent *) event;
      attact = (NclAssignmentAction *) action;

      if (event->getCurrentState () != EventState::SLEEPING)
        return;                 // nothing to do

      property = attevt->getAnchor ();
      g_assert_nonnull (property);

      name = property->getName ();
      from = property->getValue ();
      to = attevt->solveImplicitRefAssessment (attact->getValue ());

      string s;
      s = attevt->solveImplicitRefAssessment (attact->getDuration ());
      dur = ginga_parse_time (s);

      attevt->start ();
      attevt->setValue (to);
      obj->setProperty (name, from, to, dur);

      // TODO: Wrap this in a closure to be called at the end of animation.
      attevt->stop ();
      return;
    }

  switch (action->getType ())
    {
    case SimpleAction::START:
      obj->prepare (event);
      g_assert (obj->start ());
      break;
    case SimpleAction::STOP:
      g_assert (obj->stop ());
      break;
    case SimpleAction::PAUSE:
      g_assert (obj->pause ());
      break;
    case SimpleAction::RESUME:
      g_assert (obj->resume ());
      break;
    case SimpleAction::ABORT:
      g_assert (obj->abort ());
      break;
    default:
      g_assert_not_reached ();
    }
}

void
Scheduler::runActionOverComposition (ExecutionObjectContext *ctxObj,
                                     NclSimpleAction *action)
{
  NclEvent *event;
  EventType type;
  SimpleAction::Type acttype;

  Node *node;
  Entity *entity;
  Composition *compNode;
  NclNodeNesting *compPerspective;

  event = action->getEvent ();
  g_assert_nonnull (event);

  type = event->getType ();

  if (type == EventType::ATTRIBUTION)
    {
      ERROR_NOT_IMPLEMENTED
        ("context property attribution is not supported");
    }

  if (type == EventType::SELECTION)
    {
      WARNING ("trying to select composition '%s'",
               ctxObj->getId ().c_str ());
      return;
    }

  node = ctxObj->getNode ();
  g_assert_nonnull (node);

  entity = cast (Entity *, node);
  g_assert_nonnull (entity);

  compNode = cast (Composition *, entity);
  g_assert_nonnull (compNode);

  if (compNode->getParent () == nullptr)
    {
      compPerspective
        = new NclNodeNesting (compNode->getPerspective ());
    }
  else
    {
      compPerspective
        = ctxObj->getNodePerspective ();
    }

  acttype = action->getType ();
  if (acttype == SimpleAction::START)     // start all ports
    {
      ctxObj->suspendLinkEvaluation (false);
      for (auto port: *compNode->getPorts ())
        {
          NclNodeNesting *persp;
          vector<Node *> nestedSeq;
          Anchor *iface;

          ExecutionObject *child;
          NclEvent *evt;

          persp = compPerspective->copy ();
          g_assert_nonnull (persp);

          nestedSeq = port->getMapNodeNesting ();
          persp->append (&nestedSeq);

          // Create or get the execution object mapped by port.
          child = _converter
            ->getExecutionObjectFromPerspective (persp, nullptr);
          g_assert (child);

          iface = port->getFinalInterface ();
          g_assert_nonnull (iface);

          if (!instanceof (Area *, iface))
            continue;           // nothing to do

          evt = _converter->getEvent (child, iface,
                                      EventType::PRESENTATION, "");
          g_assert_nonnull (evt);
          g_assert (instanceof (PresentationEvent *, evt));

          runAction (evt, action);

          delete persp;
        }
    }
  else if (acttype == SimpleAction::STOP) // stop all children
    {
      ctxObj->suspendLinkEvaluation (true);
      for (const auto pair: *ctxObj->getExecutionObjects ())
        {
          ExecutionObject *child;
          NclEvent *evt;

          child = pair.second;
          evt = child->getMainEvent ();
          if (evt == nullptr)
            evt = child->getWholeContentPresentationEvent ();
          g_assert_nonnull (evt);
          runAction (evt, action);
        }
      ctxObj->suspendLinkEvaluation (false);
    }
  else if (acttype == SimpleAction::ABORT)
    {
      ERROR_NOT_IMPLEMENTED ("action 'abort' is not supported");
    }
  else if (acttype == SimpleAction::PAUSE)
    {
      ERROR_NOT_IMPLEMENTED ("action 'pause' is not supported");
    }
  else if (acttype == SimpleAction::RESUME)
    {
      ERROR_NOT_IMPLEMENTED ("action 'resume' is not supported");
    }
  else
    {
      g_assert_not_reached ();
    }
  delete compPerspective;
}

void
Scheduler::runActionOverSwitch (ExecutionObjectSwitch *switchObj,
                                SwitchEvent *event,
                                NclSimpleAction *action)
{
  ExecutionObject *selectedObject;
  NclEvent *selectedEvent;

  selectedObject = switchObj->getSelectedObject ();
  if (selectedObject == nullptr)
    {
      selectedObject = _converter->processExecutionObjectSwitch (switchObj);

      if (selectedObject == nullptr)
        {
          return;
        }
    }

  selectedEvent = event->getMappedEvent ();
  if (selectedEvent != nullptr)
    {
      runAction (selectedEvent, action);
    }
  else
    {
      runSwitchEvent (switchObj, event, selectedObject, action);
    }

  if (action->getType () == SimpleAction::STOP
      || action->getType () == SimpleAction::ABORT)
    {
      switchObj->select (nullptr);
    }
}

void
Scheduler::runSwitchEvent (ExecutionObjectSwitch *switchObj,
                                    SwitchEvent *switchEvent,
                                    ExecutionObject *selectedObject,
                                    NclSimpleAction *action)
{
  NclEvent *selectedEvent;
  SwitchPort *switchPort;
  vector<Port *>::iterator i;
  NclNodeNesting *nodePerspective;
  vector<Node *> nestedSeq;
  ExecutionObject *endPointObject;

  selectedEvent = nullptr;
  switchPort = (SwitchPort *)(switchEvent->getInterface ());
  for (auto mapping: *switchPort->getPorts ())
    {
      if (mapping->getNode () == selectedObject->getNode ())
        {
          nodePerspective = switchObj->getNodePerspective ();
          nestedSeq = mapping->getMapNodeNesting ();
          nodePerspective->append (&nestedSeq);
          try
            {
              endPointObject
                = _converter
                ->getExecutionObjectFromPerspective (
                                                     nodePerspective, nullptr);

              if (endPointObject != nullptr)
                {
                  selectedEvent
                    = _converter
                    ->getEvent (
                                endPointObject,
                                mapping->getFinalInterface (),
                                switchEvent->getType (),
                                switchEvent->getKey ());
                }
            }
          catch (exception *exc)
            {
              // continue
            }

          break;
        }
    }

  if (selectedEvent != nullptr)
    {
      switchEvent->setMappedEvent (selectedEvent);
      runAction (selectedEvent, action);
    }
}

GINGA_FORMATTER_END