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

#include "fts_engine.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr uint8_t READ = 0x10, READ_DATA = 0x11, READ_PACING = 0x12;
constexpr uint8_t WRITE = 0x20, WRITE_PACING = 0x21, WRITE_DATA = 0x22;
constexpr uint8_t DELETE = 0x30, DELETE_STATUS = 0x31;
constexpr uint8_t MKDIR = 0x40, MKDIR_STATUS = 0x41;
constexpr uint8_t LISTDIR = 0x50, LISTDIR_ENTRY = 0x51;
constexpr uint8_t MOVE = 0x60, MOVE_STATUS = 0x61;

// ERROR_PROTOCOL (0x04) is only ever sent by clients.
constexpr uint8_t OK = 0x01, ERROR = 0x02, ERROR_NO_FILE = 0x03, ERROR_READ_ONLY = 0x05;

constexpr uint32_t FLAG_DIRECTORY = 0x01;

// Request header sizes, command byte included.
constexpr size_t READ_REQ = 12, READ_PACING_REQ = 12, WRITE_REQ = 20, WRITE_DATA_REQ = 12;
constexpr size_t PATH_REQ = 4, MKDIR_REQ = 16, MOVE_REQ = 6;

uint16_t le16(const std::string &b, size_t at)
{
    return uint16_t(uint8_t(b[at]) | uint8_t(b[at + 1]) << 8);
}

uint32_t le32(const std::string &b, size_t at)
{
    return uint32_t(le16(b, at)) | uint32_t(le16(b, at + 2)) << 16;
}

uint64_t le64(const std::string &b, size_t at)
{
    return uint64_t(le32(b, at)) | uint64_t(le32(b, at + 4)) << 32;
}

struct Out {
    std::string bytes;
    Out &u8(uint8_t v) { bytes.push_back(char(v)); return *this; }
    Out &pad(size_t n) { bytes.append(n, '\0'); return *this; }
    Out &u16(uint16_t v) { return u8(uint8_t(v)).u8(uint8_t(v >> 8)); }
    Out &u32(uint32_t v) { return u16(uint16_t(v)).u16(uint16_t(v >> 16)); }
    Out &u64(uint64_t v) { return u32(uint32_t(v)).u32(uint32_t(v >> 32)); }
    Out &raw(const std::string &s) { bytes += s; return *this; }
};

uint8_t statusFor(int error)
{
    switch (error) {
    case ENOENT: case ENOTDIR: return ERROR_NO_FILE;
    case EACCES: case EPERM: case EROFS: return ERROR_READ_ONLY;
    default: return ERROR;
    }
}

uint64_t mtimeNs(const struct stat &st)
{
    return uint64_t(st.st_mtim.tv_sec) * 1000000000ull + uint64_t(st.st_mtim.tv_nsec);
}

bool validUtf8(const std::string &s)
{
    size_t i = 0;
    while (i < s.size()) {
        const uint8_t c = uint8_t(s[i]);
        size_t extra;
        uint32_t cp;
        if (c < 0x80) { i++; continue; }
        else if ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
        else return false;
        if (i + extra >= s.size()) return false;
        for (size_t k = 1; k <= extra; k++) {
            const uint8_t d = uint8_t(s[i + k]);
            if ((d & 0xC0) != 0x80) return false;
            cp = cp << 6 | (d & 0x3F);
        }
        static const uint32_t minimum[] = {0, 0x80, 0x800, 0x10000};
        if (cp < minimum[extra] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
        i += extra + 1;
    }
    return true;
}

bool realPath(const std::string &path, std::string *out, int *error)
{
    char buffer[PATH_MAX];
    if (!::realpath(path.c_str(), buffer)) {
        *error = errno;
        return false;
    }
    *out = buffer;
    return true;
}

int removeEntry(const char *path, const struct stat *, int, struct FTW *)
{
    return ::remove(path) == 0 ? 0 : errno;
}

} // namespace

FtsEngine::FtsEngine(std::string root, uint32_t freeSpace, uint32_t maxRead)
    : m_freeSpace(freeSpace), m_maxRead(maxRead)
{
    int error = 0;
    if (!realPath(root, &m_root, &error))
        m_root = root;  // every resolve() below will then fail closed
}

