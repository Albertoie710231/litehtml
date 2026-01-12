#ifndef LH_LAYOUT_PROFILER_H
#define LH_LAYOUT_PROFILER_H

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <mutex>

namespace litehtml {

// Simple profiler for layout timing
// Enable with LITEHTML_PROFILE_LAYOUT define
#ifdef LITEHTML_PROFILE_LAYOUT

class layout_profiler {
public:
    static layout_profiler& instance() {
        static layout_profiler p;
        return p;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_times.clear();
        m_counts.clear();
        m_start_time = std::chrono::steady_clock::now();
    }

    void add(const std::string& name, double ms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_times[name] += ms;
        m_counts[name]++;
    }

    void print() {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto total = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - m_start_time).count();

        std::cout << "\n=== Layout Profile ===" << std::endl;
        std::cout << "Total time: " << total << "ms" << std::endl;

        // Sort by time
        std::vector<std::pair<std::string, double>> sorted(m_times.begin(), m_times.end());
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& [name, time] : sorted) {
            double percent = (time / total) * 100;
            std::cout << "  " << name << ": " << time << "ms ("
                      << percent << "%) [" << m_counts[name] << " calls]" << std::endl;
        }
        std::cout << "=====================\n" << std::endl;
    }

private:
    layout_profiler() { reset(); }
    std::map<std::string, double> m_times;
    std::map<std::string, int> m_counts;
    std::chrono::steady_clock::time_point m_start_time;
    std::mutex m_mutex;
};

class scoped_timer {
public:
    scoped_timer(const std::string& name) : m_name(name),
        m_start(std::chrono::steady_clock::now()) {}

    ~scoped_timer() {
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - m_start).count();
        layout_profiler::instance().add(m_name, ms);
    }
private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;
};

#define PROFILE_SCOPE(name) litehtml::scoped_timer _timer_##__LINE__(name)
#define PROFILE_RESET() litehtml::layout_profiler::instance().reset()
#define PROFILE_PRINT() litehtml::layout_profiler::instance().print()

#else
// No-op when profiling disabled
#define PROFILE_SCOPE(name)
#define PROFILE_RESET()
#define PROFILE_PRINT()
#endif

} // namespace litehtml

#endif // LH_LAYOUT_PROFILER_H
