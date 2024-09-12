# Score-P Intel PMT Plugin

This repository provides a Score-P plugin to access the metrics provided by the Intel Platform Monitoring Technology[1].

## Prerequisites

- Score-P

libintelpmt prerequisites:
- Python 3
    - lxml
    - jinja2
Python, python-lxml and python-jinja2 are required to generate the C++ classes from Intels PMT XML definition files.

## Installation


```
git clone --recursive-submodules https://github.com/score-p/scorep_plugin_intelpmt.git
cd scorep_plugin_intelpmt
mkdir build & cd build
cmake ../
make

#copy the resulting libintelpmt_plugin.so into your LD_LIBRARY_PATH
```

## Usage

```
export SCOREP_METRIC_PLUGINS=intelpmt_plugin
export SCOREP_METRIC_INTEL_PMT_PLUGIN="/sys/class/intel_pmt/telem1::TEMPERATURE[0]::CORE_TEMP"
```

Metrics are given in the form of:

```
[sysfs path]::[metric name]
```

Where `[sysfs path]` is the sysfs path to the intel_pmt device (usually of the form `/sys/class/intel_pmt/telem*`) and `[metric name]` is the name of the metric.

A list of metrics can be retrieved with the `pmttool list` command. `pmttool` is part of the libintelpmt repository.

[1] https://research.spec.org/icpe_proceedings/2024/proceedings/p95.pdf
