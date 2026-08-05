// Excluded under `disable-logging`: every macro exercised here becomes a
// no-op (or, for `alog_is_enabled_channel!`, an unconditional `false`) under
// that feature, which would make these assertions fail by design rather than
// by bug. See `disable_logging_test.rs` for that feature's own coverage.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{
    alog, alog_channel, alog_fn_channel, alog_is_enabled_channel, alog_map_channel,
    alog_scoped_block_channel, alog_scoped_timer_channel, use_channel, Config, Filters, Level,
    MapData, MessageLevel, Writer,
};
use common::{test_lock, CaptureSink};
use serde_json::Value;
use std::collections::HashMap;

use_channel!("CHANTEST");

fn log_via_channel_macro() {
    alog_channel!(MessageLevel::Info, "via channel macro");
}

fn traced_via_channel_macro() {
    alog_fn_channel!();
}

#[test]
fn channel_macro_matches_explicit_channel() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    log_via_channel_macro();
    alog!("CHANTEST", MessageLevel::Info, "via explicit channel");

    let lines = sink.lines();
    assert_eq!(lines.len(), 2);
    assert!(lines[0].contains("CHANT"));
    assert!(lines[0].contains("via channel macro"));
    assert!(lines[1].contains("via explicit channel"));
}

#[test]
fn filtering_out_channel_blocks_channel_macro() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    let mut filters = HashMap::new();
    filters.insert("CHANTEST".to_string(), Level::Off);
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        filters: Filters::Map(filters),
        ..Default::default()
    });

    log_via_channel_macro();

    assert!(sink.contents().is_empty());
}

#[test]
fn alog_map_channel_matches_bound_channel() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: alog::FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let mut extra = MapData::new();
    extra.insert("k".to_string(), serde_json::json!("v"));
    alog_map_channel!(MessageLevel::Info, extra, "via map channel macro");

    let parsed: Value = serde_json::from_str(sink.contents().trim_end()).unwrap();
    assert_eq!(parsed["channel"], "CHANTEST");
    assert_eq!(parsed["k"], "v");
    assert_eq!(parsed["message"], "via map channel macro");
}

#[test]
fn alog_is_enabled_channel_matches_bound_channel() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    let mut filters = HashMap::new();
    filters.insert("CHANTEST".to_string(), Level::Warning);
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        filters: Filters::Map(filters),
        ..Default::default()
    });

    assert!(alog_is_enabled_channel!(MessageLevel::Warning));
    assert!(!alog_is_enabled_channel!(MessageLevel::Info));
}

#[test]
fn alog_fn_channel_uses_bound_channel() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Trace,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    traced_via_channel_macro();

    let lines = sink.lines();
    assert_eq!(lines.len(), 2);
    assert!(lines[0].contains("CHANT"));
    assert!(lines[0].ends_with("] BEGIN: channel_test::traced_via_channel_macro()"));
    assert!(lines[1].ends_with("] END: channel_test::traced_via_channel_macro()"));
}

#[test]
fn alog_scoped_block_channel_and_scoped_timer_channel_use_bound_channel() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        alog_scoped_block_channel!(MessageLevel::Info, "block via channel");
    }
    {
        alog_scoped_timer_channel!(MessageLevel::Info, "timer via channel");
    }

    let lines = sink.lines();
    assert_eq!(lines.len(), 4);
    assert!(lines[0].contains("CHANT"));
    assert!(lines[0].ends_with("] BEGIN: block via channel"));
    assert!(lines[1].ends_with("] END: block via channel"));
    assert!(lines[2].contains("] timer via channel"));
    assert!(lines[3].contains("duration_ms"));
}
