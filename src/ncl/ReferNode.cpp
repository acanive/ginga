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
#include "ReferNode.h"

#include "ContentNode.h"

GINGA_NCL_BEGIN

ReferNode::ReferNode (const string &id) : Node (id)
{
  _instanceType = "new";
  _referredNode = NULL;
}

ReferNode::ReferNode (const string &id, Entity *entity) : Node (id)
{
  _instanceType = "new";

  setReferredEntity (entity);
}

string
ReferNode::getInstanceType ()
{
  return _instanceType;
}

void
ReferNode::setInstanceType (const string &instance)
{
  this->_instanceType = instance;
}

Entity *
ReferNode::getReferredEntity ()
{
  return _referredNode;
}

void
ReferNode::setReferredEntity (Entity *entity)
{
  _referredNode = entity;
  ((ContentNode *)_referredNode)->addSameInstance (this);
}

GINGA_NCL_END
