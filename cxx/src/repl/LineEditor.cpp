#include "repl/LineEditor.hpp"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Platform detection
// ---------------------------------------------------------------------------
#if defined(_WIN32)
#  define HOPE_PLATFORM_WINDOWS 1
#elif defined(__unix__) || defined(__APPLE__)
#  define HOPE_PLATFORM_POSIX 1
#endif

#ifdef HOPE_PLATFORM_POSIX
#  include <termios.h>
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <csignal>
#  include <cstring>
#endif

#include <cstdio>

namespace hope {

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

LineEditor::LineEditor() {
#ifdef HOPE_PLATFORM_POSIX
    saved_termios_ = new ::termios{};
#endif
}

LineEditor::~LineEditor() {
    if (raw_mode_active_) exit_raw_mode();
#ifdef HOPE_PLATFORM_POSIX
    delete saved_termios_;
    saved_termios_ = nullptr;
#endif
}

// ---------------------------------------------------------------------------
// History management
// ---------------------------------------------------------------------------

void LineEditor::add_history(const std::string& line) {
    if (!line.empty()) history_.push_back(line);
}

void LineEditor::push_history(const std::string& line) {
    if (line.empty()) return;
    // Avoid consecutive duplicates.
    if (!history_.empty() && history_.back() == line) return;
    history_.push_back(line);
}

// ---------------------------------------------------------------------------
// POSIX raw-mode implementation
// ---------------------------------------------------------------------------

#ifdef HOPE_PLATFORM_POSIX

bool LineEditor::enter_raw_mode() {
    if (!isatty(STDIN_FILENO)) return false;
    if (tcgetattr(STDIN_FILENO, saved_termios_) != 0) return false;

    ::termios raw = *saved_termios_;
    // Input flags: no break, no CR→NL, no parity, no 8-bit strip, no flow ctrl.
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Output flags: disable post-processing so we control \r\n ourselves.
    raw.c_oflag &= ~(OPOST);
    // Character size: 8-bit.
    raw.c_cflag |= (CS8);
    // Local flags: no echo, no canonical, no extended processing.
    // Keep ISIG so Ctrl-C / Ctrl-Z still deliver signals normally.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_lflag |= ISIG;
    // Read returns after 1 byte with no timeout.
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) return false;
    raw_mode_active_ = true;
    return true;
}

void LineEditor::exit_raw_mode() {
    if (!raw_mode_active_) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, saved_termios_);
    raw_mode_active_ = false;
}

// ---------------------------------------------------------------------------
// refresh — redraw the input line in place.
//
// We keep everything on one physical line.  If prompt+buf would wrap we still
// let the terminal wrap naturally; we just always move back to the start of
// the prompt when refreshing.
// ---------------------------------------------------------------------------
void LineEditor::refresh(const std::string& prompt,
                         const std::string& buf, size_t cursor)
{
    // Build the output in one write to avoid flicker.
    std::string out;

    // Move to start of line.
    out += '\r';

    // Print prompt + buffer.
    out += prompt;
    out += buf;

    // Clear to end of line.
    out += "\x1b[K";

    // Move cursor to correct position: back to start, then forward
    // by (prompt_len + cursor) columns.
    int move = static_cast<int>(prompt.size() + cursor);
    out += '\r';
    if (move > 0) {
        out += "\x1b[";
        out += std::to_string(move);
        out += 'C';
    }

    // Write atomically.
    (void)::write(STDOUT_FILENO, out.c_str(), out.size());
}

