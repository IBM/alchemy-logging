use alog::{Formatter, JsonFormatter, Level, LogRecord, MapData, MessageLevel};
use serde_json::Value;

#[test]
fn required_fields_present_with_correct_types() {
    let formatter = JsonFormatter;
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "2024-01-01T00:00:00.000Z",
        message: "hello",
        num_indent: 1,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    let parsed: Value = serde_json::from_str(out.trim_end()).unwrap();

    assert_eq!(parsed["channel"], "TEST");
    assert_eq!(parsed["level"], Level::Info.ordinal());
    assert_eq!(parsed["level_str"], "info");
    assert_eq!(parsed["timestamp"], "2024-01-01T00:00:00.000Z");
    assert_eq!(parsed["message"], "hello");
    assert_eq!(parsed["num_indent"], 1);
    assert!(parsed.get("thread_id").is_none());
}

#[test]
fn thread_id_present_when_set() {
    let formatter = JsonFormatter;
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "msg",
        num_indent: 0,
        thread_id: Some("3"),
        extra: None,
    };

    let out = formatter.format(&record);
    let parsed: Value = serde_json::from_str(out.trim_end()).unwrap();
    assert_eq!(parsed["thread_id"], "3");
}

#[test]
fn extra_keys_are_flattened_to_top_level() {
    let formatter = JsonFormatter;
    let mut extra = MapData::new();
    extra.insert("request_id".to_string(), serde_json::json!("abc-123"));
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "msg",
        num_indent: 0,
        thread_id: None,
        extra: Some(&extra),
    };

    let out = formatter.format(&record);
    let parsed: Value = serde_json::from_str(out.trim_end()).unwrap();
    assert_eq!(parsed["request_id"], "abc-123");
    assert!(!parsed.as_object().unwrap().contains_key("extra"));
}

#[test]
fn output_is_single_line() {
    let formatter = JsonFormatter;
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "multi\nline",
        num_indent: 0,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out.trim_end().lines().count(), 1);
}
