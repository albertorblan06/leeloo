#include "upload_server.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <esp_random.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "upload.h"

namespace leeloo::upload_server {
namespace {

using namespace leeloo::upload;

constexpr const char* kSsid = "Leeloo";
// web/dist is the LittleFS image (platformio.ini data_dir), so assets sit at the root.
constexpr const char* kWebRoot = "";
constexpr const char* kBooksDir = "/books";
constexpr const char* kTempPath = "/books/.upload.tmp";
constexpr const char* kBackupPath = "/books/.replaced.tmp";
constexpr const char* kCookieName = "leeloo_session=";
constexpr std::size_t kTokenChars = 16;
constexpr std::size_t kChunkBytes = 1024;
constexpr std::size_t kMaxNameHeader = 3 * kMaxNameBytes;  // every byte percent-encoded
constexpr std::size_t kMaxAuthBody = 64;

httpd_handle_t g_server = nullptr;
char g_pin[kPinDigits + 1] = {};
char g_token[kTokenChars + 1] = {};  // empty until a client authenticates

const char* status_line(int status) {
  switch (status) {
    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 409: return "409 Conflict";
    case 413: return "413 Payload Too Large";
    case 422: return "422 Unprocessable Entity";
    case 507: return "507 Insufficient Storage";
    default: return "500 Internal Server Error";
  }
}

void send_json_error(httpd_req_t* r, int status, const char* code) {
  char body[48];
  std::snprintf(body, sizeof body, "{\"error\":\"%s\"}", code);
  httpd_resp_set_status(r, status_line(status));
  httpd_resp_set_type(r, "application/json");
  httpd_resp_send(r, body, HTTPD_RESP_USE_STRLEN);
}

// Answering with request body still unread makes TCP reset the connection, and
// the phone then shows a network error instead of this code. So drain it first,
// bounded by the book limit.
void drain_body(httpd_req_t* r, std::size_t already_read) {
  const std::size_t unread = r->content_len > already_read ? r->content_len - already_read : 0;
  char buf[256];
  std::size_t left = std::min(unread, kMaxBookBytes);
  while (left > 0) {
    const int n = httpd_req_recv(r, buf, std::min(left, sizeof buf));
    if (n <= 0) return;
    left -= static_cast<std::size_t>(n);
  }
}

esp_err_t fail_with(httpd_req_t* r, Result result, std::size_t already_read = 0) {
  drain_body(r, already_read);
  send_json_error(r, http_status(result), error_code(result));
  return ESP_OK;
}

const char* mime_for(const char* path) {
  const char* dot = std::strrchr(path, '.');
  if (dot == nullptr) return "application/octet-stream";
  if (std::strcmp(dot, ".html") == 0) return "text/html";
  if (std::strcmp(dot, ".css") == 0) return "text/css";
  if (std::strcmp(dot, ".js") == 0 || std::strcmp(dot, ".mjs") == 0) return "text/javascript";
  return "application/octet-stream";
}

esp_err_t static_handler(httpd_req_t* r) {
  const char* uri = r->uri;
  std::size_t len = std::strcspn(uri, "?");
  if (len == 1 && uri[0] == '/') {
    uri = "/index.html";
    len = std::strlen(uri);
  }
  char path[HTTPD_MAX_URI_LEN + 16];
  if (len + std::strlen(kWebRoot) + 4 > sizeof path || std::strstr(uri, "..") != nullptr) {
    httpd_resp_send_404(r);
    return ESP_OK;
  }
  std::snprintf(path, sizeof path, "%s%.*s.gz", kWebRoot, static_cast<int>(len), uri);
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) {
    httpd_resp_send_404(r);
    return ESP_OK;
  }
  // The type comes from the name without the ".gz" suffix.
  path[std::strlen(path) - 3] = '\0';
  httpd_resp_set_type(r, mime_for(path));
  httpd_resp_set_hdr(r, "Content-Encoding", "gzip");
  char buf[kChunkBytes];
  for (int n = f.read(reinterpret_cast<uint8_t*>(buf), sizeof buf); n > 0;
       n = f.read(reinterpret_cast<uint8_t*>(buf), sizeof buf)) {
    if (httpd_resp_send_chunk(r, buf, n) != ESP_OK) return ESP_FAIL;
  }
  return httpd_resp_send_chunk(r, nullptr, 0);
}

bool read_body(httpd_req_t* r, char* buf, std::size_t len) {
  for (std::size_t got = 0; got < len;) {
    const int n = httpd_req_recv(r, buf + got, len - got);
    if (n <= 0) return false;
    got += static_cast<std::size_t>(n);
  }
  return true;
}

esp_err_t auth_handler(httpd_req_t* r) {
  char body[kMaxAuthBody];
  char pin[kPinDigits + 1];
  if (r->content_len == 0 || r->content_len > sizeof body) {
    drain_body(r, 0);
    send_json_error(r, 401, "invalid_pin");
    return ESP_OK;
  }
  if (!read_body(r, body, r->content_len)) return ESP_FAIL;
  if (!extract_pin(body, r->content_len, pin) || !pin_matches(g_pin, pin, kPinDigits)) {
    send_json_error(r, 401, "invalid_pin");
    return ESP_OK;
  }
  // Reusing the token keeps an earlier client's cookie valid if it reconnects.
  if (g_token[0] == '\0') {
    std::uint8_t random[kTokenChars / 2];
    esp_fill_random(random, sizeof random);
    for (std::size_t i = 0; i < sizeof random; ++i) {
      std::snprintf(g_token + 2 * i, 3, "%02x", random[i]);
    }
  }
  char cookie[96];
  std::snprintf(cookie, sizeof cookie, "%s%s; HttpOnly; SameSite=Strict; Path=/", kCookieName, g_token);
  httpd_resp_set_hdr(r, "Set-Cookie", cookie);
  httpd_resp_set_type(r, "application/json");
  return httpd_resp_send(r, "{}", 2);
}

bool authorized(httpd_req_t* r) {
  if (g_token[0] == '\0') return false;
  char cookies[256];  // browsers may send several cookies for the AP address
  if (httpd_req_get_hdr_value_str(r, "Cookie", cookies, sizeof cookies) != ESP_OK) return false;
  const char* p = std::strstr(cookies, kCookieName);
  if (p == nullptr) return false;
  p += std::strlen(kCookieName);
  unsigned diff = 0;
  for (std::size_t i = 0; i < kTokenChars; ++i) {
    if (p[i] == '\0') return false;
    diff |= static_cast<unsigned>(static_cast<std::uint8_t>(p[i]) ^ static_cast<std::uint8_t>(g_token[i]));
  }
  return diff == 0 && (p[kTokenChars] == '\0' || p[kTokenChars] == ';' || p[kTokenChars] == ' ');
}

std::size_t count_books() {
  std::size_t count = 0;
  File dir = LittleFS.open(kBooksDir);
  if (!dir || !dir.isDirectory()) return 0;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    const char* name = f.name();
    const std::size_t len = std::strlen(name);
    if (!f.isDirectory() && len > 4 && std::strcmp(name + len - 4, ".txt") == 0) ++count;
  }
  return count;
}