// ---------------------------------------------------------------------------
// edit_line — the actual interactive editing loop (POSIX TTY path).
// ---------------------------------------------------------------------------
std::optional<std::string> LineEditor::edit_line(const std::string& prompt)
{
    // Print prompt.
    (void)::write(STDOUT_FILENO, prompt.c_str(), prompt.size());

    std::string buf;       // current line buffer
    size_t      cursor = 0;// insertion point (index into buf)
    int         hist_idx = static_cast<int>(history_.size()); // points past end
    std::string saved_buf; // original line saved when navigating history

    // Bracketed-paste mode: enable so we can detect paste bursts.
    static const char* bp_on  = "\x1b[?2004h";
    static const char* bp_off = "\x1b[?2004l";
    (void)::write(STDOUT_FILENO, bp_on, strlen(bp_on));

    auto cleanup = [&]() {
        (void)::write(STDOUT_FILENO, bp_off, strlen(bp_off));
    };

    bool in_paste = false; // inside a bracketed paste

    while (true) {
        unsigned char c = 0;
        ssize_t n = ::read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            // EOF or error.
            cleanup();
            (void)::write(STDOUT_FILENO, "\r\n", 2);
            if (buf.empty()) return std::nullopt;
            push_history(buf);
            return buf;
        }

        // --- Escape sequence / special key handling ---
        if (c == '\x1b') {
            // Read up to two more bytes.
            unsigned char seq[4] = {};
            ssize_t r1 = ::read(STDIN_FILENO, &seq[0], 1);
            if (r1 <= 0) {
                // Bare ESC — ignore.
                continue;
            }
            if (seq[0] == '[') {
                ssize_t r2 = ::read(STDIN_FILENO, &seq[1], 1);
                if (r2 <= 0) continue;

                // Bracketed paste start: ESC [ 2 0 0 ~
                if (seq[1] == '2') {
                    unsigned char rest[3] = {};
                    (void)::read(STDIN_FILENO, rest, 3);
                    if (rest[0] == '0' && rest[1] == '0' && rest[2] == '~') {
                        in_paste = true;
                    }
                    continue;
                }
                // Bracketed paste end: ESC [ 2 0 1 ~
                // (same prefix — we check here too)
                if (seq[1] == '2') {
                    // already handled above, but guard defensively
                    continue;
                }

                if (seq[1] >= '0' && seq[1] <= '9') {
                    // Extended sequence: ESC [ <digit> ~
                    unsigned char tilde = 0;
                    (void)::read(STDIN_FILENO, &tilde, 1);
                    if (tilde == '~') {
                        if (seq[1] == '3') {
                            // Delete key: ESC [ 3 ~
                            if (cursor < buf.size())
                                buf.erase(cursor, 1);
                            refresh(prompt, buf, cursor);
                        }
                        // ESC [ 1 ~ = Home, ESC [ 4 ~ = End (some terminals)
                        if (seq[1] == '1') {
                            cursor = 0;
                            refresh(prompt, buf, cursor);
                        }
                        if (seq[1] == '4') {
                            cursor = buf.size();
                            refresh(prompt, buf, cursor);
                        }
                        // Bracketed paste end: ESC [ 2 0 1 ~ (seq[1]=='2' already handled)
                    }
                    continue;
                }

                switch (seq[1]) {
                    case 'A': // Up arrow — previous history
                        if (hist_idx == static_cast<int>(history_.size()))
                            saved_buf = buf; // save current line
                        if (hist_idx > 0) {
                            --hist_idx;
                            buf    = history_[hist_idx];
                            cursor = buf.size();
                            refresh(prompt, buf, cursor);
                        }
                        break;
                    case 'B': // Down arrow — next history
                        if (hist_idx < static_cast<int>(history_.size())) {
                            ++hist_idx;
                            if (hist_idx == static_cast<int>(history_.size()))
                                buf = saved_buf;
                            else
                                buf = history_[hist_idx];
                            cursor = buf.size();
                            refresh(prompt, buf, cursor);
                        }
                        break;
                    case 'C': // Right arrow
                        if (cursor < buf.size()) {
                            ++cursor;
                            refresh(prompt, buf, cursor);
                        }
                        break;
                    case 'D': // Left arrow
                        if (cursor > 0) {
                            --cursor;
                            refresh(prompt, buf, cursor);
                        }
                        break;
                    case 'H': // Home (xterm)
                        cursor = 0;
                        refresh(prompt, buf, cursor);
                        break;
                    case 'F': // End (xterm)
                        cursor = buf.size();
                        refresh(prompt, buf, cursor);
                        break;
                    default:
                        break;
                }
            } else if (seq[0] == 'O') {
                // SS3 sequences: ESC O H = Home, ESC O F = End
                unsigned char s2 = 0;
                (void)::read(STDIN_FILENO, &s2, 1);
                if (s2 == 'H') { cursor = 0;           refresh(prompt, buf, cursor); }
                if (s2 == 'F') { cursor = buf.size();   refresh(prompt, buf, cursor); }
            }
            continue;
        }

        // Bracketed paste end marker: ESC[201~ — handled in escape branch.
        // Inside a paste we just treat everything as regular characters.
        // (in_paste reset here for safety when terminal doesn't send end marker)

        // --- Control characters ---
        switch (c) {
            case '\r':   // Enter
            case '\n':
            {
                // Move to next line.
                (void)::write(STDOUT_FILENO, "\r\n", 2);
                cleanup();
                push_history(buf);
                in_paste = false;
                return buf;
            }
            case 127:    // Backspace (DEL)
            case '\b':   // Backspace (BS)
                if (cursor > 0) {
                    buf.erase(cursor - 1, 1);
                    --cursor;
                    refresh(prompt, buf, cursor);
                }
                break;
            case 4:      // Ctrl-D: delete at cursor, or EOF on empty line
                if (buf.empty()) {
                    (void)::write(STDOUT_FILENO, "\r\n", 2);
                    cleanup();
                    return std::nullopt;
                }
                if (cursor < buf.size()) {
                    buf.erase(cursor, 1);
                    refresh(prompt, buf, cursor);
                }
                break;
            case 1:      // Ctrl-A: beginning of line
                cursor = 0;
                refresh(prompt, buf, cursor);
                break;
            case 5:      // Ctrl-E: end of line
                cursor = buf.size();
                refresh(prompt, buf, cursor);
                break;
            case 2:      // Ctrl-B: back one character
                if (cursor > 0) { --cursor; refresh(prompt, buf, cursor); }
                break;
            case 6:      // Ctrl-F: forward one character
                if (cursor < buf.size()) { ++cursor; refresh(prompt, buf, cursor); }
                break;
            case 16:     // Ctrl-P: previous history (same as Up)
                if (hist_idx == static_cast<int>(history_.size()))
                    saved_buf = buf;
                if (hist_idx > 0) {
                    --hist_idx;
                    buf = history_[hist_idx]; cursor = buf.size();
                    refresh(prompt, buf, cursor);
                }
                break;
            case 14:     // Ctrl-N: next history (same as Down)
                if (hist_idx < static_cast<int>(history_.size())) {
                    ++hist_idx;
                    if (hist_idx == static_cast<int>(history_.size()))
                        buf = saved_buf;
                    else
                        buf = history_[hist_idx];
                    cursor = buf.size();
                    refresh(prompt, buf, cursor);
                }
                break;
            case 21:     // Ctrl-U: clear line
                buf.clear(); cursor = 0;
                refresh(prompt, buf, cursor);
                break;
            case 11:     // Ctrl-K: kill to end of line
                buf.erase(cursor);
                refresh(prompt, buf, cursor);
                break;
            case 23:     // Ctrl-W: delete previous word
            {
                if (cursor == 0) break;
                size_t end = cursor;
                // Skip trailing spaces.
                while (cursor > 0 && buf[cursor - 1] == ' ') --cursor;
                // Skip word characters.
                while (cursor > 0 && buf[cursor - 1] != ' ') --cursor;
                buf.erase(cursor, end - cursor);
                refresh(prompt, buf, cursor);
                break;
            }
            case 12:     // Ctrl-L: clear screen, redraw
            {
                (void)::write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
                refresh(prompt, buf, cursor);
                break;
            }
            default:
                // Printable character (or pasted byte): insert at cursor.
                if (c >= 32 || in_paste) {
                    buf.insert(cursor, 1, static_cast<char>(c));
                    ++cursor;
                    refresh(prompt, buf, cursor);
                }
                break;
        }
    }
}

#else // !HOPE_PLATFORM_POSIX

bool  LineEditor::enter_raw_mode()  { return false; }
void  LineEditor::exit_raw_mode()   {}
void  LineEditor::refresh(const std::string&, const std::string&, size_t) {}
std::optional<std::string> LineEditor::edit_line(const std::string& prompt) {
    // Should never be called on non-POSIX.
    return std::nullopt;
}

#endif // HOPE_PLATFORM_POSIX

// ---------------------------------------------------------------------------
// read_line — public entry point
// ---------------------------------------------------------------------------

std::optional<std::string> LineEditor::read_line(const std::string& prompt)
{
#ifdef HOPE_PLATFORM_POSIX
    if (isatty(STDIN_FILENO)) {
        if (!enter_raw_mode()) {
            // Couldn't enter raw mode — fall through to getline.
        } else {
            auto result = edit_line(prompt);
            exit_raw_mode();
            return result;
        }
    }
#endif
    // Fallback: plain getline (non-TTY, Windows, or raw-mode failure).
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return line;
}

} // namespace hope
