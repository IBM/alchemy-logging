// Excluded under `disable-logging`: `alog!` becomes a no-op under that
// feature, which would make these assertions fail by design rather than by
// bug. See `disable_logging_test.rs` for proof that the `Scoped*` types
// themselves (constructed directly, as they are here) keep working under
// that feature.
#![cfg(not(feature = "disable-logging"))]

mod common;

use alog::{
    alog, Config, FormatterKind, Level, MapData, MessageLevel, ScopedIndent, ScopedLog,
    ScopedMetadata, ScopedTimer, Writer,
};
use common::{test_lock, CaptureSink};
use serde_json::Value;

#[test]
fn scoped_log_emits_begin_and_end_with_indentation() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        let _scope = ScopedLog::new("TEST", MessageLevel::Info, "doing work");
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
fn scoped_log_indentation_is_symmetric_across_panic() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let result = std::panic::catch_unwind(|| {
        let _scope = ScopedLog::new("TEST", MessageLevel::Info, "will panic");
        panic!("boom");
    });
    assert!(result.is_err());

    alog!("TEST", MessageLevel::Info, "after panic");
    let lines = sink.lines();
    let last = lines.last().unwrap();
    assert!(last.ends_with("] after panic"));
}

#[test]
fn disabled_scoped_log_emits_nothing() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Error,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        let _scope = ScopedLog::new("TEST", MessageLevel::Info, "quiet");
    }

    assert!(sink.contents().is_empty());
}

#[test]
fn scoped_indent_nests_and_unwinds() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    alog!("TEST", MessageLevel::Info, "level0");
    {
        let _i1 = ScopedIndent::new();
        alog!("TEST", MessageLevel::Info, "level1");
        {
            let _i2 = ScopedIndent::new();
            alog!("TEST", MessageLevel::Info, "level2");
        }
        alog!("TEST", MessageLevel::Info, "level1-again");
    }
    alog!("TEST", MessageLevel::Info, "level0-again");

    let lines = sink.lines();
    assert_eq!(lines.len(), 5);
    assert!(lines[0].ends_with("] level0"));
    assert!(lines[1].ends_with("]   level1"));
    assert!(lines[2].ends_with("]     level2"));
    assert!(lines[3].ends_with("]   level1-again"));
    assert!(lines[4].ends_with("] level0-again"));
}

#[test]
fn scoped_timer_logs_duration_ms_extra() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        let _timer = ScopedTimer::new("TEST", MessageLevel::Info, "did work in ");
        std::thread::sleep(std::time::Duration::from_millis(5));
    }

    let out = sink.contents();
    let parsed: Value = serde_json::from_str(out.trim_end()).unwrap();
    let duration_ms = parsed["duration_ms"]
        .as_f64()
        .expect("duration_ms present as f64");
    assert!(
        duration_ms >= 4.0,
        "expected at least ~5ms, got {duration_ms}"
    );
}

#[test]
fn disabled_scoped_timer_emits_nothing() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        default_level: Level::Error,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    {
        let _timer = ScopedTimer::new("TEST", MessageLevel::Info, "quiet");
    }

    assert!(sink.contents().is_empty());
}

#[test]
fn scoped_metadata_attaches_and_removes_keys() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let mut map = MapData::new();
    map.insert("request_id".to_string(), serde_json::json!("abc"));
    {
        let _meta = ScopedMetadata::new(map);
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

#[test]
fn nested_scoped_metadata_is_robust_to_nesting() {
    let _guard = test_lock();
    let sink = CaptureSink::new();
    alog::configure(Config {
        formatter: FormatterKind::Json,
        writer: Writer::Custom(Box::new(sink.clone())),
        ..Default::default()
    });

    let mut outer_map = MapData::new();
    outer_map.insert("outer_key".to_string(), serde_json::json!("outer"));
    let _outer = ScopedMetadata::new(outer_map);
    {
        let mut inner_map = MapData::new();
        inner_map.insert("inner_key".to_string(), serde_json::json!("inner"));
        let _inner = ScopedMetadata::new(inner_map);
        alog!("TEST", MessageLevel::Info, "nested");
    }
    alog!("TEST", MessageLevel::Info, "outer only");

    let lines = sink.lines();
    let nested: Value = serde_json::from_str(&lines[0]).unwrap();
    assert_eq!(nested["metadata"]["outer_key"], "outer");
    assert_eq!(nested["metadata"]["inner_key"], "inner");

    let outer_only: Value = serde_json::from_str(&lines[1]).unwrap();
    assert_eq!(outer_only["metadata"]["outer_key"], "outer");
    assert!(outer_only["metadata"].get("inner_key").is_none());
}