FtsEngine::~FtsEngine()
{
    closeFile();
}

void FtsEngine::closeFile()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void FtsEngine::reset()
{
    closeFile();
    m_buf.clear();
    m_header.clear();
    m_oldPath.clear();
    expect(Command, 1);
}

void FtsEngine::expect(State state, size_t bytes)
{
    m_state = state;
    m_need = bytes;
}

void FtsEngine::sendReply(const std::string &message)
{
    if (onMessage)
        onMessage(message);
}

void FtsEngine::feed(const uint8_t *data, size_t size)
{
    m_buf.append(reinterpret_cast<const char *>(data), size);
    size_t used = 0;
    while (m_buf.size() - used >= m_need) {
        std::string chunk = m_buf.substr(used, m_need);
        used += m_need;
        step(std::move(chunk));
    }
    m_buf.erase(0, used);
}

bool FtsEngine::inside(const std::string &real) const
{
    return real == m_root || (real.size() > m_root.size() && real.compare(0, m_root.size(), m_root) == 0
                              && real[m_root.size()] == '/');
}

FtsEngine::Resolved FtsEngine::resolve(const std::string &raw, bool followFinal) const
{
    Resolved r;
    if (raw.empty() || raw[0] != '/' || raw.find('\0') != std::string::npos || !validUtf8(raw)) {
        r.status = ERROR;
        return r;
    }
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= raw.size()) {
        size_t end = raw.find('/', start);
        if (end == std::string::npos)
            end = raw.size();
        if (end > start)
            parts.push_back(raw.substr(start, end - start));
        start = end + 1;
    }
    for (const std::string &p : parts) {
        if (p == "." || p == "..") {
            r.status = ERROR;
            return r;
        }
    }
    if (parts.empty()) {
        r.path = m_root;
        r.isRoot = true;
        return r;
    }
    std::string parent = m_root;
    for (size_t i = 0; i + 1 < parts.size(); i++)
        parent += "/" + parts[i];
    std::string parentReal;
    int error = 0;
    if (!realPath(parent, &parentReal, &error)) {
        r.status = statusFor(error) == ERROR_NO_FILE ? ERROR_NO_FILE : ERROR;
        return r;
    }
    if (!inside(parentReal)) {
        r.status = ERROR;
        return r;
    }
    r.path = (parentReal == "/" ? "" : parentReal) + "/" + parts.back();
    struct stat st;
    if (followFinal && ::lstat(r.path.c_str(), &st) == 0 && S_ISLNK(st.st_mode)) {
        std::string real;
        if (!realPath(r.path, &real, &error)) {
            r.status = statusFor(error) == ERROR_NO_FILE ? ERROR_NO_FILE : ERROR;
            return r;
        }
        if (!inside(real)) {
            r.status = ERROR;
            return r;
        }
        r.path = real;
        r.isRoot = real == m_root;
    }
    return r;
}

