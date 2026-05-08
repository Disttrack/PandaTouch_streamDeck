#include "l10n.h"
#include "storage.h"

static const L10n g_l10n_en = {
    "PandaDeck Dash", "Keyboard:", "OS:", "Grid:", "Background:",
    "Button Configuration", "Name", "Command",
    "App (Win+R / Cmd+Space)", "Media Key", "Basic Combo (Ctrl/Cmd + Key)", "Advanced Combo",
    "Save Changes", "Library", "Upload",
    "Backup & Restore", "Download Backup", "Restore Backup",
    "Firmware OTA", "Select .bin file to update the device.", "Update",
    "Updating System...", "Restore configuration? All settings will be overwritten.",
    "Restore Complete!", "Configuration saved!", "Delete ",
    "Update firmware? The device will restart.",
    "Settings - Customization", "Global Background Color", "Grid Layout Size",
    "Target OS (Win/Mac)", "WiFi Setup", "Keyboard Language (US/ES)",
    "Back", "Cancel", "Save", "Editing Button ", "Editing Global Background",
    "Label:", "Icon:", "Action:", "Cmd/Key:", "Custom Image:",
    "SSID:", "Password:", "Save & Connect",
    "Select Grid Layout", "Select Target OS", "Select Keyboard Language",
    "None", "Basic combination uses Ctrl (Win) or Cmd (Mac) plus one key.",
    "Button", "- Key -",
    {"None", "OK", "Close", "Copy", "Paste", "Cut", "Play", "Pause", "PlayPause", "Mute", "Settings", "Home", "Save", "Edit", "File", "Dir", "Plus", "Prev", "Next", "Stop"},
    "Background Color", "Icon", "Custom Image"
};

static const L10n g_l10n_es = {
    "PandaDeck Dash", "Teclado:", "SO:", "Cuadricula:", "Fondo:",
    "Configuracion de Botones", "Nombre", "Comando",
    "App (Win+R / Cmd+Space)", "Multimedia", "Combo Basico (Ctrl/Cmd + Tecla)", "Combo Avanzado",
    "Guardar Cambios", "Libreria", "Subir",
    "Copia de Seguridad", "Descargar Backup", "Restaurar Backup",
    "Firmware OTA", "Selecciona archivo .bin para actualizar el dispositivo.", "Actualizar",
    "Actualizando sistema...", "Restaurar configuracion? Se sobrescribiran todos los ajustes.",
    "Restauracion completada!", "Configuracion guardada!", "Borrar ",
    "Deseas actualizar el firmware? El dispositivo se reiniciara.",
    "Ajustes - Personalizacion", "Color de Fondo Global", "Tamano de Cuadricula",
    "SO de Destino (Win/Mac)", "Configurar WiFi", "Idioma de Teclado (US/ES)",
    "Atras", "Cancelar", "Guardar", "Editando Boton ", "Editando Fondo Global",
    "Etiqueta:", "Icono:", "Accion:", "Comando/Tecla:", "Imagen Custom:",
    "SSID:", "Contrasena:", "Guardar y Conectar",
    "Seleccionar Cuadricula", "Seleccionar SO", "Seleccionar Idioma",
    "Ninguno", "La combinacion basica usa Ctrl (Windows) o Cmd (Mac) mas una tecla.",
    "Boton", "- Tecla -",
    {"Ninguno", "Aceptar", "Cerrar", "Copiar", "Pegar", "Cortar", "Reproducir", "Pausa", "Play/Pausa", "Silencio", "Ajustes", "Inicio", "Guardar", "Editar", "Archivo", "Carpeta", "Mas", "Anterior", "Siguiente", "Parar"},
    "Color de Fondo", "Icono", "Imagen Personalizada"
};

const char* g_sym_names[20] = {"None", "OK", "Close", "Copy", "Paste", "Cut", "Play", "Pause", "PlayPause", "Mute", "Settings", "Home", "Save", "Edit", "File", "Dir", "Plus", "Prev", "Next", "Stop"};

const char* g_sym_codes[20] = {
    "",
    "\xEF\x80\x8C", "\xEF\x80\x8D", "\xEF\x83\x85", "\xEF\x83\xAA",
    "\xEF\x83\x84", "\xEF\x81\x8B", "\xEF\x81\x8C", "\xEF\x81\x8B\xEF\x81\x8C",
    "\xEF\x80\xA6", "\xEF\x80\x93", "\xEF\x80\x95", "\xEF\x83\x87",
    "\xEF\x8C\x84", "\xEF\x85\x9B", "\xEF\x81\xBB", "\xEF\x81\xA7",
    "\xEF\x81\x88", "\xEF\x81\x91", "\xEF\x81\x8D"
};

const L10n* get_l10n() {
    return (g_kb_lang == LANG_ES) ? &g_l10n_es : &g_l10n_en;
}

const char* get_symbol_by_index(int idx) {
    if (idx < 0 || idx >= 20) return "";
    return g_sym_codes[idx];
}

int get_index_by_symbol(const char* sym) {
    if (!sym || sym[0] == '\0') return 0;
    for (int i = 1; i < 20; i++) {
        if (strcmp(sym, g_sym_codes[i]) == 0) return i;
    }
    return 0;
}
