//! A feature-tour example: computes Fibonacci sequences while demonstrating
//! most of `alog`'s features (channels, levels, filters, scoped logs,
//! timers, metadata, and map data).
//!
//! Configure it via environment variables:
//!
//!   ALOG_DEFAULT_LEVEL    - default level for all channels (default: "info")
//!   ALOG_FILTERS          - per-channel overrides, e.g. "FIB:debug4"
//!   ALOG_USE_JSON         - "true" to use the JSON formatter (default: pretty)
//!   ALOG_ENABLE_THREAD_ID - "true" to include a thread id on every record
//!
//! Run it with, e.g.:
//!
//!   ALOG_DEFAULT_LEVEL=debug4 cargo run --example fib -- 5 8 3
//!
//! With the `disable-logging` feature enabled, every macro call below
//! becomes a no-op, which leaves several bindings computed only for logging
//! (e.g. `start_msg`, `rendered`) genuinely unused - hence the blanket
//! `allow` below for that feature only.
#![cfg_attr(feature = "disable-logging", allow(unused))]

use alog::{
    alog, alog_channel, alog_fn_channel, alog_is_enabled, alog_map, alog_scoped_block,
    alog_scoped_metadata, alog_scoped_timer, alog_scoped_timer_channel, use_channel,
};
use alog::{Config, Filters, FormatterKind, Level, MapData, MessageLevel, Writer};
use serde_json::json;
use std::env;
use std::process::ExitCode;
use std::thread::JoinHandle;

// TUTORIAL: `use_channel!` declares a free function, `__alog_channel()`, in
// this module. `alog_channel!` calls it implicitly, so call sites in this
// module don't need to repeat the channel name.
use_channel!("FIB");

/// Computes the first `n` terms of the Fibonacci sequence, sleeping briefly
/// on each term to simulate real work.
fn fib(n: u32) -> Vec<u64> {
    // TUTORIAL: `alog_fn_channel!` opens a Trace-level scope named after the
    // enclosing function, using the channel bound by `use_channel!` above
    // instead of repeating it. It logs "BEGIN: fib(n)" now and "END: fib(n)"
    // when the scope drops - including on an early return or unwind. Every
    // user-facing macro that takes a channel has one of these `_channel`
    // siblings; use `alog_fn!(channel, ...)` instead when you want an
    // explicit channel, as `main` does below with "MAIN".
    alog_fn_channel!("{n}");

    // TUTORIAL: `alog_scoped_timer_channel!` binds a `ScopedTimer` to a
    // hidden variable for you, using the bound "FIB" channel: it starts a
    // clock now and, when dropped, logs the elapsed time with a
    // `duration_ms` field attached.
    let start_msg = format!("Computed sequence of length {n} in ");
    alog_scoped_timer_channel!(MessageLevel::Debug, "{start_msg}");

    let mut first: u64 = 0;
    let mut second: u64 = 1;
    let mut out = Vec::with_capacity(n as usize);

    for c in 0..n {
        // TUTORIAL: `alog_map!` attaches an arbitrary JSON map to a single
        // log record. We use `debug4` here since this fires on every loop
        // iteration and would otherwise be very noisy.
        alog_map!(
            "FIB",
            MessageLevel::Debug4,
            MapData::from_iter([
                ("c".to_string(), json!(c)),
                ("first".to_string(), json!(first)),
                ("second".to_string(), json!(second)),
            ]),
            "loop iteration"
        );

        let next = if c <= 1 {
            c as u64
        } else {
            let next = first + second;
            first = second;
            second = next;
            next
        };
        // Simulate this being expensive.
        std::thread::sleep(std::time::Duration::from_millis(next.min(20)));
        out.push(next);
    }

    alog_map!(
        "FIB",
        MessageLevel::Debug3,
        MapData::from_iter([
            ("first".to_string(), json!(first)),
            ("second".to_string(), json!(second)),
        ]),
        "final variable state"
    );

    out
}

/// Fans work out across threads and collects the results, mirroring the
/// `cpp` example's `std::async`-based `FibonacciCalculator`.
struct FibonacciCalculator {
    handles: Vec<JoinHandle<Vec<u64>>>,
}

impl FibonacciCalculator {
    fn new() -> Self {
        Self {
            handles: Vec::new(),
        }
    }

