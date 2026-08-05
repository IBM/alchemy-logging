// Excluded under `disable-logging`: `alog!` becomes a no-op under that
// feature, which would make these assertions fail by design rather than by
// bug. See `disable_logging_test.rs` for that feature's own coverage.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{alog, Config, FormatterKind, MessageLevel, ScopedIndent, Writer};
use common::{test_lock, CaptureSink};

#[test]
fn thread_id_field_is_opt_in() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        thread_id: false,
        ..Default::default()
    });
    alog!("TEST", MessageLevel::Info, "no thread id");
    let parsed: serde_json::Value = serde_json::from_str(sink.contents().trim_end()).unwrap();
    assert!(parsed.get("thread_id").is_none());
}

#[test]
fn thread_id_field_present_when_enabled() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        thread_id: true,
        ..Default::default()
    });
    alog!("TEST", MessageLevel::Info, "with thread id");
    let parsed: serde_json::Value = serde_json::from_str(sink.contents().trim_end()).unwrap();
    assert!(parsed.get("thread_id").is_some());
}

#[test]
fn indentation_is_isolated_per_thread() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let _outer_indent = ScopedIndent::new();
    alog!("TEST", MessageLevel::Info, "main thread indented");

    let handle = std::thread::spawn(|| {
        alog!("TEST", MessageLevel::Info, "spawned thread not indented");
    });
    handle.join().unwrap();

    let lines = sink.lines();
    assert_eq!(lines.len(), 2);
    assert!(lines[0].ends_with("]   main thread indented"));
    assert!(lines[1].ends_with("] spawned thread not indented"));
}
