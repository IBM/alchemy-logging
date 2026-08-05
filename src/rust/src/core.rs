//! The global singleton: configuration, filtering, and record dispatch.

use std::collections::HashMap;
use std::io::{self, Write};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Mutex, OnceLock, RwLock};
use std::time::{SystemTime, UNIX_EPOCH};

use serde_json::Value;

use crate::formatter::{Formatter, JsonFormatter, PrettyFormatter};
use crate::level::{Level, MessageLevel};
use crate::record::{LogRecord, MapData};
use crate::scope;

/// Per-channel level filters, either as an explicit map or as a spec string
/// of the form `"CHAN1:info,CHAN2:debug"`.
#[derive(Debug, Clone, Default)]
pub enum Filters {
    Map(HashMap<String, Level>),
    Spec(String),
    #[default]
    None,
}

impl Filters {
    fn into_map(self) -> HashMap<String, Level> {
        match self {
            Filters::Map(m) => m,
            Filters::None => HashMap::new(),
            Filters::Spec(s) => parse_filter_spec(&s),
        }
    }
}

fn parse_filter_spec(spec: &str) -> HashMap<String, Level> {
    let mut map = HashMap::new();
    for entry in spec.split(',') {
        if entry.is_empty() {
            continue;
        }
        let mut parts = entry.splitn(2, ':');
        if let (Some(chan), Some(level_str)) = (parts.next(), parts.next()) {
            if let Ok(level) = level_str.parse::<Level>() {
                map.insert(chan.to_string(), level);
            }
        }
    }
    map
}

/// The output formatter to configure the crate with.
#[derive(Default)]
pub enum FormatterKind {
    #[default]
    Pretty,
    Json,
    Custom(Box<dyn Formatter>),
}

impl FormatterKind {
    fn into_formatter(self) -> Box<dyn Formatter> {
        match self {
            FormatterKind::Pretty => Box::new(PrettyFormatter::default()),
            FormatterKind::Json => Box::new(JsonFormatter),
            FormatterKind::Custom(f) => f,
        }
    }
}

/// The sink that formatted records are written to.
#[derive(Default)]
pub enum Writer {
    #[default]
    Stdout,
    Custom(Box<dyn Write + Send>),
}

/// Top-level configuration for the crate. Construct with [`Config::default`]
/// and override only the fields you need.
///
/// ```
/// alog::configure(alog::Config {
///     default_level: alog::Level::Debug,
///     ..Default::default()
/// });
/// ```
#[derive(Default)]
pub struct Config {
    pub default_level: Level,
    pub filters: Filters,
    pub formatter: FormatterKind,
    pub writer: Writer,
    pub thread_id: bool,
}

struct ConfigState {
    default_level: Level,
    filters: HashMap<String, Level>,
    thread_id_enabled: bool,
}

struct SinkState {
    formatter: Box<dyn Formatter>,
    sink: Box<dyn Write + Send>,
}

static CONFIG: OnceLock<RwLock<ConfigState>> = OnceLock::new();
static SINK: OnceLock<Mutex<SinkState>> = OnceLock::new();

fn config() -> &'static RwLock<ConfigState> {
    CONFIG.get_or_init(|| {
        RwLock::new(ConfigState {
            default_level: Level::Info,
            filters: HashMap::new(),
            thread_id_enabled: false,
        })
    })
}

fn sink() -> &'static Mutex<SinkState> {
    SINK.get_or_init(|| {
        Mutex::new(SinkState {
            formatter: Box::new(PrettyFormatter::default()),
            sink: Box::new(io::stdout()),
        })
    })
}