void FtsEngine::step(std::string chunk)
{
    switch (m_state) {
    case Command:
        m_command = uint8_t(chunk[0]);
        m_header = chunk;
        switch (m_command) {
        case READ: expect(ReadHeader, READ_REQ - 1); break;
        case WRITE: expect(WriteHeader, WRITE_REQ - 1); break;
        case DELETE: case LISTDIR: expect(PathHeader, PATH_REQ - 1); break;
        case MKDIR: expect(MkdirHeader, MKDIR_REQ - 1); break;
        case MOVE: expect(MoveHeader, MOVE_REQ - 1); break;
        default: expect(Command, 1); break;  // stray byte: skip it
        }
        return;

    case ReadHeader:
        m_header += chunk;
        m_offset = le32(m_header, 4);
        m_chunk = le32(m_header, 8);
        expect(ReadPath, le16(m_header, 2));
        return;
    case ReadPath:
        expect(Command, 1);
        beginRead(chunk);
        return;
    case ReadPacing:
        expect(Command, 1);
        if (uint8_t(chunk[0]) != READ_PACING || uint8_t(chunk[1]) != OK) {
            closeFile();
            return;
        }
        m_offset = le32(chunk, 4);
        m_chunk = le32(chunk, 8);
        sendReadChunk();
        return;

    case WriteHeader:
        m_header += chunk;
        m_offset = le32(m_header, 4);
        m_mtime = le64(m_header, 8);
        m_total = le32(m_header, 16);
        expect(WritePath, le16(m_header, 2));
        return;
    case WritePath:
        expect(Command, 1);
        beginWrite(chunk);
        return;
    case WriteDataHeader: {
        expect(Command, 1);
        if (uint8_t(chunk[0]) != WRITE_DATA || uint8_t(chunk[1]) != OK || le32(chunk, 4) != m_offset) {
            closeFile();  // client aborted; the file keeps what was written
            return;
        }
        // Defensive: the grant never exceeds the remaining size, so neither may the data.
        m_dataSize = std::min(le32(chunk, 8), m_total - m_offset);
        expect(WriteData, m_dataSize);
        return;
    }
    case WriteData: {
        expect(Command, 1);
        size_t done = 0;
        while (done < chunk.size()) {
            const ssize_t n = ::pwrite(m_fd, chunk.data() + done, chunk.size() - done, off_t(m_offset) + off_t(done));
            if (n < 0 && errno == EINTR)
                continue;
            if (n <= 0) {
                const int error = n < 0 ? errno : EIO;
                sendReply(Out().u8(WRITE_PACING).u8(statusFor(error)).pad(2).u32(m_offset).u64(0).u32(0).bytes);
                closeFile();
                return;
            }
            done += size_t(n);
        }
        m_offset += uint32_t(chunk.size());
        grantOrFinishWrite();
        return;
    }

    case PathHeader:
        m_header += chunk;
        expect(PathBody, le16(m_header, 2));
        return;
    case PathBody:
        expect(Command, 1);
        if (m_command == DELETE)
            doDelete(chunk);
        else
            doList(chunk);
        return;

    case MkdirHeader:
        m_header += chunk;
        m_mtime = le64(m_header, 8);
        expect(MkdirPath, le16(m_header, 2));
        return;
    case MkdirPath:
        expect(Command, 1);
        doMkdir(chunk);
        return;

    case MoveHeader:
        m_header += chunk;
        expect(MoveOld, le16(m_header, 2));
        return;
    case MoveOld:
        m_oldPath = chunk;
        expect(MoveSep, 1);  // separator byte, any value
        return;
    case MoveSep:
        expect(MoveNew, le16(m_header, 4));
        return;
    case MoveNew:
        expect(Command, 1);
        doMove(m_oldPath, chunk);
        return;
    }
}

void FtsEngine::beginRead(const std::string &raw)
{
    const Resolved r = resolve(raw, true);
    auto fail = [this](uint8_t status) {
        sendReply(Out().u8(READ_DATA).u8(status).pad(2).u32(m_offset).u32(0).u32(0).bytes);
    };
    if (r.status) {
        fail(r.status);
        return;
    }
    m_fd = ::open(r.path.c_str(), O_RDONLY | O_CLOEXEC);
    struct stat st;
    if (m_fd < 0 || ::fstat(m_fd, &st) != 0) {
        const int error = errno;
        closeFile();
        fail(statusFor(error));
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        closeFile();
        fail(ERROR);
        return;
    }
    m_total = st.st_size > off_t(UINT32_MAX) ? UINT32_MAX : uint32_t(st.st_size);
    sendReadChunk();
}

void FtsEngine::sendReadChunk()
{
    if (m_fd < 0)
        return;
    const uint32_t remaining = m_offset < m_total ? m_total - m_offset : 0;
    const uint32_t want = std::min({m_chunk, m_maxRead, remaining});
    std::string data(want, '\0');
    size_t got = 0;
    while (got < want) {
        const ssize_t n = ::pread(m_fd, &data[got], want - got, off_t(m_offset) + off_t(got));
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0) {
            const int error = errno;
            closeFile();
            sendReply(Out().u8(READ_DATA).u8(statusFor(error)).pad(2).u32(m_offset).u32(0).u32(0).bytes);
            return;
        }
        if (n == 0)
            break;  // the file shrank under us
        got += size_t(n);
    }
    data.resize(got);
    sendReply(Out().u8(READ_DATA).u8(OK).pad(2).u32(m_offset).u32(m_total).u32(uint32_t(got)).raw(data).bytes);
    m_offset += uint32_t(got);
    if (m_offset >= m_total || got == 0) {
        closeFile();
        return;
    }
    expect(ReadPacing, READ_PACING_REQ);
}

