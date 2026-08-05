// Excluded under `disable-logging`: `alog!` becomes a no-op under that
// feature, which would make these assertions fail by design rather than by
// bug. See `disable_logging_test.rs` for that feature's own coverage.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{alog, Config, Filters, FormatterKind, Level, MessageLevel, Writer};
use common::{test_lock, CaptureSink};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

#[test]
fn reconfigure_fully_replaces_state() {
    let _guard = test_lock();
    let sink1 = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Debug,
        formatter: FormatterKind::Pretty,
        writer: Writer::Custom(Box::new(sink1.clone())),
        ..Default::default()
    });
    alog!("TEST", MessageLevel::Debug, "goes to sink1");
    assert!(sink1.contents().contains("goes to sink1"));

    let sink2 = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Error,
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink2.clone())),
        ..Default::default()
    });
    alog!("TEST", MessageLevel::Debug, "should not appear anywhere");
    alog!("TEST", MessageLevel::Error, "goes to sink2 as json");

    assert!(!sink1.contents().contains("should not appear"));
    assert!(!sink2.contents().contains("should not appear"));
    let out = sink2.contents();
    assert!(out.trim_end().starts_with('{'));
    assert!(out.contains("goes to sink2 as json"));
}

#[test]
fn adjust_levels_leaves_formatter_and_sink_untouched() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Info,
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    alog::adjust_levels(Level::Debug, Filters::None);
    alog!("TEST", MessageLevel::Debug, "now visible");

    let out = sink.contents();
    assert!(out.trim_end().starts_with('{'));
    assert!(out.contains("now visible"));
}

#[test]
fn concurrent_configure_and_logging_does_not_panic() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let stop = Arc::new(AtomicBool::new(false));
    let logger_stop = stop.clone();
    let logger = std::thread::spawn(move || {
        while !logger_stop.load(Ordering::Relaxed) {
            alog!("TEST", MessageLevel::Info, "logging");
        }
    });

    for _ in 0..50 {
        alog::configure(Config {
            writer: Writer::Custom(Box::new(sink.clone())),
            ..Default::default()
        });
    }

    stop.store(true, Ordering::Relaxed);
    logger.join().unwrap();
}