// Puts the finished temp file at `path`. An existing book is moved aside first,
// so a failed rename can be undone and the old book is not lost.
Result commit(const char* path, bool replacing) {
  if (replacing && !LittleFS.rename(path, kBackupPath)) return Result::StorageError;
  if (LittleFS.rename(kTempPath, path)) {
    if (replacing) LittleFS.remove(kBackupPath);
    return Result::Ok;
  }
  if (replacing) LittleFS.rename(kBackupPath, path);  // put the old book back
  return Result::StorageError;
}

// Reads X-Book-Name, decodes and sanitizes it. Returns the length, 0 if rejected.
std::size_t book_name_from_request(httpd_req_t* r, char* name, std::size_t cap) {
  const std::size_t header_len = httpd_req_get_hdr_value_len(r, "X-Book-Name");
  if (header_len == 0 || header_len > kMaxNameHeader) return 0;
  char encoded[kMaxNameHeader + 1];
  if (httpd_req_get_hdr_value_str(r, "X-Book-Name", encoded, sizeof encoded) != ESP_OK) return 0;
  char decoded[kMaxNameBytes + 1];
  const int decoded_len = percent_decode(encoded, header_len, decoded, sizeof decoded);
  if (decoded_len < 0) return 0;
  return sanitize_name(decoded, static_cast<std::size_t>(decoded_len), name, cap);
}

