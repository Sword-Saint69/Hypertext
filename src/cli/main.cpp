// ─────────────────────────────────────────────────────────────────────────────
// main.cpp — HyperCore CLI entry point.
//
// Commands:
//   htx compress   <input> <output> [options]
//   htx decompress <input> <output> [options]
//   htx info       <archive>
//   htx version
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/version.hpp>
#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>
#include <hypercore/core/ICompressor.hpp>
#include <hypercore/core/IArchive.hpp>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace hypercore;

#include <hypercore/analysis/TextAnalyzer.hpp>
#include <hypercore/analysis/FileAnalyzer.hpp>
#include <hypercore/analysis/PatternMiner.hpp>
#include <hypercore/analysis/GrammarBuilder.hpp>
#include <hypercore/core/ContextMixer.hpp>
#include <hypercore/core/CharacterPredictor.hpp>
#include <hypercore/core/RansEncoder.hpp>
#include <hypercore/core/BitWriter.hpp>
#include <hypercore/core/ArchiveWriter.hpp>
#include <cmath>
#include <fstream>
#include <vector>
#include <blake3.h>

// ─────────────────────────────────────────────────────────────────────────────
// Logger setup
// ─────────────────────────────────────────────────────────────────────────────

static void setup_logger(bool verbose, bool quiet) {
    auto logger = spdlog::stdout_color_mt("htx");
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [htx] %v");

    if (quiet) {
        spdlog::set_level(spdlog::level::err);
    } else if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string format_bytes(u64 bytes) {
    if (bytes < 1024)       return std::format("{} B",       bytes);
    if (bytes < 1024*1024)  return std::format("{:.1f} KB",  bytes / 1024.0);
    if (bytes < 1024*1024*1024ull) return std::format("{:.2f} MB", bytes / (1024.0*1024.0));
    return std::format("{:.2f} GB", bytes / (1024.0*1024.0*1024.0));
}

// ─────────────────────────────────────────────────────────────────────────────
// Stub operations (replaced by real compressor in Phase 7)
// ─────────────────────────────────────────────────────────────────────────────

static int cmd_compress(const std::string& input, const std::string& output,
                         int level,
                         bool no_gpu,
                         bool no_grammar,
                         bool analyze_only) {
    spdlog::info("Compressing: {} -> {}", input, output);
    spdlog::info("  Level    : {}", level);
    spdlog::info("  GPU      : {}", !no_gpu ? "enabled" : "disabled");
    spdlog::info("  Grammar  : {}", !no_grammar ? "enabled" : "disabled");

    if (!fs::exists(input)) {
        spdlog::error("Input file not found: {}", input);
        return EXIT_FAILURE;
    }

    const auto input_size = fs::file_size(input);
    spdlog::info("  Input    : {} ({})", input, format_bytes(input_size));

    std::vector<u8> buffer;
    if (input_size > 0) {
        buffer.resize(input_size);
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    }

    ArchiveWriter archive;
    if (!analyze_only) {
        if (!archive.open(output).has_value()) {
            spdlog::error("Failed to open output file: {}", output);
            return EXIT_FAILURE;
        }
    }

    // Thread pool setup
    BS::thread_pool pool;
    size_t block_size = 1024 * 1024; // 1 MB
    size_t num_blocks = (buffer.size() + block_size - 1) / block_size;
    if (num_blocks == 0) num_blocks = 1;

    struct BlockResult {
        BlockEntry entry;
        std::vector<u8> compressed_data;
    };

    std::vector<std::future<BlockResult>> futures;
    spdlog::info("Processing {} block(s) with {} threads...", num_blocks, pool.get_thread_count());

    for (size_t i = 0; i < num_blocks; ++i) {
        size_t offset = i * block_size;
        size_t size = std::min(block_size, buffer.size() - offset);
        
        hypercore::ByteSpan block_span(buffer.data() + offset, size);

        futures.push_back(pool.submit_task([block_span, offset]() -> BlockResult {
            BlockResult result;
            result.entry.offset = 0; 
            result.entry.original_size = static_cast<u32>(block_span.size());
            result.entry.dominant_island = static_cast<u8>(IslandType::Unknown);

            if (block_span.empty()) {
                result.entry.compressed_size = 0;
                return result;
            }

            Block b = Block::from(block_span.data(), static_cast<u32>(block_span.size()));
            
            analysis::TextAnalyzer text_analyzer;
            analysis::AnalysisResult res = text_analyzer.analyze(b.view());
            if (!res.islands.empty()) {
                result.entry.dominant_island = static_cast<u8>(res.islands[0].type);
            }

            analysis::PatternMiner miner;
            if (result.entry.dominant_island == static_cast<u8>(IslandType::Binary)) {
                miner.mine_phrases(b.view().span(), 8);
            } else {
                miner.mine_ngrams(b.view().span(), res.tokens);
            }

            analysis::GrammarBuilder grammar;
            grammar.build_from_bytes(b.view().span());

            ContextMixer mixer;
            mixer.add_predictor(std::make_unique<CharacterPredictor>());

            PredictorContext p_ctx;
            p_ctx.island = static_cast<IslandType>(result.entry.dominant_island);

            BitWriter bit_writer;
            RansEncoder encoder(bit_writer);

            for (u32 j = 0; j < b.size(); ++j) {
                p_ctx.position = j;
                p_ctx.history = ByteSpan(b.data(), j);
                ByteDistribution dist = mixer.predict(p_ctx);
                u8 actual_byte = b.data()[j];
                QuantizedDistribution q_dist = QuantizedDistribution::from(dist);
                encoder.encode_symbol(actual_byte, q_dist);
                mixer.update(p_ctx, actual_byte);
            }
            encoder.flush();

            result.entry.compressed_size = static_cast<u32>(bit_writer.get_buffer().size());
            result.compressed_data = bit_writer.get_buffer();
            return result;
        }));
    }

    std::vector<BlockEntry> block_index;
    u64 current_archive_offset = 68; // ArchiveHeader size

    for (auto& fut : futures) {
        BlockResult res = fut.get();
        if (!analyze_only) {
            res.entry.offset = current_archive_offset;
            if (!archive.write_block(res.entry, ByteSpan(res.compressed_data.data(), res.compressed_data.size())).has_value()) {
                spdlog::error("Failed to write block to archive");
                return EXIT_FAILURE;
            }
            current_archive_offset += res.compressed_data.size();
        }
        block_index.push_back(res.entry);
    }

    if (!analyze_only) {
        ArchiveHeader a_head;
        a_head.num_blocks = static_cast<u32>(block_index.size());
        a_head.original_size = input_size;

        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, buffer.data(), buffer.size());
        blake3_hasher_finalize(&hasher, a_head.blake3, sizeof(a_head.blake3));

        if (!archive.finalize(a_head, block_index).has_value()) {
            spdlog::error("Failed to finalize archive");
            return EXIT_FAILURE;
        }

        spdlog::info("Archive assembled successfully at {}", output);
    }

    return EXIT_SUCCESS;
}

