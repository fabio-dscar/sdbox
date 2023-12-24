#include <log.h>

#include <thread.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/fmt_helper.h>
#include <spdlog/formatter.h>
#include <spdlog/pattern_formatter.h>

#include <backward.hpp>

using namespace sdbox;

class thread_name_flag : public spdlog::custom_flag_formatter {
public:
    void
    format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
        auto name = GetThreadName();
        dest.append(name.data(), name.data() + name.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<thread_name_flag>();
    }
};

void sdbox::InitLogger() {
    auto cliFormatter = std::make_unique<spdlog::pattern_formatter>();
    cliFormatter->add_flag<thread_name_flag>('W');
    cliFormatter->set_pattern("[%T.%F][%^%l%$][%=20W] %v");

    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console->set_level(spdlog::level::info);
    console->set_formatter(std::move(cliFormatter));

    auto fileFormatter = std::make_unique<spdlog::pattern_formatter>();
    fileFormatter->add_flag<thread_name_flag>('W');
    fileFormatter->set_pattern("[%D %T.%F][%^%l%$][%W] %v");

    auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
    file->set_level(spdlog::level::trace);
    file->set_formatter(std::move(fileFormatter));

    auto sinks  = spdlog::sinks_init_list{console, file};
    auto logger = std::make_shared<spdlog::logger>("main", sinks);
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::err);

    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(10));
}

std::shared_ptr<sdbox::Logger> sdbox::GetLogger(const std::string& name) {
    return spdlog::get(name);
}

void sdbox::PrintStackTrace() {
    auto filter = [&](const backward::ResolvedTrace& trace) -> bool {
        return trace.source.filename.find("backward.hpp") == std::string::npos &&
               trace.object_function.find("PrintStackTrace") == std::string::npos;
    };

    backward::TraceResolver resolver;
    backward::StackTrace    st;
    st.load_here();

    std::vector<backward::ResolvedTrace> traces;
    for (int i = st.size() - 1; i >= 0; --i) {
        auto resolved = resolver.resolve(st[i]);
        if (filter(resolved))
            traces.push_back(resolved);
    }

    for (std::size_t i = 0; i < traces.size(); ++i)
        traces[i].idx = traces.size() - 1 - i;

    backward::Printer printer;
    printer.snippet              = true;
    printer.color_mode           = backward::ColorMode::always;
    printer.address              = true;
    printer.object               = true;
    printer.inliner_context_size = 8;
    printer.trace_context_size   = 8;

    std::ostringstream oss;
    printer.print(traces.begin(), traces.end(), oss);

    auto logger = spdlog::default_logger();
    logger->critical(oss.str());
}