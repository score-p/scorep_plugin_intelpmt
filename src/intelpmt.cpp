
#include <scorep/plugin/plugin.hpp>

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <regex>
#include <thread>
#include <vector>

#include <libintelpmt/libintelpmt.hpp>

using pmt_id = std::pair<std::filesystem::path, uint64_t>;

using TVPair = std::pair<scorep::chrono::ticks, double>;

class IntelPMTMeasurementThread
{
public:
    IntelPMTMeasurementThread(intelpmt::Device& dev, std::chrono::milliseconds interval)
    : instance_(dev.open()), interval_(interval)
    {
    }

    void add_counter(uint64_t counter)
    {
        counters_.emplace_back(counter);
        data_[counter] = std::vector<TVPair>();
    }

    void measurement()
    {
        while (!stop_)
        {
            for (auto& counter : counters_)
            {
                std::lock_guard<std::mutex> lock(read_mutex_);

                double value = instance_.read_counter(counter);

                data_.at(counter).emplace_back(scorep::chrono::measurement_clock::now(), value);
            }
            std::this_thread::sleep_for(interval_);
        }
    }

    void start()
    {
        thread_ = std::thread([this]() { this->measurement(); });
    }

    void stop()
    {
        stop_ = true;

        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    std::vector<TVPair> get_values_for_sensor(uint64_t counter)
    {
        std::lock_guard<std::mutex> lock(read_mutex_);
        auto ret = data_.at(counter);
        data_.at(counter).clear();
        return ret;
    }

private:
    bool stop_ = false;
    std::mutex read_mutex_;
    intelpmt::DeviceInstance instance_;
    std::vector<uint64_t> counters_;
    std::chrono::milliseconds interval_;
    std::map<uint64_t, std::vector<TVPair>> data_;
    std::thread thread_;
};

using namespace scorep::plugin::policy;
using scorep::plugin::logging;

template <typename T, typename Policies>
using intelpmt_object_id = object_id<pmt_id, T, Policies>;

class intelpmt_plugin
: public scorep::plugin::base<intelpmt_plugin, async, once, scorep_clock, intelpmt_object_id>
{
public:
    intelpmt_plugin()
    : measurement_interval_(
          std::chrono::milliseconds(stoi(scorep::environment_variable::get("interval", "50"))))
    {
    }

    std::vector<scorep::plugin::metric_property>
    get_metric_properties(const std::string& metric_name)
    {
        std::vector<scorep::plugin::metric_property> counters;

        std::regex counter_regex("([^:]+)::(.+)");
        std::smatch counter_match;

        if (std::regex_match(metric_name, counter_match, counter_regex))
        {
            std::string metric_path = counter_match[1];
            auto& devices = intelpmt::get_pmt_devices();
            auto dev = std::find_if(devices.begin(), devices.end(), [metric_path](auto& arg) {
                return arg.get_path() == metric_path;
            });

            if (dev == devices.end())
            {
                logging::warn() << "Unknown device: " << metric_path << ", ignored";
                return {};
            }

            if (pmts_.count(metric_path) == 0)
            {
                pmts_.emplace(std::piecewise_construct, std::forward_as_tuple(metric_path),
                              std::forward_as_tuple(*dev, measurement_interval_));
            }

            if (counter_match[2] == "*")
            {
                for (auto& counter : dev->get_counter_names())
                {
                    pmts_.at(metric_path).add_counter(counter.second);

                    counters.push_back(
                        scorep::plugin::metric_property(metric_name, metric_name,
                                                        dev->get_units().at(counter.second).unit)
                            .absolute_point()
                            .value_double());

                    make_handle(metric_name, metric_path, counter.second);
                }
            }
            else
            {
                std::string counter_name = counter_match[2];
                auto& counter_names = dev->get_counter_names();
                auto counter =
                    std::find_if(counter_names.begin(), counter_names.end(),
                                 [&counter_name](auto arg) { return arg.first == counter_name; });

                if (counter != counter_names.end())
                {

                    pmts_.at(metric_path).add_counter(counter->second);

                    counters.push_back(
                        scorep::plugin::metric_property(metric_name, metric_name,
                                                        dev->get_units().at(counter->second).unit)
                            .absolute_point()
                            .value_double());

                    make_handle(metric_name, metric_path, counter->second);
                }
                else
                {
                    logging::warn() << "Could not find counter: " << counter_name << " in device "
                                    << metric_path;
                }
            }
        }
        else
        {
            logging::warn() << "Invalid sensor: " << metric_name << ", ignored";
        }
        return counters;
    }

    template <typename C>
    void get_all_values(pmt_id& id, C& cursor)
    {
        std::vector<TVPair> values = pmts_.at(id.first).get_values_for_sensor(id.second);

        for (auto& value : values)
        {
            cursor.write(value.first, value.second);
        }
    }

    void add_metric(pmt_id& id)
    {
    }

    void start()
    {
        for (auto& pmt : pmts_)
        {
            pmt.second.start();
        }
    }

    void stop()
    {
        for (auto& pmt : pmts_)
        {
            pmt.second.stop();
        }
    }

private:
    std::chrono::milliseconds measurement_interval_;
    std::map<std::filesystem::path, IntelPMTMeasurementThread> pmts_;
};

SCOREP_METRIC_PLUGIN_CLASS(intelpmt_plugin, "intelpmt")