static int cmd_decompress(const std::string& input, const std::string& output) {
    spdlog::info("Decompressing: {} → {}", input, output);

    if (!fs::exists(input)) {
        spdlog::error("Archive not found: {}", input);
        return EXIT_FAILURE;
    }

    // ── Placeholder: real decompression pipeline goes here (Phase 7) ─────────
    spdlog::warn("Decompression pipeline not yet implemented (Phase 7).");
    // ─────────────────────────────────────────────────────────────────────────

    return EXIT_SUCCESS;
}

static int cmd_info(const std::string& archive_path) {
    if (!fs::exists(archive_path)) {
        spdlog::error("Archive not found: {}", archive_path);
        return EXIT_FAILURE;
    }

    const auto archive_size = fs::file_size(archive_path);

    std::cout << "\n";
    std::cout << "  Archive : " << archive_path << "\n";
    std::cout << "  Size    : " << format_bytes(archive_size) << "\n";
    std::cout << "\n";

    // ── Placeholder: real archive parsing goes here (Phase 10) ───────────────
    spdlog::warn(".htx parsing not yet implemented (Phase 10).");
    // ─────────────────────────────────────────────────────────────────────────

    return EXIT_SUCCESS;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    CLI::App app{
        std::format("{} v{} — Hybrid CPU-GPU lossless text compression",
                    hypercore::PROJECT_NAME,
                    hypercore::VERSION_STRING)
    };

    app.set_help_all_flag("--help-all", "Show all help including subcommand options");

    bool verbose = false;
    bool quiet   = false;
    app.add_flag("-v,--verbose", verbose, "Enable verbose/debug output");
    app.add_flag("-q,--quiet",   quiet,   "Suppress all output except errors");

    // ── compress subcommand ───────────────────────────────────────────────────
    auto* compress_cmd = app.add_subcommand("compress", "Compress a file to .htx format");

    std::string compress_input;
    std::string compress_output;
    int         compress_level   = 5;
    bool        no_gpu           = false;
    bool        no_grammar       = false;
    bool        analyze_only     = false;

    compress_cmd->add_option("input",  compress_input,  "Input file path")->required();
    compress_cmd->add_option("output", compress_output, "Output .htx archive path")->required();
    compress_cmd->add_option("-l,--level", compress_level,
                             "Compression level: 1 (fast) – 9 (best)")->check(CLI::Range(1, 9));
    compress_cmd->add_flag("--no-gpu",     no_gpu,     "Disable GPU acceleration");
    compress_cmd->add_flag("--no-grammar", no_grammar, "Disable grammar compression");
    compress_cmd->add_flag("--analyze-only", analyze_only, "Run Phase 3 Text Analysis on the first block and exit");

    // ── decompress subcommand ─────────────────────────────────────────────────
    auto* decompress_cmd = app.add_subcommand("decompress", "Decompress a .htx archive");

    std::string decompress_input;
    std::string decompress_output;

    decompress_cmd->add_option("input",  decompress_input,  "Input .htx archive")->required();
    decompress_cmd->add_option("output", decompress_output, "Output file path")->required();

    // ── info subcommand ───────────────────────────────────────────────────────
    auto* info_cmd = app.add_subcommand("info", "Display information about a .htx archive");

    std::string info_archive;
    info_cmd->add_option("archive", info_archive, "Path to .htx archive")->required();

    // ── version subcommand ────────────────────────────────────────────────────
    auto* version_cmd = app.add_subcommand("version", "Print version and build information");

    // Require at least one subcommand
    app.require_subcommand(1);

    CLI11_PARSE(app, argc, argv);

    setup_logger(verbose, quiet);

    // Dispatch
    if (*compress_cmd) {
        return cmd_compress(compress_input, compress_output,
                             compress_level, no_gpu, no_grammar, analyze_only);
    }
    if (*decompress_cmd) {
        return cmd_decompress(decompress_input, decompress_output);
    }
    if (*info_cmd) {
        return cmd_info(info_archive);
    }
    if (*version_cmd) {
        std::cout << std::format("{} version {}\n", PROJECT_NAME, VERSION_STRING);
        std::cout << std::format("  C++ standard : C++23\n");
#if defined(HYPERCORE_CUDA_ENABLED)
        std::cout << std::format("  GPU support  : CUDA (enabled)\n");
#else
        std::cout << std::format("  GPU support  : CPU-only build\n");
#endif
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
