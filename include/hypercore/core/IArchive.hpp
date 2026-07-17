#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// IArchive.hpp — Abstract archive reader/writer interfaces for .htx format.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>
#include <string>
#include <vector>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
/// .htx archive magic bytes and format version.
// ─────────────────────────────────────────────────────────────────────────────
struct ArchiveFormat {
    static constexpr u8  MAGIC[4]    = {'H', 'T', 'X', '\x01'};
    static constexpr u16 VERSION     = 1;
    static constexpr u16 MIN_VERSION = 1;   ///< Oldest version this code can read
};

// ─────────────────────────────────────────────────────────────────────────────
/// Flags packed into the archive header's flags field.
// ─────────────────────────────────────────────────────────────────────────────
enum class ArchiveFlags : u16 {
    None              = 0x0000,
    GpuPath           = 0x0001,  ///< Archive was produced using GPU acceleration
    ContextIslands    = 0x0002,  ///< Context Island detection was enabled
    GrammarEnabled    = 0x0004,  ///< Grammar compression was applied
    NeuralPredictor   = 0x0008,  ///< Neural predictor was used
};

inline ArchiveFlags operator|(ArchiveFlags a, ArchiveFlags b) noexcept {
    return static_cast<ArchiveFlags>(static_cast<u16>(a) | static_cast<u16>(b));
}
inline bool operator&(ArchiveFlags a, ArchiveFlags b) noexcept {
    return (static_cast<u16>(a) & static_cast<u16>(b)) != 0;
}

// ─────────────────────────────────────────────────────────────────────────────
/// Metadata about a single compressed block in the archive.
// ─────────────────────────────────────────────────────────────────────────────
struct BlockEntry {
    u64 offset          = 0;   ///< Byte offset of this block in the archive file
    u32 compressed_size = 0;   ///< Compressed block size in bytes
    u32 original_size   = 0;   ///< Original (uncompressed) block size in bytes
    u8  dominant_island = static_cast<u8>(IslandType::Unknown);
};

// ─────────────────────────────────────────────────────────────────────────────
/// Top-level archive header — written at byte offset 0 of every .htx file.
// ─────────────────────────────────────────────────────────────────────────────
struct ArchiveHeader {
    u8           magic[4]          = {'H', 'T', 'X', '\x01'};
    u16          format_version    = ArchiveFormat::VERSION;
    u16          flags             = 0;
    u64          original_size     = 0;    ///< Total uncompressed file size
    u32          num_blocks        = 0;
    u64          dict_offset       = 0;    ///< Byte offset to global dictionary section
    u64          grammar_offset    = 0;    ///< Byte offset to grammar table section
    u8           blake3[32]        = {};   ///< BLAKE3 hash of the original file
};

// ─────────────────────────────────────────────────────────────────────────────
/// Abstract archive writer interface.
///
/// Implementations write the .htx binary format to disk.
// ─────────────────────────────────────────────────────────────────────────────
class IArchiveWriter {
public:
    virtual ~IArchiveWriter() = default;

    /// Open the output file and write the archive header stub.
    [[nodiscard]] virtual Status open(const std::string& path) = 0;

    /// Append a compressed block and update the block index.
    [[nodiscard]] virtual Status write_block(const BlockEntry& entry,
                                              ByteSpan           compressed_data) = 0;

    /// Write the global dictionary section.
    [[nodiscard]] virtual Status write_global_dict(ByteSpan dict_data) = 0;

    /// Write the grammar table section.
    [[nodiscard]] virtual Status write_grammar(ByteSpan grammar_data) = 0;

    /// Finalize the archive: patch the header, write the block index,
    /// compute and write the CRC64, then close the file.
    [[nodiscard]] virtual Status finalize(const ArchiveHeader& header,
                                           const std::vector<BlockEntry>& index) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Abstract archive reader interface.
///
/// Implementations read and validate .htx archives from disk.
// ─────────────────────────────────────────────────────────────────────────────
class IArchiveReader {
public:
    virtual ~IArchiveReader() = default;

    /// Open and validate an archive. Reads and verifies the header magic/version.
    [[nodiscard]] virtual Status open(const std::string& path) = 0;

    /// Read the archive header (after open()).
    [[nodiscard]] virtual const ArchiveHeader& header() const noexcept = 0;

    /// Read the block index (after open()).
    [[nodiscard]] virtual const std::vector<BlockEntry>& block_index() const noexcept = 0;

    /// Read and decompress a specific block by index.
    [[nodiscard]] virtual Result<Block> read_block(u32 block_index) = 0;

    /// Read the global dictionary section.
    [[nodiscard]] virtual Result<ByteVector> read_global_dict() = 0;

    /// Read the grammar table section.
    [[nodiscard]] virtual Result<ByteVector> read_grammar() = 0;

    /// Close the archive file handle.
    virtual void close() noexcept = 0;

    /// Print a human-readable summary of the archive to stdout.
    virtual void print_info() const = 0;
};

} // namespace hypercore
