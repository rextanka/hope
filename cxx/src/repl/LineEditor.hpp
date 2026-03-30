#pragma once

// LineEditor — single-line input with history, cursor movement, and
// basic editing, implemented via POSIX raw terminal mode.
//
// On non-POSIX platforms (or when stdin is not a TTY) the editor falls back
// transparently to std::getline, so the interpreter still works everywhere.
//
// Usage:
//   LineEditor ed;
//   while (auto line = ed.read_line("hope> ")) {
//       // *line is the user's input, without the trailing newline
//   }
//   // returns nullopt on EOF (Ctrl-D on empty line)
//
// Key bindings:
//   Left / Ctrl-B     move cursor left
//   Right / Ctrl-F    move cursor right
//   Home / Ctrl-A     move to start of line
//   End / Ctrl-E      move to end of line
//   Up / Ctrl-P       previous history entry
//   Down / Ctrl-N     next history entry (or blank line)
//   Backspace         delete character before cursor
//   Delete / Ctrl-D   delete character at cursor (EOF on empty line)
//   Ctrl-U            clear entire line
//   Ctrl-K            kill from cursor to end of line
//   Ctrl-W            delete previous word
//   Enter             submit line
//   Ctrl-C            discard current line, show new prompt

#include <optional>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#  include <termios.h>
#endif

namespace hope {

class LineEditor {
public:
    LineEditor();
    ~LineEditor();

    // Read one line from the user.  Returns the line (without trailing '\n')
    // or nullopt on EOF.  `prompt` is printed before each line.
    std::optional<std::string> read_line(const std::string& prompt);

    // Add an entry to the history programmatically (e.g. loaded from a file).
    void add_history(const std::string& line);

    // Return the full history (oldest first).
    const std::vector<std::string>& history() const { return history_; }

private:
    std::vector<std::string> history_;

    // Raw-mode helpers (no-ops on non-POSIX / non-TTY builds).
    bool  enter_raw_mode();
    void  exit_raw_mode();
    bool  raw_mode_active_ = false;

    // The actual editing loop used when stdin is a TTY.
    std::optional<std::string> edit_line(const std::string& prompt);

    // Redraw the current line in place.
    void  refresh(const std::string& prompt,
                  const std::string& buf, size_t cursor);

    // Push a completed non-empty line into history (deduplicating consecutive
    // identical entries).
    void  push_history(const std::string& line);

#if defined(__unix__) || defined(__APPLE__)
    // POSIX termios saved state.
    ::termios* saved_termios_ = nullptr;
#endif
};

} // namespace hope
