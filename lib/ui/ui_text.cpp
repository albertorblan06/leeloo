#include "ui_text.h"

namespace leeloo::ui {

MessageText message_text(Message message) {
  switch (message) {
    case Message::WifiOff: return {"Wi-Fi desactivado", "Actívalo en Ajustes."};
    case Message::ApStartFailed: return {"No se pudo iniciar", "el Wi-Fi."};
    case Message::UploadSaved: return {"Libro guardado.", ""};
    case Message::UploadTimedOut: return {"Conexión cerrada", "por inactividad."};
    case Message::DeleteFailed: return {"No se pudo borrar.", "El libro se conserva."};
    case Message::BookOpenFailed: return {"No se pudo abrir", "el libro."};
    case Message::AudioStartFailed: return {"No se pudo iniciar", "la grabación."};
    case Message::AudioBusy: return {"Almacenamiento", "ocupado. Reintenta."};
    case Message::AudioNoSpace: return {"Sin espacio.", "Libera espacio."};
    case Message::AudioSaveFailed: return {"No se guardó", "la grabación."};
    case Message::AudioSaved: return {"Grabación guardada.", ""};
    case Message::AudioDeleted: return {"Grabación borrada.", ""};
    case Message::AudioDeleteFailed: return {"No se pudo borrar", "la grabación."};
    case Message::AudioTransferred: return {"Grabación enviada.", "Ya está en tu móvil."};
    case Message::AudioUploadStartFailed: return {"No se pudo iniciar", "el Wi-Fi."};
    case Message::AudioUploadTimedOut: return {"Envío cerrado", "por inactividad."};
    case Message::None: break;
  }
  return {"", ""};
}

MessageText wifi_help(settings::WifiMode mode) {
  switch (mode) {
    case settings::WifiMode::Off: return {"No se puede enviar", "ni recibir nada."};
    case settings::WifiMode::Auto: return {"Se activa solo al", "añadir o enviar."};
    case settings::WifiMode::On: return {"Siempre activo.", "Gasta más batería."};
  }
  return {"", ""};
}

}  // namespace leeloo::ui
