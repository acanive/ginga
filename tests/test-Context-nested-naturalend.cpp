/* Copyright (Ctx) 2006-2017 PUC-Rio/Laboratorio TeleMidia

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
along with Ginga.  If not, see <https://www.gnu.org/licenses/>.  */

#include "Event.h"
#include "Media.h"
#include "Context.h"
#include "Parser.h"


#define PARSE_AND_START(fmt, doc, str)                                     \
  G_STMT_START                                                             \
  {                                                                        \
    string buf = str;                                                      \
    string errmsg;                                                         \
    *fmt = new Formatter (0, nullptr, nullptr);                            \
    g_assert_nonnull (*fmt);                                               \
    if (!(*fmt)->start (buf.c_str (), buf.length (), &errmsg))             \
      {                                                                    \
        g_printerr ("*** Unexpected error: %s", errmsg.c_str ());          \
        g_assert_not_reached ();                                           \
      }                                                                    \
    *doc = (*fmt)->getDocument ();                                         \
    g_assert_nonnull (*doc);                                               \
  }                                                                        \
  G_STMT_END

int
main (void)
{

  // Presentation events ---------------------------------------------------

  // @lambda: START from state OCCURRING.
  {
    Formatter *fmt;
    Document *doc;
    PARSE_AND_START (&fmt, &doc, "\
<ncl>\n\
  <head>\n\
  </head>\n\
  <body>\n\
    <port id='p1' component='ctx1'/>\n\
    <context id='ctx1'>\n\
      <port id='p2' component='m1'/>\n\
      <port id='p3' component='ctx2'/>\n\
      <media id='m1'/>\n\
      <context id='ctx2'>\n\
        <port id='p4' component='m2'/>\n\
        <media id='m2' src='samples/bunny.ogg'>\n\
          <area id='a2' begin='3s'/>\n\
        </media>\n\
      </context>\n\
    </context>\n\
  </body>\n\
</ncl>\n");

    Event *root_lambda = doc->getRoot ()->getLambda ();
    g_assert_nonnull (root_lambda);

    Context *ctx1 = cast (Context *, doc->getObjectById ("ctx1"));
    g_assert_nonnull (ctx1);
    Event *ctx1_lambda = ctx1->getLambda ();
    g_assert_nonnull (ctx1_lambda);

    Media *m1 = cast (Media *, doc->getObjectById ("m1"));
    g_assert_nonnull (m1);
    Event *m1_lambda = m1->getLambda ();
    g_assert_nonnull (m1_lambda);

    Context *ctx2 = cast (Context *, doc->getObjectById ("ctx2"));
    g_assert_nonnull (ctx1);
    Event *ctx2_lambda = ctx2->getLambda ();

    Media *m2 = cast (Media *, doc->getObjectById ("m2"));
    g_assert_nonnull (m2);
    Event *m2_lambda = m2->getLambda ();
    g_assert_nonnull (m2_lambda);

    // --------------------------------
    // check start document

    // when start the document, only the lambda@root is OCCURING
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::SLEEPING);
    g_assert (m1_lambda->getState () == Event::SLEEPING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, lambda@ctx1 is OCCURRING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::SLEEPING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, lambda@ctx2 and lambda@m1
    // are OCCURRING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::OCCURRING);
    g_assert (ctx2_lambda->getState () == Event::OCCURRING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, m2 is OCCURRING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::OCCURRING);
    g_assert (ctx2_lambda->getState () == Event::OCCURRING);
    g_assert (m2_lambda->getState () == Event::OCCURRING);

    // --------------------------------
    // main check

    // after stop m2, m2 is SLEEPING
    g_assert_true (m2_lambda->transition (Event::STOP));
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::OCCURRING);
    g_assert (ctx2_lambda->getState () == Event::OCCURRING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, ctx2 is SLEEPING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::OCCURRING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // after stop m1, m1 is SLEEPING
    g_assert_true (m1_lambda->transition (Event::STOP));
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::OCCURRING);
    g_assert (m1_lambda->getState () == Event::SLEEPING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, ctx1 is SLEEPING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::OCCURRING);
    g_assert (ctx1_lambda->getState () == Event::SLEEPING);
    g_assert (m1_lambda->getState () == Event::SLEEPING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, root_lambda is SLEEPING
    fmt->sendTick (0, 0, 0);
    g_assert (root_lambda->getState () == Event::SLEEPING);
    g_assert (ctx1_lambda->getState () == Event::SLEEPING);
    g_assert (m1_lambda->getState () == Event::SLEEPING);
    g_assert (ctx2_lambda->getState () == Event::SLEEPING);
    g_assert (m2_lambda->getState () == Event::SLEEPING);

    // in next reaction, fmt is stoped
    fmt->sendTick (0, 0, 0);
    g_assert (fmt->getState () == GINGA_STATE_STOPPED);

    delete fmt;
  }

  exit (EXIT_SUCCESS);
}