esp_err_t books_handler(httpd_req_t* r) {
  if (!authorized(r)) return fail_with(r, Result::Unauthorized);
  char name[kMaxNameBytes + 1];
  if (book_name_from_request(r, name, sizeof name) == 0) return fail_with(r, Result::BadName);
  if (r->content_len == 0) return fail_with(r, Result::InvalidFormat);
  if (r->content_len > kMaxBookBytes) return fail_with(r, Result::TooLarge);

  char path[sizeof name + 16];
  std::snprintf(path, sizeof path, "%s/%s.txt", kBooksDir, name);
  const bool replacing = LittleFS.exists(path);
  const std::size_t books = count_books() - (replacing ? 1 : 0);
  const std::size_t free_bytes = LittleFS.totalBytes() - LittleFS.usedBytes();
  const Result capacity = check_capacity(r->content_len, free_bytes, books);
  if (capacity != Result::Ok) return fail_with(r, capacity);

  File f = LittleFS.open(kTempPath, "w");
  if (!f) return fail_with(r, Result::StorageError);

  BookValidator validator;
  Result result = Result::Ok;
  std::uint8_t buf[kChunkBytes];
  std::size_t remaining = r->content_len;
  while (remaining > 0 && result == Result::Ok) {
    const int n = httpd_req_recv(r, reinterpret_cast<char*>(buf), std::min(remaining, sizeof buf));
    if (n <= 0) {  // timeout or closed socket: nobody is left to answer
      f.close();
      LittleFS.remove(kTempPath);
      return ESP_FAIL;
    }
    const std::size_t got = static_cast<std::size_t>(n);
    result = validator.feed(buf, got);
    if (result == Result::Ok && f.write(buf, got) != got) result = Result::StorageError;
    remaining -= got;
  }
  f.close();
  if (result == Result::Ok) result = validator.finish();
  if (result == Result::Ok) result = commit(path, replacing);
  if (result != Result::Ok) {
    LittleFS.remove(kTempPath);
    return fail_with(r, result, r->content_len - remaining);
  }

  char body[kMaxNameBytes + 48];
  std::snprintf(body, sizeof body, "{\"id\":\"%s\",\"bytes\":%u}", name,
                static_cast<unsigned>(r->content_len));
  httpd_resp_set_status(r, "201 Created");
  httpd_resp_set_type(r, "application/json");
  return httpd_resp_send(r, body, HTTPD_RESP_USE_STRLEN);
}

httpd_uri_t make_route(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*)) {
  httpd_uri_t route = {};  // zero-fills the websocket fields that exist only in some configs
  route.uri = uri;
  route.method = method;
  route.handler = handler;
  return route;
}

bool register_handlers() {
  // Specific routes first; the wildcard GET catches every static asset.
  const httpd_uri_t routes[] = {
      make_route("/api/auth", HTTP_POST, auth_handler),
      make_route("/api/books", HTTP_POST, books_handler),
      make_route("/*", HTTP_GET, static_handler),
  };
  for (const httpd_uri_t& route : routes) {
    if (httpd_register_uri_handler(g_server, &route) != ESP_OK) return false;
  }
  return true;
}

}  // namespace

bool start(char* pin_out) {
  stop();
  // No auto-format: a mount failure must not overwrite existing books.
  if (!LittleFS.begin(false)) return false;
  LittleFS.mkdir(kBooksDir);   // fails harmlessly when it already exists
  // Leftovers from an interrupted upload or replace. shortcut: a power cut between
  // the two renames in commit() drops the replaced book here; upgrade with a
  // journal file if that window matters on hardware.
  LittleFS.remove(kTempPath);
  LittleFS.remove(kBackupPath);

  if (!WiFi.mode(WIFI_AP) || !WiFi.softAP(kSsid)) {
    stop();
    return false;
  }
  // Generated after the radio is up, so the PIN draws on RF entropy.
  format_pin(esp_random(), g_pin);
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 8192;  // static_handler keeps a 1 KiB buffer on the stack
  config.max_open_sockets = 4;
  config.lru_purge_enable = true;  // the page's parallel asset loads must not starve new clients
  config.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&g_server, &config) != ESP_OK || !register_handlers()) {
    stop();
    return false;
  }
  std::memcpy(pin_out, g_pin, sizeof g_pin);
  return true;
}

void stop() {
  if (g_server != nullptr) {
    httpd_stop(g_server);
    g_server = nullptr;
  }
  WiFi.softAPdisconnect(true);
  std::memset(g_pin, 0, sizeof g_pin);
  std::memset(g_token, 0, sizeof g_token);
}

}  // namespace leeloo::upload_server