/// Configure the crate's global logging behavior. May be called multiple
/// times at runtime; each call fully replaces the previous configuration.
pub fn configure(cfg: Config) {
    let filters = cfg.filters.into_map();
    let formatter = cfg.formatter.into_formatter();
    let writer: Box<dyn Write + Send> = match cfg.writer {
        Writer::Stdout => Box::new(io::stdout()),
        Writer::Custom(w) => w,
    };

    {
        let mut state = config().write().unwrap();
        state.default_level = cfg.default_level;
        state.filters = filters;
        state.thread_id_enabled = cfg.thread_id;
    }
    {
        let mut sink_state = sink().lock().unwrap();
        sink_state.formatter = formatter;
        sink_state.sink = writer;
    }
}

/// Adjust only the default level and per-channel filters, leaving the
/// configured formatter and sink untouched.
pub fn adjust_levels(default_level: Level, filters: Filters) {
    let filters = filters.into_map();
    let mut state = config().write().unwrap();
    state.default_level = default_level;
    state.filters = filters;
}

/// Returns true if `channel` is enabled at `level` under the current
/// configuration.
pub fn is_enabled(channel: &str, level: MessageLevel) -> bool {
    let state = config().read().unwrap();
    let filter_level = state
        .filters
        .get(channel)
        .copied()
        .unwrap_or(state.default_level);
    filter_level >= Level::from(level)
}

thread_local! {
    static THREAD_ID: u64 = next_thread_id();
}
static THREAD_ID_COUNTER: AtomicU64 = AtomicU64::new(1);

fn next_thread_id() -> u64 {
    THREAD_ID_COUNTER.fetch_add(1, Ordering::Relaxed)
}

/// The function every logging macro funnels into. Not part of the public
/// API: use the `alog!`/`alog_map!` macros instead.
#[doc(hidden)]
pub fn __log_impl(channel: &str, level: MessageLevel, message: String, extra: Option<MapData>) {
    let thread_id_enabled = config().read().unwrap().thread_id_enabled;
    let thread_id_str = thread_id_enabled.then(|| THREAD_ID.with(|id| id.to_string()));

    let mut merged = extra.unwrap_or_default();
    if let Some(metadata) = scope::metadata_snapshot() {
        merged.insert("metadata".to_string(), Value::Object(metadata));
    }
    let final_extra = if merged.is_empty() {
        None
    } else {
        Some(merged)
    };

    let timestamp = iso8601_now();
    let record = LogRecord {
        channel,
        level,
        timestamp: &timestamp,
        message: &message,
        num_indent: scope::indent_level(),
        thread_id: thread_id_str.as_deref(),
        extra: final_extra.as_ref(),
    };

    let mut sink_state = sink().lock().unwrap();
    let formatted = sink_state.formatter.format(&record);
    let _ = sink_state.sink.write_all(formatted.as_bytes());
}

/// Formats the current time as an ISO 8601 timestamp with millisecond
/// precision (`YYYY-MM-DDTHH:mm:ss.sssZ`), hand-rolled from [`SystemTime`]
/// to avoid a date/time dependency.
fn iso8601_now() -> String {
    let since_epoch = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default();
    let secs = since_epoch.as_secs();
    let millis = since_epoch.subsec_millis();

    let days = (secs / 86400) as i64;
    let rem = secs % 86400;
    let hour = rem / 3600;
    let minute = (rem % 3600) / 60;
    let second = rem % 60;

    let (year, month, day) = civil_from_days(days);
    format!(
        "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:03}Z",
        year, month, day, hour, minute, second, millis
    )
}

/// Converts a count of days since the Unix epoch into a (year, month, day)
/// civil calendar date. Adapted from Howard Hinnant's public-domain
/// `civil_from_days` algorithm
/// (<http://howardhinnant.github.io/date_algorithms.html>).
fn civil_from_days(z: i64) -> (i64, u32, u32) {
    let z = z + 719468;
    let era = if z >= 0 { z } else { z - 146096 } / 146097;
    let doe = (z - era * 146097) as u64;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y = yoe as i64 + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = (doy - (153 * mp + 2) / 5 + 1) as u32;
    let m = if mp < 10 { mp + 3 } else { mp - 9 } as u32;
    let year = y + if m <= 2 { 1 } else { 0 };
    (year, m, d)
}
