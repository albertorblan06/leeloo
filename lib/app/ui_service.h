#pragma once

#include "messages.h"
#include "ports.h"
#include "rtos.h"
#include "settings.h"
#include "ui.h"

namespace leeloo::app {

// Owns the menu state machine. Every event from any task reaches it through one
// queue; the command the state machine returns becomes a message to the RSVP or
// net service, or a direct call on a port for the quick local ones (delete, save settings).
// The UI task blocks on its event queue and calls handle() for each event.
class UiService {
 public:
  UiService(const settings::Settings& initial, ILibrary* library, ISettingsStore* store,
            IDisplayControl* display, rtos::Queue<RsvpMsg>* rsvp_out, rtos::Queue<NetMsg>* net_out,
            ILog* log = nullptr);

  // Runs one event. The caller redraws afterwards: every event can change the screen.
  void handle(ui::Event event);

  // Keeps the menu's book count in step with the library. Only on change: the
  // count is not reset on every event, which would move the list cursor back to 0.
  void sync_books();

  const ui::Ui& model() const { return ui_; }

 private:
  void execute(const ui::Outcome& out);

  ui::Ui ui_;
  ILibrary* library_;
  ISettingsStore* store_;
  IDisplayControl* display_;
  rtos::Queue<RsvpMsg>* rsvp_out_;
  rtos::Queue<NetMsg>* net_out_;
  ILog* log_;
};

}  // namespace leeloo::app
