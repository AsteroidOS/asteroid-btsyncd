/*
 * Adafruit BLE File Transfer protocol v4: transport-free server engine.
 *
 * Copyright (C) 2026 - Martin Monperrus <martin.monperrus@gnieh.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FTS_ENGINE_H
#define FTS_ENGINE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

/*
 * Plain C++17 and POSIX, no Qt, so the host test suite can drive the very
 * same code that runs in asteroid-btsyncd. Its executable specification and
 * tests (a Python reference server, run together with this engine against
 * Adafruit's own client) are in tests/fts/.
 *
 * Requests are a byte stream: feed() accepts any split or coalescing of the
 * client's writes. Every complete reply is handed to onMessage as one
 * message; the transport splits it into notifications, starting a new
 * notification for each message.
 */
class FtsEngine
{
public:
    explicit FtsEngine(std::string root, uint32_t freeSpace = 32768, uint32_t maxRead = 65536);
    ~FtsEngine();
    FtsEngine(const FtsEngine &) = delete;
    FtsEngine &operator=(const FtsEngine &) = delete;

    std::function<void(const std::string &)> onMessage;

    void feed(const uint8_t *data, size_t size);
    void reset();                       // link lost: drop any half-parsed command
    bool midCommand() const { return m_state != Command || !m_buf.empty(); }
    const std::string &root() const { return m_root; }

    static constexpr uint32_t Version = 4;

private:
    enum State {
        Command,
        ReadHeader, ReadPath, ReadPacing,
        WriteHeader, WritePath, WriteDataHeader, WriteData,
        PathHeader, PathBody,
        MkdirHeader, MkdirPath,
        MoveHeader, MoveOld, MoveSep, MoveNew,
    };

    struct Resolved {
        uint8_t status = 0;  // 0: ok, else the Adafruit status to reply with
        std::string path;    // filesystem path
        bool isRoot = false;
    };

    void expect(State state, size_t bytes);
    void step(std::string chunk);
    Resolved resolve(const std::string &raw, bool followFinal) const;
    bool inside(const std::string &real) const;
    void closeFile();

    void beginRead(const std::string &raw);
    void sendReadChunk();
    void beginWrite(const std::string &raw);
    void grantOrFinishWrite();
    void doDelete(const std::string &raw);
    void doList(const std::string &raw);
    void doMkdir(const std::string &raw);
    void doMove(const std::string &oldRaw, const std::string &newRaw);
    void sendReply(const std::string &message);

    std::string m_root;
    uint32_t m_freeSpace;
    uint32_t m_maxRead;

    State m_state = Command;
    size_t m_need = 1;
    std::string m_buf;
    uint8_t m_command = 0;
    std::string m_header;
    std::string m_oldPath;

    int m_fd = -1;
    uint32_t m_offset = 0;   // READ: next offset to send; WRITE: next offset to receive
    uint32_t m_total = 0;
    uint32_t m_chunk = 0;
    uint64_t m_mtime = 0;
    uint32_t m_dataSize = 0;
};

#endif // FTS_ENGINE_H