    fn add_sequence_length(&mut self, n: u32) {
        // TUTORIAL: `ScopedMetadata` attaches key/value pairs to every
        // record logged *on this thread* while the guard is alive - like
        // indentation, metadata is thread-local. `fib` runs on a freshly
        // spawned thread below, so this won't appear on its records, but it
        // will appear on the "queuing job" record logged right here.
        let job_number = self.handles.len() + 1;
        let mut metadata = MapData::new();
        metadata.insert("job_number".to_string(), json!(job_number));
        alog_scoped_metadata!(metadata);

        alog_channel!(MessageLevel::Debug, "queuing job");

        // TUTORIAL: Top-level interface functions use `alog_fn_channel!` to
        // add a Trace-level BEGIN/END pair around the whole call. Since this
        // file bound `use_channel!("FIB")` at module scope, both `fib` and
        // `FibonacciCalculator`'s methods log on the "FIB" channel; only
        // `main`, below, logs on "MAIN" explicitly.
        alog_fn_channel!("{n}");
        self.handles.push(std::thread::spawn(move || fib(n)));
    }

    fn get_results(mut self) -> Vec<Vec<u64>> {
        alog_fn_channel!();
        alog_scoped_timer_channel!(MessageLevel::Info, "Finished all jobs in ");

        let mut out = Vec::new();
        for (i, handle) in self.handles.drain(..).enumerate() {
            alog_channel!(MessageLevel::Debug2, "waiting on job {}", i + 1);
            out.push(handle.join().expect("fib worker thread panicked"));
        }
        out
    }
}

fn load_env_string(key: &str, default: &str) -> String {
    env::var(key).unwrap_or_else(|_| default.to_string())
}

fn load_env_bool(key: &str, default: bool) -> bool {
    env::var(key)
        .map(|v| v.eq_ignore_ascii_case("true"))
        .unwrap_or(default)
}

fn main() -> ExitCode {
    // Read configuration from the environment.
    let default_level_str = load_env_string("ALOG_DEFAULT_LEVEL", "info");
    let filters_str = load_env_string("ALOG_FILTERS", "");
    let use_json = load_env_bool("ALOG_USE_JSON", false);
    let enable_thread_id = load_env_bool("ALOG_ENABLE_THREAD_ID", false);

    // TUTORIAL: This demonstrates all of the standard configuration
    // features of `alog`:
    //   * default_level: the level enabled for any channel not named below
    //   * filters:        per-channel level overrides, e.g. "FIB:debug4"
    //   * formatter:      Pretty (for humans) or Json (for aggregation)
    //   * thread_id:      if true, every record includes the thread id
    let default_level: Level = default_level_str.parse().unwrap_or(Level::Info);
    let filters = if filters_str.is_empty() {
        Filters::None
    } else {
        Filters::Spec(filters_str)
    };
    alog::configure(Config {
        default_level,
        filters,
        formatter: if use_json {
            FormatterKind::Json
        } else {
            FormatterKind::Pretty
        },
        writer: Writer::Stdout,
        thread_id: enable_thread_id,
    });

    // TUTORIAL: When logging with no channel bound via `use_channel!`,
    // simply provide the channel name as `alog!`'s first argument.
    alog!("MAIN", MessageLevel::Info, "Logging Configured");
    alog!("MAIN", MessageLevel::Debug, "Hello World");

    // Parse command line args as sequence lengths.
    let mut sequence_lengths = Vec::new();
    {
        // TUTORIAL: `alog_scoped_block!` binds a `ScopedLog` to a hidden
        // variable for you, wrapping a logically grouped set of actions in
        // BEGIN/END log lines - here, parsing the command line.
        alog_scoped_block!("MAIN", MessageLevel::Debug, "Parsing Command Line");

        for (i, arg) in env::args().skip(1).enumerate() {
            alog!("MAIN", MessageLevel::Debug2, "Parsing argument {}", i + 1);
            match arg.parse::<u32>() {
                Ok(val) => {
                    alog!("MAIN", MessageLevel::Debug2, "Parsed value [{val}]");
                    sequence_lengths.push(val);
                }
                Err(_) => {
                    alog!("MAIN", MessageLevel::Fatal, "Invalid argument [{arg}]");
                    return ExitCode::FAILURE;
                }
            }
        }
        if sequence_lengths.is_empty() {
            alog!(
                "MAIN",
                MessageLevel::Fatal,
                "Must provide at least one sequence length argument"
            );
            return ExitCode::FAILURE;
        }
    }

    let mut calculator = FibonacciCalculator::new();
    {
        alog_scoped_timer!("MAIN", MessageLevel::Debug, "Done adding sequences in ");
        for length in sequence_lengths {
            calculator.add_sequence_length(length);
        }
    }

    let results = calculator.get_results();
    for sequence in results {
        // TUTORIAL: When constructing a log message requires more than a
        // single expression, guard the work with `alog_is_enabled!` so it's
        // skipped entirely when the channel/level isn't enabled.
        if alog_is_enabled!("MAIN", MessageLevel::Info) {
            let rendered = sequence
                .iter()
                .map(|n| n.to_string())
                .collect::<Vec<_>>()
                .join(" ");
            alog!("MAIN", MessageLevel::Info, "[ {rendered} ]");
        }
    }

    ExitCode::SUCCESS
}
