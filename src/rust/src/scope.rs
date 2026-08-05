//! RAII scope guards: indentation, begin/end scoped logs, and metadata.

use std::cell::RefCell;
use std::collections::HashMap;
use std::time::Instant;

use serde_json::{json, Value};

use crate::core;
use crate::level::MessageLevel;
use crate::record::MapData;

thread_local! {
    static INDENT: RefCell<u32> = const { RefCell::new(0) };
    static METADATA: RefCell<HashMap<String, Value>> = RefCell::new(HashMap::new());
}

pub(crate) fn indent_level() -> u32 {
    INDENT.with(|indent| *indent.borrow())
}

fn push_indent() {
    INDENT.with(|indent| *indent.borrow_mut() += 1);
}

fn pop_indent() {
    INDENT.with(|indent| {
        let mut indent = indent.borrow_mut();
        if *indent > 0 {
            *indent -= 1;
        }
    });
}

pub(crate) fn metadata_snapshot() -> Option<MapData> {
    METADATA.with(|metadata| {
        let metadata = metadata.borrow();
        if metadata.is_empty() {
            None
        } else {
            let mut map = MapData::new();
            for (key, value) in metadata.iter() {
                map.insert(key.clone(), value.clone());
            }
            Some(map)
        }
    })
}

/// Inserts every key/value pair from `map` into the current thread's
/// metadata, returning the keys that were inserted so they can later be
/// removed by [`metadata_remove`].
fn metadata_set(map: MapData) -> Vec<String> {
    METADATA.with(|metadata| {
        let mut metadata = metadata.borrow_mut();
        let mut keys = Vec::with_capacity(map.len());
        for (key, value) in map.into_iter() {
            keys.push(key.clone());
            metadata.insert(key, value);
        }
        keys
    })
}

fn metadata_remove(keys: &[String]) {
    METADATA.with(|metadata| {
        let mut metadata = metadata.borrow_mut();
        for key in keys {
            metadata.remove(key);
        }
    });
}

/// Increments the current thread's indentation level at construction and
/// decrements it at destruction. Indentation is thread-local, so it composes
/// safely with concurrent logging from other threads.
pub struct ScopedIndent(());

impl ScopedIndent {
    pub fn new() -> Self {
        push_indent();
        ScopedIndent(())
    }
}

impl Default for ScopedIndent {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for ScopedIndent {
    fn drop(&mut self) {
        pop_indent();
    }
}

/// Logs a `"BEGIN: {message}"` record at construction and a
/// `"END: {message}"` record at destruction, indenting everything logged in
/// between. Both records (and the indentation change) are skipped entirely
/// if `channel`/`level` is not enabled at construction time.
pub struct ScopedLog {
    channel: &'static str,
    level: MessageLevel,
    message: String,
    enabled: bool,
    indent: Option<ScopedIndent>,
}

impl ScopedLog {
    pub fn new(channel: &'static str, level: MessageLevel, message: impl Into<String>) -> Self {
        let message = message.into();
        let enabled = core::is_enabled(channel, level);
        if enabled {
            core::__log_impl(channel, level, format!("BEGIN: {message}"), None);
        }
        Self {
            channel,
            level,
            message,
            enabled,
            indent: enabled.then(ScopedIndent::new),
        }
    }
}

impl Drop for ScopedLog {
    fn drop(&mut self) {
        if self.enabled {
            // Drop the indent before logging the end message so that it is
            // logged back at the outer indentation level.
            self.indent = None;
            core::__log_impl(
                self.channel,
                self.level,
                format!("END: {}", self.message),
                None,
            );
        }
    }
}

/// Starts a clock at construction (only if `channel`/`level` is enabled) and,
/// at destruction, logs the elapsed time as a human-readable message with a
/// `duration_ms` field attached (per the spec, always the floating-point
/// number of milliseconds elapsed).
pub struct ScopedTimer {
    channel: &'static str,
    level: MessageLevel,
    message: String,
    start: Option<Instant>,
}

impl ScopedTimer {
    pub fn new(channel: &'static str, level: MessageLevel, message: impl Into<String>) -> Self {
        let message = message.into();
        let start = core::is_enabled(channel, level).then(Instant::now);
        Self {
            channel,
            level,
            message,
            start,
        }
    }
}

impl Drop for ScopedTimer {
    fn drop(&mut self) {
        let Some(start) = self.start else { return };
        let elapsed = start.elapsed();
        let nanos = elapsed.as_nanos() as f64;

        let (value, suffix) = if nanos >= 100_000_000.0 {
            (elapsed.as_secs_f64(), "s")
        } else if nanos >= 1_000_000.0 {
            (nanos / 1_000_000.0, "ms")
        } else if nanos >= 1_000.0 {
            (nanos / 1_000.0, "us")
        } else {
            (nanos, "ns")
        };

        let mut extra = MapData::new();
        extra.insert("duration_ms".to_string(), json!(nanos / 1_000_000.0));

        let message = format!("{}{:.3}{}", self.message, value, suffix);
        core::__log_impl(self.channel, self.level, message, Some(extra));
    }
}

/// Adds a set of key/value pairs to the current thread's metadata at
/// construction, and removes exactly those keys (by name) at destruction.
/// Metadata is merged into every record's `extra` data (under a nested
/// `"metadata"` key) for as long as any `ScopedMetadata` guard is alive.
pub struct ScopedMetadata {
    keys: Vec<String>,
}

impl ScopedMetadata {
    pub fn new(map: MapData) -> Self {
        Self {
            keys: metadata_set(map),
        }
    }
}

impl Drop for ScopedMetadata {
    fn drop(&mut self) {
        metadata_remove(&self.keys);
    }
}
