// Excluded under `disable-logging`: `alog!` becomes a no-op under that
// feature, which would make these assertions fail by design rather than by
// bug. See `disable_logging_test.rs` for that feature's own coverage.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{alog, Config, Filters, Level, MessageLevel, Writer};
use common::{test_lock, CaptureSink};
use std::collections::HashMap;

#[test]
fn default_level_allows_info_blocks_debug() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    alog!("TEST", MessageLevel::Info, "visible");
    alog!("TEST", MessageLevel::Debug, "hidden");

    let contents = sink.contents();
    assert!(contents.contains("visible"));
    assert!(!contents.contains("hidden"));
}

#[test]
fn per_channel_override() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    let mut filters = HashMap::new();
    filters.insert("VERBOSE".to_string(), Level::Debug);
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        filters: Filters::Map(filters),
        ..Default::default()
    });

    alog!("VERBOSE", MessageLevel::Debug, "verbose debug");
    alog!("TEST", MessageLevel::Debug, "default debug");

    let contents = sink.contents();
    assert!(contents.contains("verbose debug"));
    assert!(!contents.contains("default debug"));
}

#[test]
fn off_blocks_channel_entirely() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    let mut filters = HashMap::new();
    filters.insert("SILENT".to_string(), Level::Off);
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        filters: Filters::Map(filters),
        ..Default::default()
    });

    alog!("SILENT", MessageLevel::Fatal, "should never appear");

    assert!(sink.contents().is_empty());
}

#[test]
fn filter_spec_string_parses() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        filters: Filters::Spec("SPEC:debug".to_string()),
        ..Default::default()
    });

    alog!("SPEC", MessageLevel::Debug, "spec debug");
    alog!("OTHER", MessageLevel::Debug, "other debug");

    let contents = sink.contents();
    assert!(contents.contains("spec debug"));
    assert!(!contents.contains("other debug"));
}
