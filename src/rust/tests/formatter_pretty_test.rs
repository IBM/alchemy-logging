use alog::{Formatter, LogRecord, MapData, MessageLevel, PrettyFormatter};

#[test]
fn header_includes_padded_channel_and_level_abbrev() {
    let formatter = PrettyFormatter::default();
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "2024-01-01T00:00:00.000Z",
        message: "hello",
        num_indent: 0,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out, "2024-01-01T00:00:00.000Z [TEST :INFO] hello\n");
}

#[test]
fn multiline_message_repeats_header_per_line() {
    let formatter = PrettyFormatter::default();
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "line one\nline two",
        num_indent: 0,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out, "TS [TEST :INFO] line one\nTS [TEST :INFO] line two\n");
}

#[test]
fn indentation_repeats_two_spaces_per_level() {
    let formatter = PrettyFormatter::default();
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "nested",
        num_indent: 2,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out, "TS [TEST :INFO]     nested\n");
}

#[test]
fn thread_id_appears_in_header_when_present() {
    let formatter = PrettyFormatter::default();
    let record = LogRecord {
        channel: "TEST",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "msg",
        num_indent: 0,
        thread_id: Some("7"),
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out, "TS [TEST :INFO:7] msg\n");
}

#[test]
fn channel_longer_than_width_is_truncated() {
    let formatter = PrettyFormatter::default();
    let record = LogRecord {
        channel: "LONGCHANNEL",
        level: MessageLevel::Info,
        timestamp: "TS",
        message: "msg",
        num_indent: 0,
        thread_id: None,
        extra: None,
    };

    let out = formatter.format(&record);
    assert_eq!(out, "TS [LONGC:INFO] msg\n");
}

#[test]
fn extra_map_renders_as_bulleted_lines() {
    let formatter = PrettyFormatter::default();
    let mut extra = MapData::new();
    extra.insert("count".to_string(), serde_json::json!(3));
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
    assert_eq!(out, "TS [TEST :INFO] msg\nTS [TEST :INFO]  * count: 3\n");
}
