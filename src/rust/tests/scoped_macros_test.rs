//! Covers the statement-form `alog_scoped_*!` macros, which bind their guard
//! to a hidden variable for you. `scope_test.rs` covers the underlying
//! `Scoped*` types directly; this file covers the macro plumbing on top,
//! including that multiple invocations in the same lexical scope coexist
//! without a variable-name collision (Rust's macro hygiene, not a
//! `__LINE__`-style trick).
//!
//! Excluded under `disable-logging`: every macro exercised here becomes a
//! no-op under that feature, which would make these assertions fail by
//! design rather than by bug. See `disable_logging_test.rs` for that
//! feature's own coverage.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{
    alog, alog_scoped_block, alog_scoped_indent, alog_scoped_metadata, alog_scoped_timer, Config,
    Level, MapData, MessageLevel, Writer,
};
use common::{test_lock, CaptureSink};
use serde_json::Value;

#[test]
fn alog_scoped_block_emits_begin_and_end() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        alog_scoped_block!("TEST", MessageLevel::Info, "doing work");
        alog!("TEST", MessageLevel::Info, "inside");
    }
    alog!("TEST", MessageLevel::Info, "outside");

    let lines = sink.lines();
    assert_eq!(lines.len(), 4);
    assert!(lines[0].ends_with("] BEGIN: doing work"));
    assert!(lines[1].ends_with("]   inside"));
    assert!(lines[2].ends_with("] END: doing work"));
    assert!(lines[3].ends_with("] outside"));
}

#[test]
fn two_scoped_blocks_in_the_same_lexical_scope_do_not_collide() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        // Two hidden `_alog_scoped_block` bindings live in the same block.
        // Hygiene keeps them distinct; both must still log their own
        // BEGIN/END, dropping in reverse (LIFO) order at the end of the
        // block.
        alog_scoped_block!("TEST", MessageLevel::Info, "outer");
        alog_scoped_block!("TEST", MessageLevel::Info, "inner");
    }

    let lines = sink.lines();
    assert_eq!(lines.len(), 4);
    assert!(lines[0].ends_with("] BEGIN: outer"));
    assert!(lines[1].ends_with("]   BEGIN: inner"));
    assert!(lines[2].ends_with("]   END: inner"));
    assert!(lines[3].ends_with("] END: outer"));
}

#[test]
fn disabled_scoped_block_emits_nothing() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Error,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        alog_scoped_block!("TEST", MessageLevel::Info, "quiet");
    }

    assert!(sink.contents().is_empty());
}

#[test]
fn alog_scoped_timer_logs_duration_ms_extra() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: alog::FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        alog_scoped_timer!("TEST", MessageLevel::Info, "did work in ");
        std::thread::sleep(std::time::Duration::from_millis(5));
    }

    let parsed: Value = serde_json::from_str(sink.contents().trim_end()).unwrap();
    let duration_ms = parsed["duration_ms"]
        .as_f64()
        .expect("duration_ms present as f64");
    assert!(
        duration_ms >= 4.0,
        "expected at least ~5ms, got {duration_ms}"
    );
}

#[test]
fn alog_scoped_indent_nests_within_the_macro() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    alog!("TEST", MessageLevel::Info, "level0");
    {
        alog_scoped_indent!();
        alog!("TEST", MessageLevel::Info, "level1");
    }
    alog!("TEST", MessageLevel::Info, "level0-again");

    let lines = sink.lines();
    assert_eq!(lines.len(), 3);
    assert!(lines[0].ends_with("] level0"));
    assert!(lines[1].ends_with("]   level1"));
    assert!(lines[2].ends_with("] level0-again"));
}

#[test]
fn alog_scoped_metadata_attaches_and_removes_keys() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: alog::FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let mut map = MapData::new();
    map.insert("request_id".to_string(), serde_json::json!("abc"));
    {
        alog_scoped_metadata!(map);
        alog!("TEST", MessageLevel::Info, "inside metadata scope");
    }
    alog!("TEST", MessageLevel::Info, "outside metadata scope");

    let lines = sink.lines();
    assert_eq!(lines.len(), 2);
    let inside: Value = serde_json::from_str(&lines[0]).unwrap();
    assert_eq!(inside["metadata"]["request_id"], "abc");
    let outside: Value = serde_json::from_str(&lines[1]).unwrap();
    assert!(outside.get("metadata").is_none());
}
