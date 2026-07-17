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

static int cmd_compress(const std::string& input,
                         const std::string& output,
                         int level,
                         bool no_gpu,
                         bool no_grammar,
                         bool analyze_only) {
    spdlog::info("Compressing: {} → {}", input, output);
    spdlog::info("  Level    : {}", level);
    spdlog::info("  GPU      : {}", !no_gpu ? "enabled" : "disabled");
    spdlog::info("  Grammar  : {}", !no_grammar ? "enabled" : "disabled");

    if (!fs::exists(input)) {
        spdlog::error("Input file not found: {}", input);
        return EXIT_FAILURE;
    }

    const auto input_size = fs::file_size(input);
    spdlog::info("  Input    : {} ({})", input, format_bytes(input_size));

    if (analyze_only) {
        spdlog::info("Running Phase 3 File Analysis...");
        std::ifstream file(input, std::ios::binary);
        std::vector<u8> buffer(std::min<u64>(input_size, 256 * 1024));
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        analysis::FileAnalyzer file_analyzer;
        auto meta = file_analyzer.analyze_buffer(ByteSpan(buffer.data(), buffer.size()));
        
        spdlog::info("File Properties:");
        spdlog::info("  Total Bytes   : {}", meta.total_bytes);
        spdlog::info("  Is Binary     : {}", meta.is_binary ? "Yes" : "No");
        spdlog::info("  Is Valid UTF-8: {}", meta.is_valid_utf8 ? "Yes" : "No");
        if (meta.global_island_hint != IslandType::Unknown) {
            spdlog::info("  Global Hint   : {}", to_string(meta.global_island_hint));
        }

        if (!meta.is_binary) {
            spdlog::info("Running Text Analysis on first block...");
            Block b = Block::from(buffer.data(), static_cast<u32>(buffer.size()));
            analysis::TextAnalyzer text_analyzer;
            analysis::AnalysisResult res = text_analyzer.analyze(b.view());
            
            spdlog::info("Analysis Results:");
            spdlog::info("  Tokens        : {}", res.tokens.size());
            spdlog::info("  Alpha Ratio   : {:.2f}", res.alpha_ratio);
            spdlog::info("  Digit Ratio   : {:.2f}", res.digit_ratio);
            spdlog::info("  WS Ratio      : {:.2f}", res.ws_ratio);
            spdlog::info("  Punct Ratio   : {:.2f}", res.punct_ratio);
            spdlog::info("  Bracket Dens. : {:.2f}", res.bracket_density);
            
            if (!res.islands.empty()) {
                spdlog::info("  Primary Island: {}", to_string(res.islands[0].type));
            }

            spdlog::info("Running Pattern Mining...");
            analysis::PatternMiner miner;
            if (meta.global_island_hint == IslandType::Binary) {
                miner.mine_phrases(b.view().span(), 8);
            } else {
                miner.mine_ngrams(b.view().span(), res.tokens);
            }
            
            auto top_candidates = miner.extract_top_candidates(5);
            spdlog::info("Top 5 Dictionary Candidates:");
            for (const auto& c : top_candidates) {
                std::string display_seq = c.sequence;
                // Replace newlines with spaces for clean printing
                std::replace(display_seq.begin(), display_seq.end(), '\n', ' ');
                std::replace(display_seq.begin(), display_seq.end(), '\r', ' ');
                if (display_seq.length() > 40) {
                    display_seq = display_seq.substr(0, 37) + "...";
                }
                spdlog::info("  [{}] (freq: {}, utility: {})", display_seq, c.frequency, c.utility_score);
            }

            spdlog::info("Running Grammar Building (Re-Pair)...");
            analysis::GrammarBuilder grammar;
            grammar.build_from_bytes(b.view().span());
            
            spdlog::info("Grammar Results:");
            spdlog::info("  Original Length : {}", b.size());
            spdlog::info("  Compressed Len  : {}", grammar.get_compressed_sequence().size());
            spdlog::info("  Rules Generated : {}", grammar.get_rules().size());

            spdlog::info("Running Prediction Engine Simulation...");
            ContextMixer mixer;
            mixer.add_predictor(std::make_unique<CharacterPredictor>());
            
            PredictorContext p_ctx;
            p_ctx.island = meta.global_island_hint;
            
            f64 total_entropy = 0.0;
            const u8* data = b.data();

            // Set up actual physical rANS encoding
            BitWriter bit_writer;
            RansEncoder encoder(bit_writer);

            for (u32 i = 0; i < b.size(); ++i) {
                p_ctx.position = i;
                p_ctx.history = ByteSpan(data, i); // Up to current byte
                
                ByteDistribution dist = mixer.predict(p_ctx);
                u8 actual_byte = data[i];
                
                // Add the negative log2 probability to our entropy sum (cross-entropy)
                f32 p = dist[actual_byte];
                if (p > 0.0f) {
                    total_entropy += -std::log2(p);
                } else {
                    total_entropy += 32.0; // penalty for zero probability
                }
                
                // Quantize and encode!
                QuantizedDistribution q_dist = QuantizedDistribution::from(dist);
                encoder.encode_symbol(actual_byte, q_dist);

                // Update the model with what actually happened
                mixer.update(p_ctx, actual_byte);
            }
            
            encoder.flush();

            f64 bpb = total_entropy / b.size();
            spdlog::info("Prediction Results:");
            spdlog::info("  Total Entropy   : {:.2f} bits", total_entropy);
            spdlog::info("  Bits Per Byte   : {:.3f} bpb", bpb);
            spdlog::info("  Estimated Ratio : {:.2f}x", 8.0 / bpb);

            spdlog::info("Entropy Coding (rANS) Results:");
            spdlog::info("  Encoded Bytes   : {}", bit_writer.get_buffer().size());
            f64 actual_bpb = static_cast<f64>(bit_writer.get_total_bits()) / b.size();
            spdlog::info("  Actual bpb      : {:.3f}", actual_bpb);
            spdlog::info("  Actual Ratio    : {:.2f}x", b.size() / static_cast<f64>(bit_writer.get_buffer().size()));

            if (!analyze_only) {
                spdlog::info("Running Phase 8 Archive Assembly...");
                ArchiveWriter archive;
                if (!archive.open(output_path).is_ok()) {
                    spdlog::error("Failed to open output file: {}", output_path);
                    return EXIT_FAILURE;
                }

                BlockEntry b_entry;
                b_entry.original_size = b.size();
                b_entry.compressed_size = bit_writer.get_buffer().size();
                b_entry.dominant_island = static_cast<u8>(meta.global_island_hint);
                
                // Currently writing block data
                archive.write_block(b_entry, ByteSpan(bit_writer.get_buffer().data(), bit_writer.get_buffer().size()));

                // For now, no dictionary/grammar sections written as bytes, we will do that in later phases.
                ArchiveHeader a_head;
                a_head.num_blocks = 1;
                a_head.original_size = meta.total_bytes;
                // Leave BLAKE3 zeroed out as planned.
                
                std::vector<BlockEntry> b_index = { b_entry };
                archive.finalize(a_head, b_index);

                spdlog::info("Archive assembled successfully at {}", output_path);
            }
        }
        return EXIT_SUCCESS;
    }

    // ── Placeholder: real compression pipeline goes here (Phase 7) ───────────
    spdlog::warn("Compression pipeline not yet implemented (Phase 7).");
    spdlog::warn("This is the Phase 2 skeleton — htx infrastructure only.");
    // ─────────────────────────────────────────────────────────────────────────

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
