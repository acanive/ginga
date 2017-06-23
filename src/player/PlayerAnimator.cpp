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
#include "PlayerAnimator.h"

#include "mb/Display.h"
using namespace ::ginga::mb;

GINGA_PLAYER_BEGIN


// PlayerAnimator: Public.

/**
 * @brief Creates a new player animator.
 */
PlayerAnimator::PlayerAnimator ()
{
}

/**
 * @brief Destroys player animator.
 */
PlayerAnimator::~PlayerAnimator ()
{
  this->clear ();
}

/**
 * @brief Remove all scheduled animations.
 */
void
PlayerAnimator::clear ()
{
  _scheduled.clear ();
}

/**
 * @brief Schedule an animation.
 * @param name Property name.
 * @param value Property target value.
 * @param dur Duration.
 */
void
PlayerAnimator::schedule (const string &name,
                          const string &value,
                          GingaTime dur)
{
  if (name == "bounds")
    {
      vector<string> v = ginga_parse_list (value, ',', 4, 4);
      this->doSchedule ("left", v[0], dur);
      this->doSchedule ("top", v[1], dur);
      this->doSchedule ("width", v[2], dur);
      this->doSchedule ("height", v[3], dur);
    }
  else if (name == "location")
    {
      vector<string> v = ginga_parse_list (value, ',', 2, 2);
      this->doSchedule ("left", v[0], dur);
      this->doSchedule ("top", v[1], dur);
    }
  else if (name == "size")
    {
      vector<string> v = ginga_parse_list (value, ',', 2, 2);
      this->doSchedule ("width", v[0], dur);
      this->doSchedule ("height", v[1], dur);
    }
  else if (name == "background")
    {
      SDL_Color color = ginga_parse_color (value);
      this->doSchedule ("background:r", xstrbuild ("%d", color.r), dur);
      this->doSchedule ("background:g", xstrbuild ("%d", color.g), dur);
      this->doSchedule ("background:b", xstrbuild ("%d", color.b), dur);
    }
  else
    {
      this->doSchedule (name, value, dur);
    }
}

static bool
isDone (AnimInfo *info)
{
  return info->isDone ();
}

/**
 * @brief Update scheduled animations.
 * @param rect Variable to store the resulting bounds.
 * @param color Variable to store the resulting background color.
 * @param alpha Variable to store the resulting duration.
 */
void
PlayerAnimator::update (SDL_Rect *rect, SDL_Color *bgColor, guint8 *alpha)
{

#define UPDATE(info, Type, var, min, max)                       \
  G_STMT_START                                                  \
    {                                                           \
      if (!(info)->isInit ())                                   \
        (info)->init ((double)(var));                           \
      (info)->update ();                                        \
      var = (Type) CLAMP ((info)->getCurrent (), min, max);     \
    }                                                           \
  G_STMT_END

  g_assert_nonnull (rect);
  g_assert_nonnull (bgColor);
  g_assert_nonnull (alpha);

  for (AnimInfo *info: _scheduled)
    {
      string name;

      g_assert_nonnull (info);
      g_assert (!info->isDone ());

      name = info->getName ();
      if (name == "top")
        {
          UPDATE (info, int, rect->y, G_MININT, G_MAXINT);
        }
      else if (name == "left")
        {
          UPDATE (info, int, rect->x, G_MININT, G_MAXINT);
        }
      else if (name == "width")
        {
          UPDATE (info, int, rect->w, G_MININT, G_MAXINT);
        }
      else if (name == "height")
        {
          UPDATE (info, int, rect->h, G_MININT, G_MAXINT);
        }
      else if (name == "background:r")
        {
          UPDATE (info, guint8, bgColor->r, 0, 255);
        }
      else if (name == "background:g")
        {
          UPDATE (info, guint8, bgColor->g, 0, 255);
        }
      else if (name == "background:b")
        {
          UPDATE (info, guint8, bgColor->b, 0, 255);
        }
      else if (name == "transparency")
        {
          UPDATE (info, guint8, *alpha, 0, 255);
        }
      else
        {
          WARNING ("cannot animate property '%s'", name.c_str ());
        }
    }

  _scheduled.remove_if (isDone);
}


// PlayerAnimator: Private.

void
PlayerAnimator::doSchedule (const string &name,
                            const string &value,
                            GingaTime dur)
{
  double target;
  int width, height;

  Ginga_Display->getSize (&width, &height);
  if (name == "top" || name == "height")
    {
      target = ginga_parse_percent (value, height, 0, G_MAXINT);
      TRACE ("target '%s' is '%g'", name.c_str (), target);
    }
  else if (name == "left" || name == "width")
    {
      target = ginga_parse_percent (value, width, 0, G_MAXINT);
      TRACE ("target '%s' is '%g'", name.c_str (), target);
    }
  else if (name == "transparency" || name == "background")
    {
      target = ginga_parse_percent (value, 255, 0, 255);
    }
  else
    {
      target = ginga_parse_percent (value, 100, 0, G_MAXINT);
    }

  _scheduled.push_back (new AnimInfo (name, target, dur));
}


// AnimInfo.

/**
 * @brief Creates animation info.
 * @param name Property name.
 * @param target Target value.
 * @param dur Duration.
 */
AnimInfo::AnimInfo (const string &name, double target, GingaTime dur)
{
  _name = name;
  _target = target;
  _duration = dur;
  _done = false;
  _init = false;
}

/**
 * @brief Destroys animation info.
 */
AnimInfo::~AnimInfo ()
{
}

/**
 * @brief Gets animation name.
 */
string
AnimInfo::getName ()
{
  return _name;
}

/**
 * @brief Gets animation current value.
 */
double
AnimInfo::getCurrent ()
{
  g_assert (_init);
  return _current;
}

/**
 * @brief Gets animation target value.
 */
double
AnimInfo::getTarget ()
{
  return _target;
}

/**
 * @brief Gets animation  duration.
 */
GingaTime
AnimInfo::getDuration ()
{
  return _duration;
}

/**
 * @brief Gets animation speed.
 */
double
AnimInfo::getSpeed ()
{
  g_assert (_init);
  return _speed;
}

/**
 * @brief Tests if animation is done.
 */
bool
AnimInfo::isDone ()
{
  return _done;
}

/**
 * @brief Tests if animation is initialized.
 */
bool
AnimInfo::isInit ()
{
  return _init;
}

/**
 * @brief Initializes animation.
 */
void
AnimInfo::init (double current)
{
  g_assert (!_init);
  _current = current;
  if (_duration > 0)
    _speed = (abs (_target - current)
              / (double) GINGA_TIME_AS_SECONDS (_duration));
  else
    _speed = 0;
  _init = true;
}

/**
 * @brief Updates animation.
 * @param current Current value.
 * @return The updated value.
 */
void
AnimInfo::update (void)
{
  double fps;
  int dir;

  g_assert (_init);
  g_assert (!_done);

  fps = (double) Ginga_Display->getFPS ();
  dir = (_current < _target) ? 1 : -1;
  _current += dir * (_speed / fps);

  if (_duration == 0
      || (dir > 0 && _current >= _target)
      || (dir < 0 && _current <= _target))
    {
      _done = true;
    }
}

GINGA_PLAYER_END
