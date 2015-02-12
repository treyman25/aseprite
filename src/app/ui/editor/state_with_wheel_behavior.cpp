/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/state_with_wheel_behavior.h"

#include "app/commands/commands.h"
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "ui/message.h"

namespace app {

using namespace ui;

enum WHEEL_ACTION { WHEEL_NONE,
                    WHEEL_ZOOM,
                    WHEEL_VSCROLL,
                    WHEEL_HSCROLL,
                    WHEEL_FG,
                    WHEEL_BG,
                    WHEEL_FRAME };

bool StateWithWheelBehavior::onMouseWheel(Editor* editor, MouseMessage* msg)
{
  int dz = msg->wheelDelta().x + msg->wheelDelta().y;
  WHEEL_ACTION wheelAction = WHEEL_NONE;
  bool scrollBigSteps = false;

  // Alt+mouse wheel changes the fg/bg colors
  if (msg->altPressed()) {
    if (msg->shiftPressed())
      wheelAction = WHEEL_BG;
    else
      wheelAction = WHEEL_FG;
  }
  // Normal behavior: mouse wheel zooms
  else if (UIContext::instance()->settings()->getZoomWithScrollWheel()) {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_FRAME;
    else if (msg->wheelDelta().x != 0 || msg->shiftPressed())
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_ZOOM;
  }
  // For laptops, it's convenient to that Ctrl+wheel zoom (because
  // it's the "pinch" gesture).
  else {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_ZOOM;
    else if (msg->wheelDelta().x != 0 || msg->shiftPressed())
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_VSCROLL;
  }

  switch (wheelAction) {

    case WHEEL_NONE:
      // Do nothing
      break;

    case WHEEL_FG:
      {
        int newIndex = 0;
        if (ColorBar::instance()->getFgColor().getType() == app::Color::IndexType) {
          newIndex = ColorBar::instance()->getFgColor().getIndex() + dz;
          newIndex = MID(0, newIndex, 255);
        }
        ColorBar::instance()->setFgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_BG:
      {
        int newIndex = 0;
        if (ColorBar::instance()->getBgColor().getType() == app::Color::IndexType) {
          newIndex = ColorBar::instance()->getBgColor().getIndex() + dz;
          newIndex = MID(0, newIndex, 255);
        }
        ColorBar::instance()->setBgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_FRAME:
      {
        Command* command = CommandsModule::instance()->getCommandByName
          ((dz < 0) ? CommandId::GotoNextFrame:
                      CommandId::GotoPreviousFrame);
        if (command)
          UIContext::instance()->executeCommand(command, NULL);
      }
      break;

    case WHEEL_ZOOM: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      render::Zoom zoom = editor->zoom();
      if (dz < 0) {
        while (dz++ < 0)
          zoom.in();
      }
      else {
        while (dz-- > 0)
          zoom.out();
      }

      if (editor->zoom() != zoom) {
        editor->setZoomAndCenterInMouse(zoom,
          mouseMsg->position(), Editor::kDontCenterOnZoom);
      }
      break;
    }

    case WHEEL_HSCROLL:
    case WHEEL_VSCROLL: {
      View* view = View::getView(editor);
      gfx::Rect vp = view->getViewportBounds();
      gfx::Point delta(0, 0);

      if (wheelAction == WHEEL_HSCROLL) {
        delta.x = dz * vp.w;
      }
      else {
        delta.y = dz * vp.h;
      }

      if (scrollBigSteps) {
        delta /= 2;
      }
      else {
        delta /= 10;
      }

      gfx::Point scroll = view->getViewScroll();

      editor->hideDrawingCursor();
      editor->setEditorScroll(scroll+delta, true);
      editor->showDrawingCursor();
      break;
    }

  }

  return true;
}

} // namespace app