void FtsEngine::beginWrite(const std::string &raw)
{
    auto fail = [this](uint8_t status) {
        sendReply(Out().u8(WRITE_PACING).u8(status).pad(2).u32(m_offset).u64(0).u32(0).bytes);
    };
    if (m_offset > m_total) {
        fail(ERROR);
        return;
    }
    const Resolved r = resolve(raw, false);
    if (r.status) {
        fail(r.status);
        return;
    }
    struct stat st;
    if (r.isRoot || (::lstat(r.path.c_str(), &st) == 0 && (S_ISLNK(st.st_mode) || S_ISDIR(st.st_mode)))) {
        fail(ERROR);
        return;
    }
    m_fd = ::open(r.path.c_str(), O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (m_fd < 0) {
        fail(statusFor(errno));
        return;
    }
    if (::ftruncate(m_fd, off_t(m_offset)) != 0) {
        const int error = errno;
        closeFile();
        fail(statusFor(error));
        return;
    }
    grantOrFinishWrite();
}

void FtsEngine::grantOrFinishWrite()
{
    if (m_offset < m_total) {
        const uint32_t grant = std::min(m_freeSpace, m_total - m_offset);
        sendReply(Out().u8(WRITE_PACING).u8(OK).pad(2).u32(m_offset).u64(0).u32(grant).bytes);
        expect(WriteDataHeader, WRITE_DATA_REQ);
        return;
    }
    struct timespec times[2];
    times[0].tv_sec = times[1].tv_sec = time_t(m_mtime / 1000000000ull);
    times[0].tv_nsec = times[1].tv_nsec = long(m_mtime % 1000000000ull);
    struct stat st;
    if (::ftruncate(m_fd, off_t(m_total)) != 0 || ::futimens(m_fd, times) != 0 || ::fstat(m_fd, &st) != 0) {
        const int error = errno;
        closeFile();
        sendReply(Out().u8(WRITE_PACING).u8(statusFor(error)).pad(2).u32(m_offset).u64(0).u32(0).bytes);
        return;
    }
    closeFile();
    sendReply(Out().u8(WRITE_PACING).u8(OK).pad(2).u32(m_total).u64(mtimeNs(st)).u32(0).bytes);
}

void FtsEngine::doDelete(const std::string &raw)
{
    auto reply = [this](uint8_t status) { sendReply(Out().u8(DELETE_STATUS).u8(status).bytes); };
    const Resolved r = resolve(raw, false);
    if (r.status || r.isRoot) {
        reply(r.status ? r.status : ERROR);
        return;
    }
    struct stat st;
    if (::lstat(r.path.c_str(), &st) != 0) {
        reply(statusFor(errno));
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        const int result = ::nftw(r.path.c_str(), removeEntry, 16, FTW_DEPTH | FTW_PHYS);
        reply(result == 0 ? OK : statusFor(result > 0 ? result : errno));
        return;
    }
    reply(::unlink(r.path.c_str()) == 0 ? OK : statusFor(errno));
}

void FtsEngine::doList(const std::string &raw)
{
    auto entry = [](uint8_t status, const std::string &name, uint32_t index, uint32_t total,
                    uint32_t flags, uint64_t mtime, uint32_t size) {
        return Out().u8(LISTDIR_ENTRY).u8(status).u16(uint16_t(name.size())).u32(index).u32(total)
            .u32(flags).u64(mtime).u32(size).raw(name).bytes;
    };
    const Resolved r = resolve(raw, true);
    if (r.status) {
        sendReply(entry(r.status, std::string(), 0, 0, 0, 0, 0));
        return;
    }
    DIR *dir = ::opendir(r.path.c_str());
    if (!dir) {
        sendReply(entry(statusFor(errno), std::string(), 0, 0, 0, 0, 0));
        return;
    }
    std::vector<std::string> names;
    while (struct dirent *d = ::readdir(dir)) {
        if (std::strcmp(d->d_name, ".") && std::strcmp(d->d_name, ".."))
            names.emplace_back(d->d_name);
    }
    std::sort(names.begin(), names.end());
    const uint32_t total = uint32_t(names.size());
    // One message per entry: Adafruit's client needs every 28-byte header
    // inside a single notification.
    for (uint32_t i = 0; i < total; i++) {
        struct stat st;
        if (::fstatat(dirfd(dir), names[i].c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0)
            std::memset(&st, 0, sizeof(st));
        if (S_ISLNK(st.st_mode)) {
            std::string real;
            int error = 0;
            struct stat target;
            if (realPath(r.path + "/" + names[i], &real, &error) && inside(real) && ::stat(real.c_str(), &target) == 0)
                st = target;
        }
        const bool isDir = S_ISDIR(st.st_mode);
        sendReply(entry(OK, names[i], i, total, isDir ? FLAG_DIRECTORY : 0, mtimeNs(st),
                   isDir ? 0 : uint32_t(uint64_t(st.st_size) & 0xFFFFFFFFull)));
    }
    ::closedir(dir);
    sendReply(entry(OK, std::string(), total, total, 0, 0, 0));
}

void FtsEngine::doMkdir(const std::string &raw)
{
    auto reply = [this](uint8_t status, uint64_t stored) {
        sendReply(Out().u8(MKDIR_STATUS).u8(status).pad(6).u64(stored).bytes);
    };
    const Resolved whole = resolve(raw, true);
    if (whole.status == ERROR) {  // malformed path; a missing parent is fine here
        reply(ERROR, 0);
        return;
    }
    // Create parents one level at a time so each new level is confined.
    std::string prefix;
    size_t start = 1;
    while (start <= raw.size()) {
        size_t end = raw.find('/', start);
        if (end == std::string::npos)
            end = raw.size();
        if (end > start) {
            prefix += "/" + raw.substr(start, end - start);
            const Resolved level = resolve(prefix, true);
            if (level.status) {
                reply(level.status, 0);
                return;
            }
            struct stat st;
            if (::stat(level.path.c_str(), &st) != 0) {
                if (::mkdir(level.path.c_str(), 0755) != 0) {
                    reply(statusFor(errno), 0);
                    return;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                reply(ERROR, 0);
                return;
            }
        }
        start = end + 1;
    }
    const Resolved r = resolve(raw, true);
    struct stat st;
    if (r.status) {
        reply(r.status, 0);
        return;
    }
    if (!r.isRoot) {
        struct timespec times[2];
        times[0].tv_sec = times[1].tv_sec = time_t(m_mtime / 1000000000ull);
        times[0].tv_nsec = times[1].tv_nsec = long(m_mtime % 1000000000ull);
        if (::utimensat(AT_FDCWD, r.path.c_str(), times, 0) != 0) {
            reply(statusFor(errno), 0);
            return;
        }
    }
    if (::stat(r.path.c_str(), &st) != 0) {
        reply(statusFor(errno), 0);
        return;
    }
    reply(OK, mtimeNs(st));
}

void FtsEngine::doMove(const std::string &oldRaw, const std::string &newRaw)
{
    auto reply = [this](uint8_t status) { sendReply(Out().u8(MOVE_STATUS).u8(status).bytes); };
    const Resolved from = resolve(oldRaw, false);
    const Resolved to = resolve(newRaw, false);
    if (from.status || to.status) {
        reply(from.status ? from.status : to.status);
        return;
    }
    if (from.isRoot || to.isRoot) {
        reply(ERROR);
        return;
    }
    struct stat st;
    if (::lstat(from.path.c_str(), &st) != 0) {
        reply(ERROR_NO_FILE);
        return;
    }
    reply(::rename(from.path.c_str(), to.path.c_str()) == 0 ? OK : statusFor(errno));
}
