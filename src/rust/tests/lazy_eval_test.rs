// Excluded under `disable-logging`: `alog!` becomes a no-op under that
// feature, which would trivially defeat this file's whole purpose (proving
// arguments are lazily evaluated by a macro that, under this feature,
// evaluates nothing at all).
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{alog, Config, Level, MessageLevel, Writer};
use common::{test_lock, CaptureSink};
use std::sync::atomic::{AtomicUsize, Ordering};

#[test]
fn disabled_channel_never_evaluates_arguments() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Info,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    static CALLS: AtomicUsize = AtomicUsize::new(0);
    fn expensive() -> usize {
        CALLS.fetch_add(1, Ordering::SeqCst);
        42
    }

    alog!("TEST", MessageLevel::Debug, "value is {}", expensive());

    assert_eq!(CALLS.load(Ordering::SeqCst), 0);
    assert!(sink.contents().is_empty());
}

#[test]
fn enabled_channel_evaluates_arguments_exactly_once() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Debug,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    static CALLS: AtomicUsize = AtomicUsize::new(0);
    fn expensive() -> usize {
        CALLS.fetch_add(1, Ordering::SeqCst);
        42
    }

    alog!("TEST", MessageLevel::Debug, "value is {}", expensive());

    assert_eq!(CALLS.load(Ordering::SeqCst), 1);
    assert!(sink.contents().contains("value is 42"));
}
