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

#include "aux-ginga.h"
#include "FormatterComposition.h"

GINGA_NAMESPACE_BEGIN

FormatterComposition::FormatterComposition (const string &id)
  :FormatterObject (id)
{
}

FormatterComposition::~FormatterComposition ()
{
}

const set<FormatterObject *> *
FormatterComposition::getChildren ()
{
  return &_children;
}

FormatterObject *
FormatterComposition::getChildById (const string &id)
{
  for (auto child: _children)
    if (child->getId () == id)
      return child;
  return nullptr;
}

void
FormatterComposition::addChild (FormatterObject *child)
{
  g_assert_nonnull (child);
  if (tryinsert (child, _children, insert))
    child->initParent (this);
}

GINGA_NAMESPACE_END
