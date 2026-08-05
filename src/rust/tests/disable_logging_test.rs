//! Verifies the Rust analog of `cpp`'s `-D ALOG_DISABLE_LOGGING`: with the
//! `disable-logging` feature enabled, every logging macro becomes a no-op,
//! but directly constructing a `Scoped*` guard (the escape hatch for named
//! instances, e.g. a `ScopedTimer` queried mid-scope) still logs normally.
//!
//! Gated on the feature so this file is entirely excluded from a default
//! build - it would otherwise assert on output the default build's macros
//! actually produce.
#![cfg(feature = "disable-logging")]

mod common;

use alog::{
    alog, alog_channel, alog_fn, alog_fn_channel, alog_is_enabled, alog_is_enabled_channel,
    alog_map, alog_map_channel, alog_scoped_block, alog_scoped_block_channel, alog_scoped_indent,
    alog_scoped_metadata, alog_scoped_timer, alog_scoped_timer_channel, use_channel, Config, Level,
    MapData, MessageLevel, ScopedIndent, ScopedLog, ScopedMetadata, ScopedTimer, Writer,
};
use common::{test_lock, CaptureSink};

use_channel!("TEST");

#[test]
fn every_macro_is_a_no_op_when_disabled() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Trace,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    alog!("TEST", MessageLevel::Info, "message");
    alog_map!("TEST", MessageLevel::Info, MapData::new(), "message");
    alog_channel!(MessageLevel::Info, "message");
    alog_map_channel!(MessageLevel::Info, MapData::new(), "message");
    alog_fn!("TEST");
    alog_fn_channel!();
    alog_scoped_block!("TEST", MessageLevel::Info, "message");
    alog_scoped_block_channel!(MessageLevel::Info, "message");
    alog_scoped_timer!("TEST", MessageLevel::Info, "message");
    alog_scoped_timer_channel!(MessageLevel::Info, "message");
    alog_scoped_indent!();
    alog_scoped_metadata!(MapData::new());

    assert!(sink.contents().is_empty());
}

#[test]
#[allow(clippy::assertions_on_constants)]
fn alog_is_enabled_macros_are_always_false_when_disabled() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Trace,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    assert!(!alog_is_enabled!("TEST", MessageLevel::Info));
    assert!(!alog_is_enabled_channel!(MessageLevel::Info));
}

#[test]
fn directly_constructed_scoped_guards_still_log_when_disabled() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Trace,
        formatter: alog::FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        let _indent = ScopedIndent::new();
        let _scope = ScopedLog::new("TEST", MessageLevel::Info, "still works");
        let timer = ScopedTimer::new("TEST", MessageLevel::Info, "timed ");
        let mut map = MapData::new();
        map.insert("k".to_string(), serde_json::json!("v"));
        let _meta = ScopedMetadata::new(map);
        drop(timer);
    }

    let lines = sink.lines();
    assert_eq!(lines.len(), 3, "expected BEGIN, timer, END");
    assert!(lines[0].contains("BEGIN: still works"));
    assert!(lines[1].contains("duration_ms"));
    assert!(lines[1].contains("\"k\":\"v\""));
    assert!(lines[2].contains("END: still works"));
    assert!(!lines[2].contains("metadata"));
}
