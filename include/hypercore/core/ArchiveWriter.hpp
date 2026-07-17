#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ArchiveWriter.hpp — Writes the physical .htx binary file format
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/IArchive.hpp>
#include <fstream>
#include <vector>
#include <string>

namespace hypercore {

class ArchiveWriter : public IArchiveWriter {
public:
    ArchiveWriter() = default;
    ~ArchiveWriter() override;

    [[nodiscard]] Status open(const std::string& path) override;

    [[nodiscard]] Status write_block(const BlockEntry& entry,
                                     ByteSpan compressed_data) override;

    [[nodiscard]] Status write_global_dict(ByteSpan dict_data) override;

    [[nodiscard]] Status write_grammar(ByteSpan grammar_data) override;

    [[nodiscard]] Status finalize(const ArchiveHeader& header,
                                  const std::vector<BlockEntry>& index) override;

private:
    std::ofstream m_file;
    bool m_is_open = false;

    // Helper to write raw data
    [[nodiscard]] Status write_raw(const void* data, size_t size);
};

} // namespace hypercore
