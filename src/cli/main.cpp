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
        spdlog::info("Running Phase 3 Analysis on first block...");
        std::ifstream file(input, std::ios::binary);
        std::vector<u8> buffer(std::min<u64>(input_size, 256 * 1024));
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        Block b = Block::from(buffer.data(), static_cast<u32>(buffer.size()));
        analysis::TextAnalyzer analyzer;
        analysis::AnalysisResult res = analyzer.analyze(b.view());
        
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
